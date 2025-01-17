
/*
 * Copyright (C) 2016 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
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
#ifndef MALI_GRALLOC_BUFFERACCESS_H_
#define MALI_GRALLOC_BUFFERACCESS_H_

#include "gralloc_helper.h"

int mali_gralloc_lock(const mali_gralloc_module* m, buffer_handle_t buffer, uint64_t usage, int l, int t, int w, int h, void** vaddr);
int mali_gralloc_unlock(const mali_gralloc_module* m, buffer_handle_t buffer);

int mali_gralloc_get_num_flex_planes(const mali_gralloc_module* m, buffer_handle_t buffer, uint32_t* num_planes);
int mali_gralloc_lock_async(const mali_gralloc_module* m, buffer_handle_t buffer, uint64_t usage, int l, int t, int w, int h, void** vaddr, int32_t fence_fd);
int mali_gralloc_lock_flex_async(const mali_gralloc_module* m, buffer_handle_t buffer, uint64_t usage, int l, int t, int w, int h, struct android_flex_layout* flex_layout, int32_t fence_fd);
int mali_gralloc_unlock_async(const mali_gralloc_module* m, buffer_handle_t buffer, int32_t* fence_fd);

static inline void mapForPrivateData(mali_gralloc_module const* module, private_handle_t *hnd)
{
	void *privAddress;

	if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA))
	{
		privAddress = mmap(0, hnd->size2, PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fd2, 0);
		if (privAddress == MAP_FAILED)
		{
			AERR("%s(%d): could not mmap %s for 3PRIVATE_DATA", __func__, __LINE__, strerror(errno));
		}
		else
		{
			hnd->base2 = (uint64_t)privAddress;
			exynos_ion_sync_fd(getIonFd(module), hnd->fd2);
		}
	}
	else if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA))
	{
		privAddress = mmap(0, hnd->size1, PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fd1, 0);
		if (privAddress == MAP_FAILED)
		{
			AERR("%s(%d): could not mmap %s for 2PRIVATE_DATA", __func__, __LINE__, strerror(errno));
		}
		else
		{
			hnd->base1 = (uint64_t)privAddress;
			exynos_ion_sync_fd(getIonFd(module), hnd->fd1);
		}
	}
}

static inline void unmapForPrivateData(private_handle_t *hnd)
{
	if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA))
	{
		if (munmap(INT_TO_PTR(hnd->base2), hnd->size2) < 0)
		{
			AERR("%s(%d) :could not unmap %s %d %#" PRIx64 "for 3PRIVATE_DATA", __func__, __LINE__, strerror(errno), static_cast<int>(PRIV_SIZE), hnd->base2);
		}
		else
		{
			hnd->base2 = 0;
		}
	}
	else if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA))
	{
		if (munmap(INT_TO_PTR(hnd->base1), hnd->size1) < 0)
		{
			AERR("%s(%d) :could not unmap %s %d %#" PRIx64 "for 2PRIVATE_DATA",  __func__, __LINE__, strerror(errno), static_cast<int>(PRIV_SIZE), hnd->base1);
		}
		else
		{
			hnd->base1 = 0;
		}
	}
}

static inline int mapForFds(mali_gralloc_module const* module, private_handle_t *hnd, int fd_num)
{
	void *mappedAddress = MAP_FAILED;
	switch(fd_num)
	{
		case 0:
			mappedAddress = (void*)mmap(0, hnd->size, PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fd, 0);
			break;
		case 1:
			mappedAddress = (void*)mmap(0, hnd->size1, PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fd1, 0);
			break;
		case 2:
			mappedAddress = (void*)mmap(0, hnd->size2, PROT_READ|PROT_WRITE, MAP_SHARED, hnd->fd2, 0);
			break;
	}

	if (mappedAddress == MAP_FAILED)
	{
		AERR("%s(%d): could not mmap %s for fd%d", __func__, __LINE__, strerror(errno), fd_num);
		return -errno;
	}

	switch(fd_num)
	{
		case 0:
			{
				hnd->base = (uint64_t)mappedAddress;
				exynos_ion_sync_fd(getIonFd(module), hnd->fd);
			}
			break;
		case 1:
			{
				hnd->base1 = (uint64_t)mappedAddress;
				exynos_ion_sync_fd(getIonFd(module), hnd->fd1);
			}
			break;
		case 2:
			{
				hnd->base2 = (uint64_t)mappedAddress;
				exynos_ion_sync_fd(getIonFd(module), hnd->fd2);
			}
			break;
	}

	return 0;
}

static inline void lockForRangeFlush(private_handle_t* hnd, uint64_t usage, int t, int h)
{
#ifdef GRALLOC_RANGE_FLUSH
	if(usage & GRALLOC1_USAGE_SW_WRITE_MASK)
	{
		hnd->lock_usage = GRALLOC1_PRODUCER_USAGE_CPU_WRITE;
		hnd->lock_offset = t * hnd->stride;
		hnd->lock_len = h * hnd->stride;
	}
#else
	GRALLOC_UNUSED(hnd);
	GRALLOC_UNUSED(usage);
	GRALLOC_UNUSED(t);
	GRALLOC_UNUSED(h);
#endif
}

static inline void doCacheFlushAndInvalidate(mali_gralloc_module const* module, private_handle_t* hnd)
{
#ifdef GRALLOC_RANGE_FLUSH
	if(hnd->lock_usage & GRALLOC1_USAGE_SW_WRITE_MASK)
	{
		if(((hnd->format == HAL_PIXEL_FORMAT_RGBA_8888) || (hnd->format == HAL_PIXEL_FORMAT_RGBX_8888)) && (hnd->lock_offset != 0))
			exynos_ion_sync_fd_partial(getIonFd(module), hnd->fd, hnd->lock_offset * 4, hnd->lock_len * 4);
		else
			exynos_ion_sync_fd(getIonFd(module), hnd->fd);

		if (hnd->fd1 >= 0)
			exynos_ion_sync_fd(getIonFd(module), hnd->fd1);
		if (hnd->fd2 >= 0)
			exynos_ion_sync_fd(getIonFd(module), hnd->fd2);

		hnd->lock_usage = 0;
	}
#else
	exynos_ion_sync_fd(getIonFd(module), hnd->fd);
	{
		if (hnd->fd1 >= 0)
			exynos_ion_sync_fd(getIonFd(module), hnd->fd1);
		if (hnd->fd2 >= 0)
			exynos_ion_sync_fd(getIonFd(module), hnd->fd2);
	}
#endif
}
#endif /* MALI_GRALLOC_BUFFERACCESS_H_ */
