/*
 * Copyright (C) 2020 ARM Limited. All rights reserved.
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

#include "gralloc_capabilities.h"

#include <string.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <assert.h>
#include <pthread.h>

#include "core/format_info.h"

/* Writing to runtime_caps_read is guarded by mutex caps_init_mutex. */
static pthread_mutex_t caps_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool runtime_caps_read = false;

mali_gralloc_format_caps cpu_runtime_caps;
mali_gralloc_format_caps dpu_runtime_caps;
mali_gralloc_format_caps vpu_runtime_caps;
mali_gralloc_format_caps gpu_runtime_caps;
mali_gralloc_format_caps cam_runtime_caps;

void get_ip_capabilities(void)
{
	/* Ensure capability setting is not interrupted by other
	 * allocations during start-up.
	 */
	pthread_mutex_lock(&caps_init_mutex);

	if (runtime_caps_read)
	{
		goto already_init;
	}

	sanitize_formats();

	memset((void *)&cpu_runtime_caps, 0, sizeof(cpu_runtime_caps));
	memset((void *)&dpu_runtime_caps, 0, sizeof(dpu_runtime_caps));
	memset((void *)&vpu_runtime_caps, 0, sizeof(vpu_runtime_caps));
	memset((void *)&gpu_runtime_caps, 0, sizeof(gpu_runtime_caps));
	memset((void *)&cam_runtime_caps, 0, sizeof(cam_runtime_caps));

	/* Determine CPU IP capabilities */
	cpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
	cpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102;
	cpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616;

	/* Determine DPU IP capabilities */
	dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
	dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;
#if defined(GRALLOC_DPU_SUPPORT_1010102_AFBC) && (GRALLOC_DPU_SUPPORT_1010102_AFBC == 1)
	dpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102;
#endif

#if defined(MALI_GPU_SUPPORT_AFBC_BASIC) && (MALI_GPU_SUPPORT_AFBC_BASIC == 1)
	gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;
	gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_AFBC_BASIC;
	gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA1010102;
	gpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_PIXFMT_RGBA16161616;

#endif /* defined(MALI_GPU_SUPPORT_AFBC_BASIC) && (MALI_GPU_SUPPORT_AFBC_BASIC == 1) */

	/* Determine VPU IP capabilities */
	vpu_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;

/* Build specific capability changes */
#if defined(GRALLOC_ARM_NO_EXTERNAL_AFBC) && (GRALLOC_ARM_NO_EXTERNAL_AFBC == 1)
	dpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	gpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	vpu_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
	cam_runtime_caps.caps_mask &= ~MALI_GRALLOC_FORMAT_CAPABILITY_AFBCENABLE_MASK;
#endif

	cam_runtime_caps.caps_mask |= MALI_GRALLOC_FORMAT_CAPABILITY_OPTIONS_PRESENT;

	runtime_caps_read = true;

already_init:
	pthread_mutex_unlock(&caps_init_mutex);

	MALI_GRALLOC_LOGV("GPU format capabilities 0x%" PRIx64, gpu_runtime_caps.caps_mask);
	MALI_GRALLOC_LOGV("DPU format capabilities 0x%" PRIx64, dpu_runtime_caps.caps_mask);
	MALI_GRALLOC_LOGV("VPU format capabilities 0x%" PRIx64, vpu_runtime_caps.caps_mask);
	MALI_GRALLOC_LOGV("CAM format capabilities 0x%" PRIx64, cam_runtime_caps.caps_mask);
}


/* This is used by the unit tests to get the capabilities for each IP. */
extern "C" {
	void mali_gralloc_get_caps(struct mali_gralloc_format_caps *gpu_caps,
	                           struct mali_gralloc_format_caps *vpu_caps,
	                           struct mali_gralloc_format_caps *dpu_caps,
	                           struct mali_gralloc_format_caps *cam_caps)
	{
		get_ip_capabilities();

		memcpy(gpu_caps, (void *)&gpu_runtime_caps, sizeof(*gpu_caps));
		memcpy(vpu_caps, (void *)&vpu_runtime_caps, sizeof(*vpu_caps));
		memcpy(dpu_caps, (void *)&dpu_runtime_caps, sizeof(*dpu_caps));
		memcpy(cam_caps, (void *)&cam_runtime_caps, sizeof(*cam_caps));
	}
}
