/*
 * Copyright (C) 2016 ARM Limited. All rights reserved.
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
#ifndef MALI_GRALLOC_BUFFERALLOCATION_H_
#define MALI_GRALLOC_BUFFERALLOCATION_H_

#include "gralloc_helper.h"

int mali_gralloc_buffer_allocate(mali_gralloc_module const* m, int format, uint64_t consumer_usage, uint64_t producer_usage, int w, int h, uint32_t layer_count, buffer_handle_t* pHandle);
int mali_gralloc_buffer_free(mali_gralloc_module const* m, buffer_handle_t pHandle);

static inline unsigned int _select_heap(uint64_t consumer_usage, uint64_t producer_usage)
{
	unsigned int heap_mask;
	if (producer_usage & GRALLOC1_PRODUCER_USAGE_PROTECTED)
	{
		if (GRALLOC1_SLSI_USAGE_CHECK(producer_usage, GRALLOC1_PRODUCER_USAGE_PRIVATE_NONSECURE))
			heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
		else {
			if (GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_EXT))
				heap_mask = EXYNOS_ION_HEAP_VIDEO_STREAM_MASK;
			else if ((consumer_usage & GRALLOC1_CONSUMER_USAGE_HWCOMPOSER) &&
					!(consumer_usage & GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE) &&
					!(producer_usage & GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET))
				heap_mask = EXYNOS_ION_HEAP_VIDEO_SCALER_MASK;
			else
				heap_mask = EXYNOS_ION_HEAP_VIDEO_FRAME_MASK;
		}
	}
	else if (GRALLOC1_SLSI_USAGE_CHECK(producer_usage, GRALLOC1_PRODUCER_USAGE_CAMERA_RESERVED))
	{
		heap_mask = EXYNOS_ION_HEAP_CAMERA_MASK;
	}
	else if (GRALLOC1_SLSI_USAGE_CHECK(producer_usage, GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED))
	{
		heap_mask = EXYNOS_ION_HEAP_SECURE_CAMERA_MASK;
	}
	else
	{
		heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
	}

	return heap_mask;
}

static inline int adjustFrameworkFormats(int frameworkFormat, uint64_t consumer_usage, uint64_t producer_usage)
{
	int format = frameworkFormat;
	if (format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
	{
		AINF("HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED : consumer usage(%#" PRIx64 "), producer usage(%#" PRIx64 ")\n", consumer_usage, producer_usage);
		if ((consumer_usage & GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE) || (consumer_usage & GRALLOC1_CONSUMER_USAGE_HWCOMPOSER))
		{
			if(GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_YUV_RANGE_FULL))
			{
				format = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL; // NV21M Full
			}
			else
			{
				format = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;    //NV21M narrow
			}
		}
		else if (consumer_usage & GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER)
		{
			format = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;    //NV21M narrow
		}
		else if (GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
		{
			format = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
		}
		else if ((consumer_usage & GRALLOC1_CONSUMER_USAGE_CAMERA) && (producer_usage & GRALLOC1_PRODUCER_USAGE_CAMERA))
		{
			format = HAL_PIXEL_FORMAT_YCbCr_422_I; // YUYV
		}
		else
		{
			format = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;    //NV21M narrow
		}
	}
	else if (format == HAL_PIXEL_FORMAT_YCbCr_420_888)
	{
		if (consumer_usage & GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER)
		{
			format = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
		}
		else
		{
			// Flexible framework-accessible YUV format; map to NV21 for now
			format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
		}
	}

	return format;
}

static inline int getYUVFdsAndSizes(int ionfd, int planes, int luma_size, int chroma_size, uint64_t producer_usage, uint64_t consumer_usage,
						unsigned int heap_mask, unsigned int ion_flags, int flags, int* fd, int* fd1, int* fd2, int* size, int* size1, int* size2, uint32_t layer_count)
{
	GRALLOC_UNUSED(consumer_usage);

	// fd
	*size = luma_size * layer_count;
	if (*size <= SIZE_4K)
		*size += SIZE_4K;

	*fd = exynos_ion_alloc(ionfd, *size, heap_mask, ion_flags);
	if (*fd < 0)
	{
		AERR("failed to get fd from exynos_ion_alloc, %s, %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	// fd1
	if (planes > 1)
	{
		if (GRALLOC1_SLSI_USAGE_CHECK(flags, private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA))
		{
			*size1 = PRIV_SIZE;
			heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
			ion_flags = 0;
		}
		else
		{
			*size1 = chroma_size * layer_count;
			if (*size1 <= SIZE_4K)
				*size1 += SIZE_4K;
		}

		*fd1 = exynos_ion_alloc(ionfd, *size1, heap_mask, ion_flags);
		if (*fd1 < 0)
		{
			AERR("failed to get fd1 from exynos_ion_alloc, %s, %d\n", __func__, __LINE__);
			close(*fd);
			return -EINVAL;
		}

		// fd2
		if (planes == 3)
		{
			if (GRALLOC1_SLSI_USAGE_CHECK(flags, private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA))
			{
				*size2 = PRIV_SIZE;
				heap_mask = EXYNOS_ION_HEAP_SYSTEM_MASK;
				ion_flags = 0;
			}
			else
			{
				*size2 = chroma_size * layer_count;
				if (*size2 <= SIZE_4K)
					*size2 += SIZE_4K;
			}
			*fd2 = exynos_ion_alloc(ionfd, *size2, heap_mask, ion_flags);
			if (*fd2 < 0)
			{
				AERR("failed to get fd2 from exynos_ion_alloc, %s, %d\n", __func__, __LINE__);
				close(*fd);
				close(*fd1);
				return -EINVAL;
			}
		}
	}

	return 0;
}
#endif /* MALI_GRALLOC_BUFFERALLOCATION_H_ */
