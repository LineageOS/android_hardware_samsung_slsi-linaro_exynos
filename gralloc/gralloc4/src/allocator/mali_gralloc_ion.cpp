/*
 * Copyright (C) 2016-2020 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>

#include <log/log.h>
#include <cutils/atomic.h>

#include <linux/dma-buf.h>
#include <vector>
#include <sys/ioctl.h>

#include <hardware/hardware.h>
#include <hardware/gralloc1.h>

#include <hardware/exynos/ion.h>
#include <hardware/exynos/dmabuf_container.h>

#include "mali_gralloc_buffer.h"
#include "gralloc_helper.h"
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"
#include "core/mali_gralloc_bufferdescriptor.h"
#include "core/mali_gralloc_bufferallocation.h"

#include "mali_gralloc_ion.h"

#define INIT_ZERO(obj) (memset(&(obj), 0, sizeof((obj))))

#define HEAP_MASK_FROM_ID(id) (1 << id)
#define HEAP_MASK_FROM_TYPE(type) (1 << type)

#if defined(ION_HEAP_SECURE_MASK)
#if (HEAP_MASK_FROM_TYPE(ION_HEAP_TYPE_SECURE) != ION_HEAP_SECURE_MASK)
#error "ION_HEAP_TYPE_SECURE value is not compatible with ION_HEAP_SECURE_MASK"
#endif
#endif

struct ion_device
{
	int client()
	{
		return ion_client;
	}

	static void close()
	{
		ion_device &dev = get_inst();
		if (dev.ion_client >= 0)
		{
			exynos_ion_close(dev.ion_client);
			dev.ion_client = -1;
		}
	}

	static ion_device *get()
	{
		ion_device &dev = get_inst();
		if (dev.ion_client < 0)
		{
			if (dev.open_and_query_ion() != 0)
			{
				close();
			}
		}

		if (dev.ion_client < 0)
		{
			return nullptr;
		}
		return &dev;
	}

	/*
	 *  Identifies a heap and retrieves file descriptor from ION for allocation
	 *
	 * @param usage     [in]    Producer and consumer combined usage.
	 * @param size      [in]    Requested buffer size (in bytes).
	 * @param heap_type [in]    Requested heap type.
	 * @param flags     [in]    ION allocation attributes defined by ION_FLAG_*.
	 * @param min_pgsz  [out]   Minimum page size (in bytes).
	 *
	 * @return File handle which can be used for allocation, on success
	 *         -1, otherwise.
	 */
	int alloc_from_ion_heap(uint64_t usage, size_t size, unsigned int flags, int *min_pgsz);

private:
	int ion_client;

	ion_device()
	    : ion_client(-1)
	{
	}

	static ion_device& get_inst()
	{
		static ion_device dev;
		return dev;
	}

	/*
	 * Opens the ION module. Queries heap information and stores it for later use
	 *
	 * @return              0 in case of success
	 *                      -1 for all error cases
	 */
	int open_and_query_ion();
};

static void set_ion_flags(uint64_t usage, unsigned int *ion_flags)
{
	if (ion_flags == nullptr)
		return;

	if ((usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN)
	{
		*ion_flags |= ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

		if (usage & GRALLOC_USAGE_HW_RENDER)
		{
			*ion_flags |= ION_FLAG_SYNC_FORCE;
		}
	}

	if (usage & (GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE))
	{
		*ion_flags |= ION_FLAG_MAY_HWRENDER;
	}

	if (usage & GRALLOC_USAGE_NOZEROED)
	{
		*ion_flags |= ION_FLAG_NOZEROED;
	}

	/* TODO: does not seem to be used anymore. But check again to make sure */
#ifdef GRALLOC_PROTECTED_ION_FLAG_FOR_CAMERA_RESERVED
	if (usage & GRALLOC_USAGE_CAMERA_RESERVED)
	{
		*ion_flags |= ION_FLAG_PROTECTED;
	}
#endif
	// DRM or Secure Camera
	if (usage & (GRALLOC_USAGE_PROTECTED | GRALLOC_USAGE_SECURE_CAMERA_RESERVED))
	{
		/* W/A to Android R Camera vts_r5. (W/A requested by Visual S/W group MCD) */
		if(!(usage & GRALLOC_USAGE_CAMERA_RESERVED))
		{
			*ion_flags |= ION_FLAG_PROTECTED;
		}
	}

	/* TODO: used for exynos3830. Add this as an option to Android.bp */
#if defined(GRALLOC_SCALER_WFD) && GRALLOC_SCALER_WFD == 1
	if (usage & GRALLOC_USAGE_PRIVATE_NONSECURE && usage & GRALLOC_USAGE_HW_COMPOSER)
	{
		*ion_flags |= ION_FLAG_PROTECTED;
	}
#endif
}

static unsigned int select_heap_mask(uint64_t usage)
{
	unsigned int heap_mask;
	if (usage & GRALLOC_USAGE_PROTECTED)
	{
		if (usage & GRALLOC_USAGE_PRIVATE_NONSECURE)
		{
			heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
		}
		else if (usage & GRALLOC_USAGE_CAMERA_RESERVED)
		{
			/* W/A to Android R Camera vts_r5. (W/A requested by Visual S/W group MCD) */
			heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
		}
		else if (usage & GRALLOC_USAGE_SECURE_CAMERA_RESERVED)
		{
			heap_mask = EXYNOS_ION_HEAP_SECURE_CAMERA_MASK;
		}
		else if (usage & GRALLOC_USAGE_VIDEO_EXT)
		{
			heap_mask = EXYNOS_ION_HEAP_VIDEO_STREAM_MASK;
		}
		else if ((usage & GRALLOC_USAGE_HW_COMPOSER) &&
			!(usage & GRALLOC_USAGE_HW_TEXTURE) &&
			!(usage & GRALLOC_USAGE_HW_RENDER))
		{
			heap_mask = EXYNOS_ION_HEAP_VIDEO_SCALER_MASK;
		}
		else
		{
			heap_mask = EXYNOS_ION_HEAP_VIDEO_FRAME_MASK;
		}
	}
	/* TODO: used for exynos3830. Add this as a an option to Android.bp */
#if defined(GRALLOC_SCALER_WFD) && GRALLOC_SCALER_WFD == 1
	else if (usage & GRALLOC_USAGE_PRIVATE_NONSECURE && usage & GRALLOC_USAGE_HW_COMPOSER)
	{
		heap_mask = EXYNOS_ION_HEAP_EXT_UI_MASK;
	}
#endif
	else if (usage & GRALLOC_USAGE_CAMERA_RESERVED)
	{
		heap_mask = EXYNOS_ION_HEAP_CAMERA_MASK;
	}
	else if (usage & GRALLOC_USAGE_SECURE_CAMERA_RESERVED)
	{
		heap_mask = EXYNOS_ION_HEAP_SECURE_CAMERA_MASK;
	}
	else
	{
		heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
	}

	return heap_mask;
}

int ion_device::alloc_from_ion_heap(uint64_t usage, size_t size, unsigned int flags, int *min_pgsz)
{
	int shared_fd = -1;
	int ret = -1;

	/* TODO: remove min_pgsz? I don't think this is useful on Exynos */
	if (ion_client < 0 ||
	    size <= 0 ||
	    min_pgsz == NULL)
	{
		return -1;
	}

	unsigned int heap_mask = select_heap_mask(usage);

	shared_fd = exynos_ion_alloc(ion_client, size, heap_mask, flags);

	*min_pgsz = SZ_4K;

	return shared_fd;
}

int ion_device::open_and_query_ion()
{
	int ret = -1;

	if (ion_client >= 0)
	{
		MALI_GRALLOC_LOGW("ION device already open");
		return 0;
	}

	ion_client = exynos_ion_open();
	if (ion_client < 0)
	{
		MALI_GRALLOC_LOGE("ion_open failed with %s", strerror(errno));
		return -1;
	}

	return 0;
}

static int mali_gralloc_ion_sync(const private_handle_t * const hnd,
                                       const bool read,
                                       const bool write,
                                       const bool start)
{
	int ret = 0;

	if (hnd == NULL)
	{
		return -EINVAL;
	}

	ion_device *dev = ion_device::get();
	int direction = 0;

	if (read)
	{
		direction |= ION_SYNC_READ;
	}
	if (write)
	{
		direction |= ION_SYNC_WRITE;
	}

	for (int idx = 0; idx < hnd->fd_count; idx++)
	{
		if (start)
		{
			ret |= exynos_ion_sync_start(dev->client(), hnd->fds[idx], direction);
		}
		else
		{
			ret |= exynos_ion_sync_end(dev->client(), hnd->fds[idx], direction);
		}
	}

	return ret;
}


/*
 * Signal start of CPU access to the DMABUF exported from ION.
 *
 * @param hnd   [in]    Buffer handle
 * @param read  [in]    Flag indicating CPU read access to memory
 * @param write [in]    Flag indicating CPU write access to memory
 *
 * @return              0 in case of success
 *                      errno for all error cases
 */
int mali_gralloc_ion_sync_start(const private_handle_t * const hnd,
                                const bool read,
                                const bool write)
{
	return mali_gralloc_ion_sync(hnd, read, write, true);
}


/*
 * Signal end of CPU access to the DMABUF exported from ION.
 *
 * @param hnd   [in]    Buffer handle
 * @param read  [in]    Flag indicating CPU read access to memory
 * @param write [in]    Flag indicating CPU write access to memory
 *
 * @return              0 in case of success
 *                      errno for all error cases
 */
int mali_gralloc_ion_sync_end(const private_handle_t * const hnd,
                              const bool read,
                              const bool write)
{
	return mali_gralloc_ion_sync(hnd, read, write, false);
}


void mali_gralloc_ion_free(private_handle_t * const hnd)
{
	for (int i = 0; i < hnd->fd_count; i++)
	{
		void* mapped_addr = reinterpret_cast<void*>(hnd->bases[i]);

		/* Buffer might be unregistered already so we need to assure we have a valid handle */
		if (mapped_addr != nullptr)
		{
			if (munmap(mapped_addr, hnd->alloc_sizes[i]) != 0)
			{
				/* TODO: more detailed error logs */
				MALI_GRALLOC_LOGE("Failed to munmap handle %p", hnd);
			}
		}
		close(hnd->fds[i]);
		hnd->fds[i] = -1;
		hnd->bases[i] = 0;
	}
}

static void mali_gralloc_ion_free_internal(buffer_handle_t * const pHandle,
                                           const uint32_t num_hnds)
{
	for (uint32_t i = 0; i < num_hnds; i++)
	{
		if (pHandle[i] != NULL)
		{
			private_handle_t * const hnd = (private_handle_t * const)pHandle[i];
			mali_gralloc_ion_free(hnd);
		}
	}
}

int mali_gralloc_ion_allocate_attr(private_handle_t *hnd)
{
	ion_device *dev = ion_device::get();
	if (!dev)
	{
		return -1;
	}

	int idx = hnd->get_share_attr_fd_index();
	assert(idx >= 0);
	if(idx == -1)
	{
		/* return undefined share_attr index */
		MALI_GRALLOC_LOGE("failed to get share_attr_fd_index from private_handle");
		return -1;
	}
	int ion_flags = 0;
	int min_pgsz;
	uint64_t usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;

	ion_flags = ION_FLAG_CACHED;

	hnd->fds[idx] = dev->alloc_from_ion_heap(usage, hnd->attr_size, ion_flags, &min_pgsz);
	if (hnd->fds[idx] < 0)
	{
		MALI_GRALLOC_LOGE("ion_alloc failed from client ( %d )", dev->client());
		return -1;
	}

	hnd->incr_numfds(1);

	return 0;
}

static inline int createBufferContainer(int *fd, int size, int *containerFd)
{
	int bufferContainerFd = -1;

	if (fd == NULL || size < 1 || containerFd == NULL)
	{
		MALI_GRALLOC_LOGE("invalid parameters. fd(%p), size(%d), containerFd(%p)",
				fd, size, containerFd);
		return -EINVAL;
	}

	bufferContainerFd = dma_buf_merge(fd[0], fd + 1, size - 1);
	if (bufferContainerFd < 0)
	{
		MALI_GRALLOC_LOGE("fail to create Buffer Container. containerFd(%d), size(%d)",
				bufferContainerFd, size);
		return -EINVAL;
	}

	*containerFd = bufferContainerFd;

	return 0;
}

static int allocate_to_container(buffer_descriptor_t *bufDescriptor, uint32_t ion_flags,
		int *min_pgsz, int fd_arr[])
{
	/* TODO: make batch size as BoardConfig option */
#define GRALLOC_HFR_BATCH_SIZE 8
	const int layer_count = GRALLOC_HFR_BATCH_SIZE;
	int hfr_fds[3][layer_count];
	int err = 0;
	int64_t usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

	ion_device *dev = ion_device::get();
	if (!dev)
	{
		return -1;
	}

	memset(hfr_fds, 0xff, sizeof(hfr_fds));

	for (int layer = 0; layer < layer_count; layer++)
	{
		for (int fidx = 0; fidx < bufDescriptor->fd_count; fidx++)
		{
			hfr_fds[fidx][layer] =dev->alloc_from_ion_heap(usage,
					bufDescriptor->alloc_sizes[fidx], ion_flags, min_pgsz);

			if (hfr_fds[fidx][layer] < 0)
			{
				MALI_GRALLOC_LOGE("ion_alloc failed from client ( %d )", dev->client());
				goto finish;
			}
		}
	}


	for (int fidx = 0; fidx < bufDescriptor->fd_count; fidx++)
	{
		err = createBufferContainer(hfr_fds[fidx], layer_count, &fd_arr[fidx]);
		if (err)
		{
			for (int i = 0; i < fidx; i++)
			{
				if (fd_arr[i] >= 0)
				{
					close(fd_arr[i]);
					fd_arr[i] = -1;
				}
			}

			goto finish;
		}
	}

finish:
	/* free single fds to make ref count to 1 */
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < layer_count; j++)
			if (hfr_fds[i][j] >= 0) close(hfr_fds[i][j]);

	return err;
}

/*
 *  Allocates ION buffers
 *
 * @param descriptors     [in]    Buffer request descriptors
 * @param numDescriptors  [in]    Number of descriptors
 * @param pHandle         [out]   Handle for each allocated buffer
 * @param shared_backend  [out]   Shared buffers flag
 *
 * @return File handle which can be used for allocation, on success
 *         -1, otherwise.
 */
int mali_gralloc_ion_allocate(const gralloc_buffer_descriptor_t *descriptors,
                              uint32_t numDescriptors, buffer_handle_t *pHandle,
                              bool *shared_backend)
{
	GRALLOC_UNUSED(shared_backend);

	static int support_protected = 1;
	unsigned int priv_heap_flag = 0;
	unsigned char *cpu_ptr = NULL;
	uint64_t usage;
	uint32_t i, max_buffer_index = 0;
	unsigned int ion_flags = 0;
	int min_pgsz = 0;
	int fds[5] = {-1, -1, -1, -1, -1};

	ion_device *dev = ion_device::get();
	if (!dev)
	{
		return -1;
	}

	for (i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);
		usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

		ion_flags = 0;
		set_ion_flags(usage, &ion_flags);

		/* AFBC header must be initialized, but if the buffer is not zeroed, it can't
		 * be mapped for initialization. So force disable NOZEROED flag if AFBC
		 */
		if (bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
		{
			ion_flags &= ~ION_FLAG_NOZEROED;
		}

		if (usage & GRALLOC_USAGE_HFR_MODE)
		{
			priv_heap_flag |= private_handle_t::PRIV_FLAGS_USES_HFR_MODE;

			if (0 > allocate_to_container(bufDescriptor, ion_flags, &min_pgsz, fds))
			{
				MALI_GRALLOC_LOGE("allocate to container failed");
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);
				return -1;
			}
		}
		else
		{
			for (int fidx = 0; fidx < bufDescriptor->fd_count; fidx++)
			{
				fds[fidx] = dev->alloc_from_ion_heap(usage, bufDescriptor->alloc_sizes[fidx], ion_flags, &min_pgsz);

				if (fds[fidx] < 0)
				{
					MALI_GRALLOC_LOGE("ion_alloc failed from client ( %d )", dev->client());

					for (int cidx = 0; cidx < fidx; cidx++)
					{
						close(fds[cidx]);
					}

					/* need to free already allocated memory. not just this one */
					mali_gralloc_ion_free_internal(pHandle, numDescriptors);

					return -1;
				}
			}
		}

		private_handle_t *hnd = new private_handle_t(
		    priv_heap_flag,
		    bufDescriptor->alloc_sizes,
		    bufDescriptor->consumer_usage, bufDescriptor->producer_usage,
		    fds, bufDescriptor->fd_count,
		    bufDescriptor->hal_format, bufDescriptor->alloc_format,
		    bufDescriptor->width, bufDescriptor->height, bufDescriptor->pixel_stride,
		    bufDescriptor->layer_count, bufDescriptor->plane_info);

		if (NULL == hnd)
		{
			MALI_GRALLOC_LOGE("Private handle could not be created for descriptor:%d in non-shared usecase", i);

			/* Close the obtained shared file descriptor for the current handle */
			for (int j = 0; j < bufDescriptor->fd_count; j++)
			{
				close(fds[j]);
			}

			mali_gralloc_ion_free_internal(pHandle, numDescriptors);
			return -1;
		}

		pHandle[i] = hnd;
	}

#if defined(GRALLOC_INIT_AFBC) && (GRALLOC_INIT_AFBC == 1)
	for (i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);
		private_handle_t *hnd = (private_handle_t *)(pHandle[i]);

		usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

		if ((bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
			&& !(usage & GRALLOC_USAGE_PROTECTED))
		{
			/* TODO: only map for AFBC buffers */
			cpu_ptr =
			    (unsigned char *)mmap(NULL, bufDescriptor->alloc_sizes[0], PROT_READ | PROT_WRITE, MAP_SHARED, hnd->fds[0], 0);

			if (MAP_FAILED == cpu_ptr)
			{
				MALI_GRALLOC_LOGE("mmap failed from client ( %d ), fd ( %d )", dev->client(), hnd->fds[0]);
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);
				return -1;
			}

			mali_gralloc_ion_sync_start(hnd, true, true);

			/* For separated plane YUV, there is a header to initialise per plane. */
			const plane_info_t *plane_info = bufDescriptor->plane_info;
			const bool is_multi_plane = hnd->is_multi_plane();
			for (int i = 0; i < MAX_PLANES && (i == 0 || plane_info[i].byte_stride != 0); i++)
			{
				init_afbc(cpu_ptr + plane_info[i].offset,
				          bufDescriptor->alloc_format,
				          is_multi_plane,
				          plane_info[i].alloc_width,
				          plane_info[i].alloc_height);
			}

			mali_gralloc_ion_sync_end(hnd, true, true);

			munmap(cpu_ptr, bufDescriptor->alloc_sizes[0]);
		}
	}
#endif

	return 0;
}


int mali_gralloc_ion_map(private_handle_t *hnd)
{
	uint64_t usage = hnd->producer_usage | hnd->consumer_usage;

	/* Do not allow cpu access to secure buffers */
	if (usage & (GRALLOC_USAGE_PROTECTED | GRALLOC_USAGE_NOZEROED | GRALLOC_USAGE_SECURE_CAMERA_RESERVED)
			&& !(usage & GRALLOC_USAGE_PRIVATE_NONSECURE))
	{
		return 0;
	}

	/* Do not allow cpu access to HFR buffers */
	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_HFR_MODE)
	{
		return 0;
	}

#if defined(GRALLOC_SCALER_WFD) && GRALLOC_SCALER_WFD == 1
	// WFD for nacho cannot be mapped
	{
		if (!(usage & GRALLOC_USAGE_PROTECTED) &&
			usage & GRALLOC_USAGE_PRIVATE_NONSECURE &&
			usage & GRALLOC_USAGE_HW_COMPOSER)
		{
			return 0;
		}
	}
#endif

	for (int fidx = 0; fidx < hnd->fd_count; fidx++) {
		unsigned char *mappedAddress =
			(unsigned char *)mmap(NULL, hnd->alloc_sizes[fidx], PROT_READ | PROT_WRITE,
					MAP_SHARED, hnd->fds[fidx], 0);

		if (MAP_FAILED == mappedAddress)
		{
			int err = errno;
			MALI_GRALLOC_LOGE("mmap( fds[%d]:%d size:%" PRIu64 " ) failed with %s",
					fidx, hnd->fds[fidx], hnd->alloc_sizes[fidx], strerror(err));
			hnd->dump("map fail");

			for (int cidx = 0; cidx < fidx; cidx++)
			{
				munmap((void*)hnd->bases[cidx], hnd->alloc_sizes[cidx]);
			}

			return -err;
		}

		hnd->bases[fidx] = uintptr_t(mappedAddress);
	}

	return 0;
}

int import_exynos_ion_handles(private_handle_t *hnd)
{
	int retval = -1;
	ion_device *dev = ion_device::get();
	int attr_fd = hnd->get_share_attr_fd();

	for (int idx = 0; idx < hnd->fd_count; idx++)
	{
		if (hnd->fds[idx] >= 0)
		{
			/* TODO: check ion_handles[idx] is 0 */
			retval = exynos_ion_import_handle(dev->client(), hnd->fds[idx], &hnd->ion_handles[idx]);
			if (retval)
			{
				MALI_GRALLOC_LOGE("error importing ion_handle. ion_client(%d), ion_handle[%d](%d) format(%#" PRIx64 ")",
				     dev->client(), idx, hnd->ion_handles[idx], hnd->alloc_format);
				hnd->dump(__func__);
				goto error;
			}
		}
	}

	if (attr_fd >= 0)
	{
		/* TODO: check attr_ion_handle is 0 */
		retval = exynos_ion_import_handle(dev->client(), attr_fd, &hnd->attr_ion_handle);
		if (retval)
		{
			MALI_GRALLOC_LOGE("error importing attr_ion_handle. ion_client(%d), attr_ion_handle(%d)",
			     dev->client(), hnd->attr_ion_handle);
			hnd->dump(__func__);
			goto error;
		}
	}

	return retval;

error:
	free_exynos_ion_handles(hnd);

	return retval;
}

void free_exynos_ion_handles(private_handle_t *hnd)
{
	ion_device *dev = ion_device::get();

	for (int idx = 0; idx < hnd->fd_count; idx++)
	{
		if (hnd->ion_handles[idx])
		{
			if (exynos_ion_free_handle(dev->client(), hnd->ion_handles[idx]))
			{
				MALI_GRALLOC_LOGE("error freeing ion_handle. ion_client(%d), ion_handle[%d](%d) format(%#" PRIx64 ")",
				     dev->client(), idx, hnd->ion_handles[idx], hnd->alloc_format);
				hnd->dump(__func__);
			}
			else
			{
				hnd->ion_handles[idx] = 0;
			}
		}
	}

	if (hnd->attr_ion_handle)
	{
		if (exynos_ion_free_handle(dev->client(), hnd->attr_ion_handle))
		{
			MALI_GRALLOC_LOGE("error freeing attr_ion_handle. ion_client(%d), attr_ion_handle(%d)",
			     dev->client(), hnd->attr_ion_handle);
			hnd->dump(__func__);
		}
		else
		{
			hnd->attr_ion_handle = 0;
		}
	}
}


void mali_gralloc_ion_unmap(private_handle_t *hnd)
{
	for (int i = 0; i < hnd->fd_count; i++)
	{
		int err = 0;
		plane_info_t *plane = &hnd->plane_info[i];

		if (hnd->bases[i])
		{
			err = munmap((void*)hnd->bases[i], hnd->alloc_sizes[i]);
		}

		if (err)
		{
			MALI_GRALLOC_LOGE("Could not munmap base:%p size:%" PRIu64 " '%s'",
					(void*)hnd->bases[i], hnd->alloc_sizes[i], strerror(errno));
		}
		else
		{
			hnd->bases[i] = 0;
		}
	}

	hnd->cpu_read = 0;
	hnd->cpu_write = 0;
}

void mali_gralloc_ion_close(void)
{
	ion_device::close();
}

