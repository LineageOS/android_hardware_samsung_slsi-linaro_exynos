/*
 * Copyright (C) 2016-2019 ARM Limited. All rights reserved.
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

#include <sys/ioctl.h>

#include <hardware/hardware.h>

#if GRALLOC_VERSION_MAJOR == 1
#include <hardware/gralloc1.h>
#elif GRALLOC_VERSION_MAJOR == 0
#include <hardware/gralloc.h>
#endif

#include "mali_gralloc_module.h"
#include "mali_gralloc_private_interface_types.h"
#include "mali_gralloc_buffer.h"
#include "gralloc_helper.h"
#include "framebuffer_device.h"
#include "mali_gralloc_formats.h"
#include "mali_gralloc_usages.h"
#include "mali_gralloc_bufferdescriptor.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_ion.h"

#include <hardware/exynos/ion.h>
#include <hardware/exynos/dmabuf_container.h>

#define INIT_ZERO(obj) (memset(&(obj), 0, sizeof((obj))))

#define HEAP_MASK_FROM_ID(id) (1 << id)
#define HEAP_MASK_FROM_TYPE(type) (1 << type)

static const enum mem_type TYPE_INVALID = ((enum mem_type)~0);
static const enum mem_type TYPE_SECURE = (enum mem_type)(((unsigned int)TYPE_CUSTOM) + 1);

#if defined(ION_HEAP_SECURE_MASK)
#if (HEAP_MASK_FROM_TYPE(ION_HEAP_TYPE_SECURE) != ION_HEAP_SECURE_MASK)
#error "ION_HEAP_TYPE_SECURE value is not compatible with ION_HEAP_SECURE_MASK"
#endif
#endif

static int ion_client = -1;
static bool secure_heap_exists = true;

static void set_ion_flags(enum mem_type heap_type, uint64_t usage,
                          unsigned int *priv_heap_flag, unsigned int *ion_flags)
{
#if !GRALLOC_USE_ION_DMA_HEAP
	GRALLOC_UNUSED(heap_type);
#endif

	if (usage & GRALLOC1_PRODUCER_USAGE_HFR_MODE)
	{
		*priv_heap_flag = private_handle_t::PRIV_FLAGS_USES_HFR_MODE;
	}

	if (priv_heap_flag)
	{
#if GRALLOC_USE_ION_DMA_HEAP
		if (heap_type == TYPE_DMA)
		{
			*priv_heap_flag = private_handle_t::PRIV_FLAGS_USES_ION_DMA_HEAP;
		}
#endif
	}

	if (ion_flags)
	{
#if GRALLOC_USE_ION_DMA_HEAP
		if (heap_type != TYPE_DMA)
		{
#endif
			if ((usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_OFTEN)
			{
				*ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
			}
#if GRALLOC_USE_ION_DMA_HEAP
		}
#endif
		/* LSI Integration */
		if ((usage & GRALLOC1_USAGE_SW_READ_MASK) == GRALLOC1_USAGE_READ_OFTEN)
		{
			*ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

			if (usage & GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET)
			{
				*ion_flags |= ION_FLAG_SYNC_FORCE;
			}
		}

		if (usage & GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET || usage & GRALLOC_USAGE_HW_TEXTURE)
		{
			*ion_flags |= ION_FLAG_MAY_HWRENDER;
		}

		if (usage & GRALLOC1_PRODUCER_USAGE_NOZEROED)
		{
			*ion_flags |= ION_FLAG_NOZEROED;
		}

#ifdef GRALLOC_PROTECTED_ION_FLAG_FOR_CAMERA_RESERVED
		if (usage & GRALLOC1_PRODUCER_USAGE_CAMERA_RESERVED)
		{
			*ion_flags |= ION_FLAG_PROTECTED;
		}
#endif
		// DRM or Secure Camera
		if ((usage & GRALLOC1_PRODUCER_USAGE_PROTECTED) || usage & GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED)
		{
			/* W/A to Android R Camera vts_r5. (W/A requested by Visual S/W group MCD) */
			if(!(usage & GRALLOC1_PRODUCER_USAGE_CAMERA_RESERVED))
			{
				*ion_flags |= ION_FLAG_PROTECTED;
			}
		}
#if GRALLOC_SCALER_WFD == 1
		if (usage & GRALLOC1_PRODUCER_USAGE_PRIVATE_NONSECURE && usage & GRALLOC1_CONSUMER_USAGE_HWCOMPOSER)
		{
			*ion_flags |= ION_FLAG_PROTECTED;
		}
#endif
	}
}


static unsigned int select_heap_mask(uint64_t usage)
{
	unsigned int heap_mask;
	if (usage & GRALLOC1_PRODUCER_USAGE_PROTECTED)
	{
		if (usage & GRALLOC1_PRODUCER_USAGE_PRIVATE_NONSECURE)
		{
			heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
		}
		else if (usage & GRALLOC1_PRODUCER_USAGE_CAMERA_RESERVED)
		{
			/* W/A to Android R Camera vts_r5. (W/A requested by Visual S/W group MCD) */
			heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
		}
		else if (usage & GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED)
		{
			heap_mask = EXYNOS_ION_HEAP_SECURE_CAMERA_MASK;
		}
		else if (usage & GRALLOC1_CONSUMER_USAGE_VIDEO_EXT)
		{
			heap_mask = EXYNOS_ION_HEAP_VIDEO_STREAM_MASK;
		}
		else if ((usage & GRALLOC1_CONSUMER_USAGE_HWCOMPOSER) &&
			!(usage & GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE) &&
			!(usage & GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET))
		{
			heap_mask = EXYNOS_ION_HEAP_VIDEO_SCALER_MASK;
		}
		else
		{
			heap_mask = EXYNOS_ION_HEAP_VIDEO_FRAME_MASK;
		}
	}
#if GRALLOC_SCALER_WFD == 1
	else if (usage & GRALLOC1_PRODUCER_USAGE_PRIVATE_NONSECURE && usage & GRALLOC1_CONSUMER_USAGE_HWCOMPOSER)
	{
		heap_mask = EXYNOS_ION_HEAP_EXT_UI_MASK;
	}
#endif
	else if (usage & GRALLOC1_PRODUCER_USAGE_CAMERA_RESERVED)
	{
		heap_mask = EXYNOS_ION_HEAP_CAMERA_MASK;
	}
	else if (usage & GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED)
	{
		heap_mask = EXYNOS_ION_HEAP_SECURE_CAMERA_MASK;
	}
	else
	{
		heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
	}

	return heap_mask;
}


/*
 * Returns positive number if not mapping due to AFBC or protected memory
 * Returns negative errno if mapping failed to mmap
 */
static int gralloc_map(buffer_handle_t handle)
{
	private_handle_t *hnd = (private_handle_t*)handle;
	int ret = 0;
	hnd->bases[0] = hnd->bases[1] = hnd->bases[2] = 0;

	/*
	 * VideoMetaData must be mapped for CPU access even if the buffer is a secure buffer
	 */
	if (hnd->flags & (private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA | private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA))
	{
		int idx = hnd->flags & private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA ? 2 : 1;
		void *cpuPtr = mmap(0, hnd->sizes[idx], PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fds[idx], 0);

		if (cpuPtr == MAP_FAILED)
		{
			ret = -errno;
			AERR("could not mmap %s for PRIVATE_DATA at fd%d", strerror(errno), idx);
			goto err;
		}

		hnd->bases[idx] = (uint64_t)cpuPtr;
	}

	// AFBC must not be mapped.
	if (hnd->is_compressible || hnd->flags & private_handle_t::PRIV_FLAGS_USES_HFR_MODE)
	{
		return 1;
	}

	// Don't be mapped for Secure DRM & Secure Camera
	if ((hnd->producer_usage & GRALLOC1_PRODUCER_USAGE_PROTECTED && !(hnd->consumer_usage & GRALLOC1_PRODUCER_USAGE_PRIVATE_NONSECURE))
		|| hnd->producer_usage & GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED)
	{
		return 2;
	}

	// HFR mode cannot be mapped
	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_HFR_MODE)
	{
		return 3;
	}

#if GRALLOC_SCALER_WFD == 1
	// WFD for nacho cannot be mapped
	{
		uint64_t usage = hnd->producer_usage | hnd->consumer_usage;
		if (!(usage & GRALLOC1_PRODUCER_USAGE_PROTECTED) &&
			usage & GRALLOC1_PRODUCER_USAGE_PRIVATE_NONSECURE &&
			usage & GRALLOC1_CONSUMER_USAGE_HWCOMPOSER)
		{
			return 100;
		}
	}
#endif

	if (!(hnd->producer_usage &
		(GRALLOC1_PRODUCER_USAGE_PROTECTED | GRALLOC1_PRODUCER_USAGE_NOZEROED |GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED)))
	{
		for (int idx = 0; idx < hnd->get_num_ion_fds(); idx++)
		{
			if (hnd->fds[idx] >= 0 && !hnd->bases[idx])
			{
				void *cpuPtr = (void*)mmap(0, hnd->sizes[idx], PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fds[idx], 0);

				if (cpuPtr == MAP_FAILED)
				{
					ret = -errno;
					AERR("could not mmap %s for fd%d", strerror(errno), idx);
					goto err;
				}

				if (idx == 0)
					hnd->bases[idx] = (uint64_t)cpuPtr + hnd->offset;
				else
					hnd->bases[idx] = (uint64_t)cpuPtr;
			}
		}
	}

	return 0;

err:
	for (int idx = 0; idx < hnd->get_num_ion_fds(); idx++)
	{
		if (hnd->bases[idx] != 0 && munmap((void *)hnd->bases[idx], hnd->sizes[idx]) != 0)
		{
			AERR("Failed to munmap handle %p fd%d", hnd, idx);
		}
		else
		{
			hnd->bases[idx] = 0;
		}
	}

	return ret;

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
static int alloc_from_ion_heap(uint64_t usage, size_t size,
                               enum mem_type heap_type, unsigned int flags,
                               int *min_pgsz)
{
	int shared_fd = -1;

	if (ion_client < 0 ||
	    size <= 0 ||
	    heap_type == TYPE_INVALID ||
	    min_pgsz == NULL)
	{
		return -1;
	}

	unsigned int heap_mask = select_heap_mask(usage);
	shared_fd = exynos_ion_alloc(ion_client, size, heap_mask, flags);

	/* Check if allocation from selected heap failed and fall back to system
	 * heap if possible.
	 */
	if (shared_fd < 0)
	{
		/* Don't allow falling back to sytem heap if secure was requested. */
		if (heap_type == TYPE_SECURE)
		{
			return -1;
		}

		/* Can't fall back to system heap if system heap was the heap that
		 * already failed
		 */
		if (heap_type == TYPE_SYSTEM)
		{
			AERR("%s: Allocation failed on on system heap. Cannot fallback.", __func__);
			return -1;
		}

		heap_type = TYPE_SYSTEM;

		/* Set ION flags for system heap allocation */
		set_ion_flags(heap_type, usage, NULL, &flags);

		shared_fd = exynos_ion_alloc(ion_client, size, EXYNOS_ION_HEAP_SYSTEM_MASK, flags);

		if (shared_fd < 0)
		{
			AERR("Fallback ion_alloc_fd(%d, %zd, %d, %u, %p) failed",
			      ion_client, size, 0, flags, &shared_fd);
			return -1;
		}
		else
		{
			AWAR("allocating to system heap as fallback: fd(%d) usage(%" PRIx64 ") size(%zu) heap_type(%d) heap_mask(0x%x) flags(%u)",
				shared_fd, usage, size, heap_type, heap_mask, flags);
		}
	}

	switch (heap_type)
	{
	case TYPE_SYSTEM:
		*min_pgsz = SZ_4K;
		break;

	case TYPE_SYSTEM_CONTIG:
	case TYPE_CARVEOUT:
#if GRALLOC_USE_ION_DMA_HEAP
	case TYPE_DMA:
		*min_pgsz = size;
		break;
#endif
#if GRALLOC_USE_ION_COMPOUND_PAGE_HEAP
	case TYPE_COMPOUND_PAGE:
		*min_pgsz = SZ_2M;
		break;
#endif
	/* If have customized heap please set the suitable pg type according to
	 * the customized ION implementation
	 */
	case TYPE_CUSTOM:
		*min_pgsz = SZ_4K;
		break;
	default:
		*min_pgsz = SZ_4K;
		break;
	}

	return shared_fd;
}

int alloc_metadata()
{
	int min_pgsz = 0;
	return alloc_from_ion_heap(GRALLOC1_USAGE_READ_OFTEN | GRALLOC1_USAGE_WRITE_OFTEN, PAGE_SIZE, TYPE_SYSTEM, ION_FLAG_CACHED, &min_pgsz);
}

static enum mem_type pick_ion_heap(uint64_t usage)
{
	enum mem_type heap_type = TYPE_INVALID;

	if (usage & GRALLOC_USAGE_PROTECTED)
	{
		if (secure_heap_exists)
		{
			heap_type = TYPE_SECURE;
		}
		else
		{
			AERR("Protected ION memory is not supported on this platform.");
		}
	}
	else if (!(usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) && (usage & (GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_COMPOSER)))
	{
#if GRALLOC_USE_ION_COMPOUND_PAGE_HEAP
		heap_type = TYPE_COMPOUND_PAGE;
#elif GRALLOC_USE_ION_DMA_HEAP
		heap_type = TYPE_DMA;
#else
		heap_type = TYPE_SYSTEM;
#endif
	}
	else
	{
		heap_type = TYPE_SYSTEM;
	}

	return heap_type;
}


static bool check_buffers_sharable(const gralloc_buffer_descriptor_t *descriptors,
                                   uint32_t numDescriptors)
{
	enum mem_type shared_backend_heap_type = TYPE_INVALID;
	unsigned int shared_ion_flags = 0;
	uint64_t usage;
	uint32_t i;

	if (numDescriptors <= 1)
	{
		return false;
	}

	for (i = 0; i < numDescriptors; i++)
	{
		unsigned int ion_flags;
		enum mem_type heap_type;

		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)descriptors[i];

		usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

		heap_type = pick_ion_heap(usage);
		if (heap_type == TYPE_INVALID)
		{
			return false;
		}

		set_ion_flags(heap_type, usage, NULL, &ion_flags);

		if (shared_backend_heap_type != TYPE_INVALID)
		{
			if (shared_backend_heap_type != heap_type || shared_ion_flags != ion_flags)
			{
				return false;
			}
		}
		else
		{
			shared_backend_heap_type = heap_type;
			shared_ion_flags = ion_flags;
		}
	}

	return true;
}

static int get_max_buffer_descriptor_index(const gralloc_buffer_descriptor_t *descriptors, uint32_t numDescriptors)
{
	uint32_t i, max_buffer_index = 0;
	size_t max_buffer_size = 0;

	for (i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)descriptors[i];

		if (max_buffer_size < bufDescriptor->size)
		{
			max_buffer_index = i;
			max_buffer_size = bufDescriptor->size;
		}
	}

	return max_buffer_index;
}

/*
 * Opens the ION module. Queries heap information and stores it for later use
 *
 * @return              ionfd in case of success
 *                      -1 for all error cases
 */
static int open_and_query_ion(void)
{
	int ret = -1;

	if (ion_client >= 0)
	{
		AWAR("ION device already open");
		return 0;
	}

	ion_client = exynos_ion_open();
	if (ion_client < 0)
	{
		AERR("ion_open failed with %s", strerror(errno));
		return -1;
	}

#if defined(ION_HEAP_SECURE_MASK)
	secure_heap_exists = true;
#endif

	return ion_client;
}

int mali_gralloc_ion_open(void)
{
	return open_and_query_ion();
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

#ifdef GRALLOC_ION_SYNC
	if (ion_client <= 0)
	{
		/* a second user process must obtain a client handle first via ion_open before it can obtain the shared ion buffer*/
		AWAR("ion_client not specified");

		int status = 0;
		status = open_and_query_ion();
		if (status < 0)
		{
			return status;
		}
	}

	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION &&
		!(hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION_DMA_HEAP))
	{
		int direction = 0;

		if (read)
		{
			direction |= ION_SYNC_READ;
		}
		if (write)
		{
			direction |= ION_SYNC_WRITE;
		}

		for (int idx = 0; idx < hnd->get_num_ion_fds(); idx++)
		{
			if (start)
			{
				ret |= exynos_ion_sync_start(ion_client, hnd->fds[idx], direction);
			}
			else
			{
				ret |= exynos_ion_sync_end(ion_client, hnd->fds[idx], direction);
			}
		}
	}
#else
	GRALLOC_UNUSED(read);
	GRALLOC_UNUSED(write);
	GRALLOC_UNUSED(start);
#endif

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
	if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
	{
		return;
	}
	else if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
		/* Buffer might be unregistered already so we need to assure we have a valid handle */

		for (int idx = 0; idx < hnd->get_num_ion_fds(); idx++)
		{
			if (hnd->fds[idx] >= 0)
			{
				if (hnd->bases[idx] != 0)
				{
					if (munmap((void *)hnd->bases[idx], hnd->sizes[idx]) != 0)
					{
						AERR("Failed to munmap handle %p fd%d", hnd, idx);
					}
				}

				close(hnd->fds[idx]);
			}
		}

		memset((void *)hnd, 0, sizeof(*hnd));
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

static int allocate_to_fds(buffer_descriptor_t *bufDescriptor, enum mem_type heap_type,
		uint32_t ion_flags, uint32_t *priv_heap_flag, int *min_pgsz, int* fd0, int* fd1, int* fd2)
{
	int fd_arr[3] = {-1, -1, -1};
	uint64_t usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

	for (int idx = 0; idx < bufDescriptor->fd_count; idx++)
	{
		fd_arr[idx] = alloc_from_ion_heap(usage, bufDescriptor->sizes[idx], heap_type, ion_flags, min_pgsz);

		if (fd_arr[idx] < 0)
		{
			AERR("ion_alloc failed from client ( %d )", ion_client);
			goto err;
		}
	}

	if (bufDescriptor->alloc_video_private_data)
	{
		int idx = bufDescriptor->fd_count;

		if (idx != 1 && idx != 2)
		{
			AERR("cannot allocate video private metadata for formate(%#x)", (uint32_t)bufDescriptor->internal_format);
			goto err;
		}

		fd_arr[idx] = alloc_from_ion_heap(usage, VIDEO_PRIV_DATA_SIZE, TYPE_SYSTEM, 0, min_pgsz);
		if (fd_arr[idx] < 0)
		{
			AERR("ion_alloc failed from client ( %d )", ion_client);
			goto err;
		}

		bufDescriptor->sizes[idx] = VIDEO_PRIV_DATA_SIZE;

		switch (idx)
		{
			case 2:
				*priv_heap_flag |= private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA;
				break;
			case 1:
				*priv_heap_flag |= private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA;
				break;
		}

		bufDescriptor->fd_count++;
	}

	*fd0 = fd_arr[0];
	*fd1 = fd_arr[1];
	*fd2 = fd_arr[2];

	return 0;

err:
	for (int i = 0; i < 3; i++)
	{
		if (fd_arr[i] >= 0)
		{
			close(fd_arr[i]);
		}
	}

	return -1;
}


static inline int createBufferContainer(int *fd, int size, int *containerFd)
{
	int bufferContainerFd = -1;

	if (fd == NULL || size < 1 || containerFd == NULL)
	{
		AERR("invalid parameters. fd(%p), size(%d), containerFd(%p)\n", fd, size, containerFd);
		return -EINVAL;
	}

	bufferContainerFd = dma_buf_merge(fd[0], fd + 1, size - 1);
	if (bufferContainerFd < 0)
	{
		AERR("fail to create Buffer Container. containerFd(%d), size(%d)", bufferContainerFd, size);
		return -EINVAL;
	}

	*containerFd = bufferContainerFd;

	return 0;
}

static int allocate_to_container(buffer_descriptor_t *bufDescriptor, enum mem_type heap_type,
		uint32_t ion_flags, uint32_t *priv_heap_flag, int *min_pgsz, int fd_arr[])
{
	const int layer_count = GRALLOC_HFR_BATCH_SIZE;
	int hfr_fds[3][layer_count];
	int err = 0;

	memset(hfr_fds, 0xff, sizeof(hfr_fds));

	for (int layer = 0; layer < layer_count; layer++)
	{
		err = allocate_to_fds(bufDescriptor, heap_type, ion_flags, priv_heap_flag, min_pgsz,
					&hfr_fds[0][layer], &hfr_fds[1][layer], &hfr_fds[2][layer]);
		if(err)
			goto finish;
	}

	for (int i = 0; i < bufDescriptor->fd_count; i++)
	{
		err = createBufferContainer(hfr_fds[i], layer_count, &fd_arr[i]);
		if (err)
		{
			for (int j = 0; j < i; j++)
				close(fd_arr[j]);

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
	static int support_protected = 1;
	unsigned int priv_heap_flag = 0;
	enum mem_type heap_type;
	unsigned char *cpu_ptr = NULL;
	uint64_t usage;
	uint32_t i, max_buffer_index = 0;
	union
	{
		int shared_fd;
		int fd_arr[3];
	};
	unsigned int ion_flags = 0;
	int min_pgsz = 0;
	int is_compressible = 0;

	fd_arr[0] = fd_arr[1] = fd_arr[2] = -1;

	if (ion_client < 0)
	{
		int status = 0;
		status = open_and_query_ion();
		if (status < 0)
		{
			return status;
		}
	}

	*shared_backend = check_buffers_sharable(descriptors, numDescriptors);

	if (*shared_backend)
	{
		buffer_descriptor_t *max_bufDescriptor;

		max_buffer_index = get_max_buffer_descriptor_index(descriptors, numDescriptors);
		max_bufDescriptor = (buffer_descriptor_t *)(descriptors[max_buffer_index]);
		usage = max_bufDescriptor->consumer_usage | max_bufDescriptor->producer_usage;

		heap_type = pick_ion_heap(usage);
		if (heap_type == TYPE_INVALID)
		{
			AERR("Failed to find an appropriate ion heap");
			return -1;
		}

		set_ion_flags(heap_type, usage, &priv_heap_flag, &ion_flags);

		if (allocate_to_fds(max_bufDescriptor, heap_type, ion_flags, &priv_heap_flag, &min_pgsz,
					&fd_arr[0], &fd_arr[1], &fd_arr[2]) < 0)
		{
			AERR("ion_alloc failed form client: ( %d )", ion_client);
			return -1;
		}

		for (i = 0; i < numDescriptors; i++)
		{
			buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);
			int tmp_fd;

			if (i != max_buffer_index)
			{
				tmp_fd = dup(shared_fd);

				if (tmp_fd < 0)
				{
					AERR("Ion shared fd:%d of index:%d could not be duplicated for descriptor:%d",
					      shared_fd, max_buffer_index, i);

					/* It is possible that already opened shared_fd for the
					 * max_bufDescriptor is also not closed */
					if (i < max_buffer_index)
					{
						close(shared_fd);
					}

					/* Need to free already allocated memory. */
					mali_gralloc_ion_free_internal(pHandle, numDescriptors);
					return -1;
				}
			}
			else
			{
				tmp_fd = shared_fd;
			}

			if (bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_AFBC_BASIC)
			{
				is_compressible = 1;

				if (fd_arr[1] >= 0)
				{
					ALOGW("Warning afbc flag fd already exists during create. Closing.");
					close(fd_arr[1]);
				}

				bufDescriptor->sizes[1] = sizeof(int);
				fd_arr[1] = exynos_ion_alloc(ion_client, bufDescriptor->sizes[1], EXYNOS_ION_HEAP_SYSTEM_MASK, 0);
				if (fd_arr[1] < 0)
				{
					ALOGE("Failed to allocate page for afbc flag region");
					mali_gralloc_ion_free_internal(pHandle, numDescriptors);
					close(tmp_fd);
					return -1;
				}
			}

			private_handle_t *hnd = new private_handle_t(
			    private_handle_t::PRIV_FLAGS_USES_ION | priv_heap_flag, bufDescriptor->size, 0, 0,  min_pgsz,
			    bufDescriptor->consumer_usage, bufDescriptor->producer_usage, tmp_fd, -1, -1,  bufDescriptor->hal_format,
			    bufDescriptor->internal_format, bufDescriptor->alloc_format,
			    bufDescriptor->width, bufDescriptor->height,
			    max_bufDescriptor->size, bufDescriptor->pixel_stride, bufDescriptor->layer_count,
				bufDescriptor->plane_info, is_compressible, bufDescriptor->plane_count);

			if (NULL == hnd)
			{
				AERR("Private handle could not be created for descriptor:%d of shared usecase", i);

				/* Close the obtained shared file descriptor for the current handle */
				close(tmp_fd);

				/* It is possible that already opened shared_fd for the
				 * max_bufDescriptor is also not closed */
				if (i < max_buffer_index)
				{
					close(shared_fd);
				}

				/* Free the resources allocated for the previous handles */
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);
				return -1;
			}

			pHandle[i] = hnd;
		}
	}
	else
	{
		for (i = 0; i < numDescriptors; i++)
		{
			buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);
			usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

			heap_type = pick_ion_heap(usage);
			if (heap_type == TYPE_INVALID)
			{
				AERR("Failed to find an appropriate ion heap");
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);
				return -1;
			}

			set_ion_flags(heap_type, usage, &priv_heap_flag, &ion_flags);

#if GRALLOC_INIT_AFBC == 1
			/* If initializing AFBC header using the CPU, the buffer must be zeroed
			 * in order to allow cpu mapping. Otherwise, mapping would be block for
			 * security reasons
			 * */
			if (bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
			{
				ion_flags &= ~ION_FLAG_NOZEROED;
			}
#endif

			if (~usage & GRALLOC1_PRODUCER_USAGE_HFR_MODE)
			{
				if (allocate_to_fds(bufDescriptor, heap_type, ion_flags, &priv_heap_flag, &min_pgsz,
							&fd_arr[0], &fd_arr[1], &fd_arr[2]) < 0)
				{
					AERR("ion_alloc failed from client ( %d )", ion_client);

					/* need to free already allocated memory. not just this one */
					mali_gralloc_ion_free_internal(pHandle, numDescriptors);
					return -1;
				}
			}
			else
			{
				if (0 > allocate_to_container(bufDescriptor, heap_type, ion_flags, &priv_heap_flag, &min_pgsz, fd_arr))
				{
					AERR("allocate to container failed from client ( %d )", ion_client);
					mali_gralloc_ion_free_internal(pHandle, numDescriptors);
					return -1;
				}
			}

			if (bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_AFBC_BASIC)
			{
				is_compressible = 1;

				if (fd_arr[1] >= 0)
				{
					ALOGW("Warning afbc flag fd already exists during create. Closing.");
					close(fd_arr[1]);
				}

				bufDescriptor->sizes[1] = sizeof(int);
				fd_arr[1] = exynos_ion_alloc(ion_client, bufDescriptor->sizes[1], EXYNOS_ION_HEAP_SYSTEM_MASK, 0);
				if (fd_arr[1] < 0)
				{
					ALOGE("Failed to allocate page for afbc flag region");
					mali_gralloc_ion_free_internal(pHandle, numDescriptors);
					return -1;
				}
			}

			private_handle_t *hnd = new private_handle_t(
			    private_handle_t::PRIV_FLAGS_USES_ION | priv_heap_flag, bufDescriptor->size,
				bufDescriptor->sizes[1], bufDescriptor->sizes[2], min_pgsz,
			    bufDescriptor->consumer_usage, bufDescriptor->producer_usage,
				shared_fd, fd_arr[1], fd_arr[2], bufDescriptor->hal_format,
			    bufDescriptor->internal_format, bufDescriptor->alloc_format,
			    bufDescriptor->width, bufDescriptor->height,
			    bufDescriptor->size, bufDescriptor->pixel_stride, bufDescriptor->layer_count,
				bufDescriptor->plane_info, is_compressible, bufDescriptor->plane_count);

			if (NULL == hnd)
			{
				AERR("Private handle could not be created for descriptor:%d in non-shared usecase", i);

				/* Close the obtained shared file descriptor for the current handle */
				close(shared_fd);
				mali_gralloc_ion_free_internal(pHandle, numDescriptors);
				return -1;
			}

			pHandle[i] = hnd;
		}
	}

	for (i = 0; i < numDescriptors; i++)
	{
		buffer_descriptor_t *bufDescriptor = (buffer_descriptor_t *)(descriptors[i]);
		private_handle_t *hnd = (private_handle_t *)(pHandle[i]);

		usage = bufDescriptor->consumer_usage | bufDescriptor->producer_usage;

		if (!(usage & GRALLOC_USAGE_PROTECTED))
		{
#if GRALLOC_INIT_AFBC == 1
			if ((bufDescriptor->alloc_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK) && (!(*shared_backend)))
			{
				cpu_ptr =
					(unsigned char *)mmap(NULL, bufDescriptor->size, PROT_READ | PROT_WRITE, MAP_SHARED, hnd->share_fd, 0);

				if (MAP_FAILED == cpu_ptr)
				{
					AERR("mmap failed from client ( %d ), fd ( %d )", ion_client, hnd->share_fd);
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

				munmap((void *)cpu_ptr, bufDescriptor->size);
			}
#endif
		}
	}

	return 0;
}


int import_exynos_ion_handles(private_handle_t *hnd)
{
	int retval = -1;

#if GRALLOC_VERSION_MAJOR <= 1
		/* the test condition is set to ion_client <= 0 here, because
		 * a second user process should get a ion fd greater than 0.
		 */
		if (ion_client <= 0)
		{
			/* a second user process must obtain a client handle first via ion_open before it can obtain the shared ion buffer*/
			int status = 0;
			status = open_and_query_ion();
			if (status < 0)
			{
				return status;
			}
		}
#endif

	for (int idx = 0; idx < hnd->get_num_ion_fds(); idx++)
	{
		if (hnd->fds[idx] >= 0)
		{
			retval = exynos_ion_import_handle(ion_client, hnd->fds[idx], &hnd->ion_handles[idx]);
			if (retval)
			{
				AERR("error importing handle[%d] format(%x)\n", idx, (uint32_t)hnd->internal_format);
				goto clean_up;
			}
		}
	}

	return retval;

clean_up:
	for (int idx = 0; idx < hnd->get_num_ion_fds(); idx++)
	{
		if (hnd->ion_handles[idx])
		{
			exynos_ion_free_handle(ion_client, hnd->ion_handles[idx]);
		}
	}

	return retval;
}


void free_exynos_ion_handles(private_handle_t *hnd)
{
	for (int idx = 0; idx < hnd->get_num_ion_fds(); idx++)
	{
		if (hnd->ion_handles[idx])
		{
			if (ion_client > 0 && hnd->ion_handles[idx] > 0 &&
			    exynos_ion_free_handle(ion_client, hnd->ion_handles[idx]))
			{
				AERR("error freeing ion_client(%d), ion_handle[%d](%d) format(%x)\n",
				     ion_client, idx, hnd->ion_handles[idx], (uint32_t)hnd->internal_format);
			}
		}
	}
}


int mali_gralloc_ion_map(private_handle_t *hnd)
{
	int retval = -EINVAL;

	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
#if GRALLOC_VERSION_MAJOR <= 1
		/* the test condition is set to ion_client <= 0 here, because
		 * a second user process should get a ion fd greater than 0.
		 */
		if (ion_client <= 0)
		{
			/* a second user process must obtain a client handle first via ion_open before it can obtain the shared ion buffer*/
			int status = 0;
			status = open_and_query_ion();
			if (status < 0)
			{
				return status;
			}
		}
#endif
		retval = gralloc_map(hnd);
	}

	return retval;
}

void mali_gralloc_ion_unmap(private_handle_t *hnd)
{
	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_ION)
	{
		int ret = 0, fail = 0;

		for (int idx = 0; idx < hnd->get_num_ion_fds(); idx++)
		{
			if (hnd->bases[idx])
			{
				if (munmap((void *)hnd->bases[idx], hnd->sizes[idx]) < 0)
				{
					AERR("Could not munmap base:%p size:%u '%s'", (void *)hnd->bases[0], hnd->sizes[idx], strerror(errno));
					fail = 1;
				}
				else
				{
					hnd->bases[idx] = 0;
				}
			}
		}

		if (!fail)
		{
			hnd->cpu_read = 0;
			hnd->cpu_write = 0;
		}
	}
}

void mali_gralloc_ion_close(void)
{
	if (ion_client != -1)
	{
		if (exynos_ion_close(ion_client))
		{
			AERR("Failed to close ion_client: %d err=%s", ion_client, strerror(errno));
		}

		ion_client = -1;
	}
}

