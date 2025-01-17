/*
 * Copyright (C) 2016, 2018-2020 ARM Limited. All rights reserved.
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

#include <hardware/gralloc1.h>

#include "mali_gralloc_buffer.h"
#include "allocator/mali_gralloc_ion.h"
#include "allocator/mali_gralloc_shared_memory.h"
#include "mali_gralloc_bufferallocation.h"
#include "mali_gralloc_debug.h"

static pthread_mutex_t s_map_lock = PTHREAD_MUTEX_INITIALIZER;

int mali_gralloc_reference_retain(buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
	{
		MALI_GRALLOC_LOGE("Registering/Retaining invalid buffer %p, returning error", handle);
		return -EINVAL;
	}

	private_handle_t *hnd = (private_handle_t *)handle;
	pthread_mutex_lock(&s_map_lock);

	if (hnd->allocating_pid == getpid() || hnd->remote_pid == getpid())
	{
		hnd->ref_count++;
		pthread_mutex_unlock(&s_map_lock);
		return 0;
	}
	else
	{
		hnd->remote_pid = getpid();
		hnd->ref_count = 1;
	}

	int retval= mali_gralloc_ion_map(hnd);

	/* Import ION handle to let ION driver know who's using the buffer */
	import_exynos_ion_handles(hnd);

	pthread_mutex_unlock(&s_map_lock);

	return retval;
}

int mali_gralloc_reference_release(buffer_handle_t handle, bool canFree)
{
	if (private_handle_t::validate(handle) < 0)
	{
		MALI_GRALLOC_LOGE("unregistering/releasing invalid buffer %p, returning error", handle);
		return -EINVAL;
	}

	private_handle_t *hnd = (private_handle_t *)handle;
	pthread_mutex_lock(&s_map_lock);

	if (hnd->ref_count == 0)
	{
		MALI_GRALLOC_LOGE("Buffer %p should have already been released", handle);
		pthread_mutex_unlock(&s_map_lock);
		return -EINVAL;
	}

	/* TODO: reference_release is never used by allocating pid.
	 * This if statement should be removed */
	if (hnd->allocating_pid == getpid())
	{
		hnd->ref_count--;

		if (hnd->ref_count == 0 && canFree)
		{
			mali_gralloc_buffer_free(handle);
		}
	}
	else if (hnd->remote_pid == getpid()) // never unmap buffers that were not imported into this process
	{
		hnd->ref_count--;

		if (hnd->ref_count == 0)
		{
			mali_gralloc_ion_unmap(hnd);
			free_exynos_ion_handles(hnd);

			/* TODO: Make this unmapping of shared meta fd into a function? */
			if (hnd->attr_base)
			{
				munmap(hnd->attr_base, hnd->attr_size);
				hnd->attr_base = nullptr;
			}
		}
	}
	else
	{
		MALI_GRALLOC_LOGE("Trying to unregister buffer %p from process %d that was not imported into current process: %d", hnd,
		     hnd->remote_pid, getpid());
	}

	pthread_mutex_unlock(&s_map_lock);
	return 0;
}
