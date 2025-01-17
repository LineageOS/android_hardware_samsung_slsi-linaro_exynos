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

#include "mali_gralloc_bufferaccess.h"

static int gralloc_map(mali_gralloc_module const* module, buffer_handle_t handle)
{
	private_handle_t *hnd = (private_handle_t*)handle;
	int err = 0;
	hnd->base = hnd->base1 = hnd->base2 = 0;

	// AFBC must not be mapped.
	if (hnd->is_compressible)
		return 0;

	// Map for Private data
	mapForPrivateData(module, hnd);

	// Don't be mapped for Secure DRM & Secure Camera
	if (((hnd->producer_usage & GRALLOC1_PRODUCER_USAGE_PROTECTED) && !GRALLOC1_SLSI_USAGE_CHECK(hnd->consumer_usage, GRALLOC1_PRODUCER_USAGE_PRIVATE_NONSECURE))
		|| GRALLOC1_SLSI_USAGE_CHECK(hnd->producer_usage, GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED))
		return 0;

	// Map for Buffer
	if (!(hnd->producer_usage & GRALLOC1_PRODUCER_USAGE_PROTECTED)
		&& !GRALLOC1_SLSI_USAGE_CHECK(hnd->producer_usage, GRALLOC1_PRODUCER_USAGE_NOZEROED)
		&& !GRALLOC1_SLSI_USAGE_CHECK(hnd->producer_usage, GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED))
	{
		// Map for fd
		if (hnd->fd >= 0 && !hnd->base)
		{
			err = mapForFds(module, hnd, 0);
			if (err)
				return err;
		}
		// Map for fd1
		if (hnd->fd1 >= 0 && !hnd->base1)
		{
			err = mapForFds(module, hnd, 1);
			if (err)
				return err;
		}
		// Map for fd2
		if (hnd->fd2 >= 0 && !hnd->base2)
		{
			err = mapForFds(module, hnd, 2);
			if (err)
				return err;
		}
	}

	return 0;
}

static int gralloc_unmap(buffer_handle_t handle)
{
	private_handle_t* hnd = (private_handle_t*)handle;

	// Unmap for Private data
	unmapForPrivateData(hnd);

	// Early-out for Unmapped Buffer
	if (!hnd->base)
		return 0;

	// Unmap for fd
	if (hnd->fd >= 0 && hnd->base)
	{
		if (munmap(INT_TO_PTR(hnd->base), hnd->size) < 0)
			AERR("%s(%d) :could not unmap %s %d %#" PRIx64, __func__, __LINE__, strerror(errno), hnd->size, hnd->base);

		hnd->base = 0;
	}

	// Unmap for fd1
	if (hnd->fd1 >= 0 && hnd->base1)
	{
		if (munmap(INT_TO_PTR(hnd->base1), hnd->size1) < 0)
			AERR("%s(%d) :could not unmap %s %d %#" PRIx64, __func__, __LINE__, strerror(errno), hnd->size1, hnd->base1);

		hnd->base1 = 0;
	}

	// Unmap for fd2
	if (hnd->fd2 >= 0 && hnd->base2)
	{
		if (munmap(INT_TO_PTR(hnd->base2), hnd->size2) < 0)
			AERR("%s(%d) :could not unmap %s %d %#" PRIx64, __func__, __LINE__, strerror(errno), hnd->size2, hnd->base2);

		hnd->base2 = 0;
	}
	return 0;
}

int grallocUnmap(private_handle_t *hnd)
{
	return gralloc_unmap(hnd);
}

int gralloc_register_buffer(mali_gralloc_module const* module,
		buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
		return -EINVAL;

	gralloc_map(module, handle);

	private_handle_t* hnd = (private_handle_t*)handle;

	int ret;
	ret = exynos_ion_import_handle(getIonFd(module), hnd->fd, &hnd->handle);
	if (ret)
		AERR("error importing handle fd(%d) format(%x)\n", hnd->fd, hnd->format);

	if (hnd->fd1 >= 0)
	{
		ret = exynos_ion_import_handle(getIonFd(module), hnd->fd1, &hnd->handle1);
		if (ret)
			AERR("error importing handle1 fd1(%d) format(%x)\n", hnd->fd1, hnd->format);
	}
	if (hnd->fd2 >= 0)
	{
		ret = exynos_ion_import_handle(getIonFd(module), hnd->fd2, &hnd->handle2);
		if (ret)
			AERR("error importing handle2 fd2(%d) format(%x)\n", hnd->fd2, hnd->format);
	}

	return 0;
}

int gralloc_unregister_buffer(mali_gralloc_module const* module,
		buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
		return -EINVAL;

	private_handle_t* hnd = (private_handle_t*)handle;

	gralloc_unmap(handle);

	if (hnd->handle)
		exynos_ion_free_handle(getIonFd(module), hnd->handle);
	if (hnd->handle1)
		exynos_ion_free_handle(getIonFd(module), hnd->handle1);
	if (hnd->handle2)
		exynos_ion_free_handle(getIonFd(module), hnd->handle2);

	return 0;
}

int mali_gralloc_lock(const mali_gralloc_module* m, buffer_handle_t buffer, uint64_t usage, int l, int t, int w, int h, void** vaddr)
{
	GRALLOC_UNUSED(m);
	GRALLOC_UNUSED(l);
	GRALLOC_UNUSED(t);
	GRALLOC_UNUSED(w);
	GRALLOC_UNUSED(h);

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Locking invalid buffer %p, returning error", buffer);
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)buffer;

	if (hnd->internal_format == HAL_PIXEL_FORMAT_YCbCr_420_888)
	{
		AERR("Buffers with format YCbCr_420_888 must be locked using (*lock_ycbcr)" );
		return -EINVAL;
	}

	switch(hnd->format)
	{
		case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_RGBX_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
		case HAL_PIXEL_FORMAT_RGB_888:
		case HAL_PIXEL_FORMAT_RGB_565:
		case HAL_PIXEL_FORMAT_RAW16:
		case HAL_PIXEL_FORMAT_RAW_OPAQUE:
		case HAL_PIXEL_FORMAT_BLOB:
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
		case HAL_PIXEL_FORMAT_Y8:
		case HAL_PIXEL_FORMAT_Y16:
		case HAL_PIXEL_FORMAT_YV12:
		case HAL_PIXEL_FORMAT_RGBA_1010102:
		case HAL_PIXEL_FORMAT_RGBA_FP16:
			break;
		default:
			AERR("mali_gralloc_lock doesn't support YUV formats. Please use gralloc_lock_ycbcr()");
			return -EINVAL;
	}

	// For Range Flush
	lockForRangeFlush(hnd, usage, t, h);

	// Map if CPU address is NULL
	if (!hnd->base)
	{
		gralloc_map(m, hnd);
	}

	if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_ION))
		hnd->writeOwner = usage & GRALLOC1_USAGE_SW_WRITE_MASK;

	// return CPU address
	if (usage & (GRALLOC1_USAGE_SW_READ_MASK | GRALLOC1_USAGE_SW_WRITE_MASK))
		*vaddr = INT_TO_PTR(hnd->base);

	return 0;
}

int mali_gralloc_unlock(const mali_gralloc_module* module, buffer_handle_t buffer)
{
	if (private_handle_t::validate(buffer) < 0)
	{
		AERR( "Unlocking invalid buffer %p, returning error", buffer);
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)buffer;

	// Ealry out for Non-cachable buffer
	if (!((hnd->consumer_usage & GRALLOC1_USAGE_SW_READ_MASK) == GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN))
		return 0;

	if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_ION) && hnd->writeOwner && !hnd->is_compressible)
	{
		// Cache flush & Invalidate for all fds
		doCacheFlushAndInvalidate(module, hnd);
	}

	return 0;
}

int mali_gralloc_get_num_flex_planes(const mali_gralloc_module* m, buffer_handle_t buffer, uint32_t* num_planes)
{
	GRALLOC_UNUSED(m);

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Invalid buffer %p, returning error", buffer);
		return -EINVAL;
	}
	if (NULL == num_planes)
	{
		return -EINVAL;
	}

	private_handle_t* hnd = (private_handle_t*)buffer;
	uint64_t base_format = hnd->internal_format & GRALLOC_ARM_INTFMT_FMT_MASK;

	// the number of YCBCR planes should be 3.
	switch (base_format)
	{
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		case HAL_PIXEL_FORMAT_YV12:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
		case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
		case HAL_PIXEL_FORMAT_Y8:
		case HAL_PIXEL_FORMAT_Y16:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
			*num_planes = 3;
			break;

		default:
			AERR("Can't get planes number of buffer %p: with format %" PRIx64, hnd, hnd->internal_format);
			return -EINVAL;
	}
	return 0;
}

int mali_gralloc_lock_async(const mali_gralloc_module* m, buffer_handle_t buffer, uint64_t usage,
		int l, int t, int w, int h, void** vaddr, int32_t fence_fd)
{
	if (fence_fd >= 0)
	{
		sync_wait(fence_fd, -1);
		close(fence_fd);
	}
	return mali_gralloc_lock(m, buffer, usage, l, t, w, h, vaddr);
}

static int getYCbCrInfo(private_handle_t *hnd, uint64_t usage, android_ycbcr* ycbcr)
{
	// Calculate offsets to underlying YUV data
	int ext_size = 256;
	size_t yStride;
	size_t cStride;
	size_t uOffset;
	size_t vOffset;
	size_t cStep;
	switch (hnd->format)
	{
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
			yStride = cStride = hnd->stride;
			vOffset = 1;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)((unsigned long)hnd->base1);
			ycbcr->cr = (void *)(((unsigned long)hnd->base1) + vOffset);

			if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA)
				&& GRALLOC1_SLSI_USAGE_CHECK(usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
			{
				ycbcr->cr = (void *)((unsigned long)hnd->base2);
			}
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
			yStride = cStride = hnd->stride;
			uOffset = (SBWC_8B_Y_SIZE(hnd->width, hnd->height) + SBWC_8B_Y_HEADER_SIZE(hnd->width, hnd->height));
			vOffset = uOffset + 1;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)(((unsigned long)hnd->base) + uOffset);
			ycbcr->cr = (void *)(((unsigned long)hnd->base) + vOffset);
			if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA)
				&& GRALLOC1_SLSI_USAGE_CHECK(usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
			{
				ycbcr->cr = (void *)((unsigned long)hnd->base1);
			}
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
			yStride = cStride = hnd->stride;
			uOffset = NV12N_S8B_LUMA_SIZE(yStride, hnd->vstride, ext_size) + NV12N_S2B_SIZE(hnd->width, hnd->vstride);
			uOffset = (SBWC_10B_Y_SIZE(hnd->width, hnd->height) + SBWC_10B_Y_HEADER_SIZE(hnd->width, hnd->height));
			vOffset = uOffset + 1;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)(((unsigned long)hnd->base) + uOffset);
			ycbcr->cr = (void *)(((unsigned long)hnd->base) + vOffset);
			if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA)
				&& GRALLOC1_SLSI_USAGE_CHECK(usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
			{
				ycbcr->cr = (void *)((unsigned long)hnd->base1);
			}
			break;
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
			yStride = cStride = hnd->width;
			vOffset = yStride * hnd->height;
			uOffset = vOffset + 1;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)(((unsigned long)hnd->base) + uOffset);
			ycbcr->cr = (void *)(((unsigned long)hnd->base) + vOffset);
			break;
		case HAL_PIXEL_FORMAT_YV12:
			yStride = GRALLOC_ALIGN(hnd->width, 16);
			cStride = GRALLOC_ALIGN(yStride/2, 16);
			vOffset = yStride * hnd->height;
			uOffset = vOffset + (cStride * (hnd->height / 2));
			cStep = 1;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cr = (void *)(((unsigned long)hnd->base) + vOffset);
			ycbcr->cb = (void *)(((unsigned long)hnd->base) + uOffset);
			cStep = 1;
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
			yStride = cStride = hnd->stride;
			vOffset = 1;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)((unsigned long)hnd->base1);
			ycbcr->cr = (void *)(((unsigned long)hnd->base1) + vOffset);

			if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA)
				&& GRALLOC1_SLSI_USAGE_CHECK(usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
			{
				ycbcr->cr = (void *)((unsigned long)hnd->base2);
			}
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P:
			yStride = hnd->stride;
			cStride = GRALLOC_ALIGN(yStride/2, 16);
			uOffset = yStride * hnd->height;
			vOffset = uOffset + (cStride * (hnd->height / 2));
			cStep = 1;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)(((unsigned long)hnd->base) + uOffset);
			ycbcr->cr = (void *)(((unsigned long)hnd->base) + vOffset);
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
			if (usage & GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER)
			{
				yStride = hnd->stride;
				cStride = hnd->stride;
				cStep   = 1;
				ycbcr->y  = (void *)((unsigned long)hnd->base);
				ycbcr->cb = (void *)((unsigned long)hnd->base1);
				ycbcr->cr = NULL;
			} else {
				AERR("getYCbCrInfo unexpected internal format %x", hnd->format);
				return -EINVAL;
			}
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
			yStride = cStride = hnd->stride;
			uOffset = 1;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cr = (void *)((unsigned long)hnd->base1);
			ycbcr->cb = (void *)(((unsigned long)hnd->base1) + uOffset);
			if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA)
				&& GRALLOC1_SLSI_USAGE_CHECK(usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
			{
				ycbcr->cb = (void *)((unsigned long)hnd->base2);
			}
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
			yStride = hnd->stride;
			cStride = GRALLOC_ALIGN(yStride/2, 16);
			cStep = 1;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)((unsigned long)hnd->base1);
			ycbcr->cr = (void *)((unsigned long)hnd->base2);
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
			yStride = hnd->stride;
			cStride = GRALLOC_ALIGN(yStride/2, 16);
			cStep = 1;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cr = (void *)((unsigned long)hnd->base1);
			ycbcr->cb = (void *)((unsigned long)hnd->base2);
			break;
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
			yStride = cStride = hnd->stride;
			cStep = 1;
			ycbcr->y = (void *)((unsigned long)hnd->base);
			ycbcr->cb = 0;
			ycbcr->cr = 0;
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
			yStride = cStride = hnd->stride;
			uOffset = NV12N_S8B_LUMA_SIZE(yStride, hnd->vstride, ext_size);
			vOffset = uOffset + 1;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)(((unsigned long)hnd->base) + uOffset);
			ycbcr->cr = (void *)(((unsigned long)hnd->base) + vOffset);
			if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA)
				&& GRALLOC1_SLSI_USAGE_CHECK(usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
			{
				ycbcr->cr = (void *)((unsigned long)hnd->base1);
			}
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
			yStride = cStride = hnd->stride;
			uOffset = NV12N_S8B_LUMA_SIZE(yStride, hnd->vstride, ext_size) + NV12N_S2B_SIZE(hnd->width, hnd->vstride);
			vOffset = uOffset + 1;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)(((unsigned long)hnd->base) + uOffset);
			ycbcr->cr = (void *)(((unsigned long)hnd->base) + vOffset);
			if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA)
				&& GRALLOC1_SLSI_USAGE_CHECK(usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
			{
				ycbcr->cr = (void *)((unsigned long)hnd->base1);
			}
			break;
		case HAL_PIXEL_FORMAT_Y8:
		case HAL_PIXEL_FORMAT_Y16:
			yStride = cStride = hnd->stride;
			uOffset = 0;
			vOffset = 0;
			cStep = 1;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = 0;
			ycbcr->cr = 0;
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
			yStride = cStride = hnd->stride;
			vOffset = 2;
			cStep = 2;
			ycbcr->y  = (void *)((unsigned long)hnd->base);
			ycbcr->cb = (void *)((unsigned long)hnd->base1);
			ycbcr->cr = (void *)((unsigned long)hnd->base1 + vOffset);
			if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA)
				&& (GRALLOC1_SLSI_USAGE_CHECK(usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA)))
			{
				ycbcr->cr = (void *)((unsigned long)hnd->base2);
			}
			break;
		default:
			AERR("getYCbCrInfo unexpected internal format %x",
					hnd->format);
			return -EINVAL;
	}

	ycbcr->ystride = yStride;
	ycbcr->cstride = cStride;
	ycbcr->chroma_step = cStep;

	// Zero out reserved fields
	memset(ycbcr->reserved, 0, sizeof(ycbcr->reserved));

	return 0;
}

int mali_gralloc_lock_flex_async(const mali_gralloc_module* m, buffer_handle_t buffer, uint64_t usage,
		int l, int t, int w, int h,
		struct android_flex_layout* flex_layout, int32_t fence_fd)
{
	GRALLOC_UNUSED(m);
	GRALLOC_UNUSED(l);
	GRALLOC_UNUSED(t);
	GRALLOC_UNUSED(w);
	GRALLOC_UNUSED(h);

	if (private_handle_t::validate(buffer) < 0)
	{
		AERR("Locking invalid buffer %p, returning error", buffer);
		return -EINVAL;
	}
	if (NULL == flex_layout)
	{
		return -EINVAL;
	}
	if (fence_fd >= 0)
	{
		sync_wait(fence_fd, -1);
		close(fence_fd);
	}

	private_handle_t* hnd = (private_handle_t*)buffer;

	if (GRALLOC1_FLAGS_CHECK(hnd->flags, private_handle_t::PRIV_FLAGS_USES_ION))
		hnd->writeOwner = usage & GRALLOC1_USAGE_SW_WRITE_MASK;

	// Map if all CPU addresses are NULL
	if (!hnd->base && !hnd->base1 && !hnd->base2)
		gralloc_map(m, hnd);

	android_ycbcr ycbcr;
	// Get CPU addresses and stride
	if(getYCbCrInfo(hnd, usage, &ycbcr))
		return -EINVAL;

	// Convert android_ycbcr to android_flex_layout
	if (usage & (GRALLOC1_USAGE_SW_READ_MASK | GRALLOC1_USAGE_SW_WRITE_MASK) &&
			!(hnd->internal_format & GRALLOC_ARM_INTFMT_EXT_MASK))
	{
		flex_layout->format = FLEX_FORMAT_YCbCr;
		flex_layout->num_planes = 3;
		for (uint32_t i = 0; i < flex_layout->num_planes; i++) {
			flex_layout->planes[i].bits_per_component = 8;
			flex_layout->planes[i].bits_used = 8;
			flex_layout->planes[i].h_increment = 1;
			flex_layout->planes[i].v_increment = 1;
			flex_layout->planes[i].h_subsampling = 2;
			flex_layout->planes[i].v_subsampling = 2;
		}

		flex_layout->planes[0].top_left = static_cast<uint8_t *>(ycbcr.y);
		flex_layout->planes[0].component = FLEX_COMPONENT_Y;
		flex_layout->planes[0].v_increment = static_cast<int32_t>(ycbcr.ystride);

		flex_layout->planes[1].top_left = static_cast<uint8_t *>(ycbcr.cb);
		flex_layout->planes[1].component = FLEX_COMPONENT_Cb;
		flex_layout->planes[1].h_increment = static_cast<int32_t>(ycbcr.chroma_step);
		flex_layout->planes[1].v_increment = static_cast<int32_t>(ycbcr.cstride);

		flex_layout->planes[2].top_left = static_cast<uint8_t *>(ycbcr.cr);
		flex_layout->planes[2].component = FLEX_COMPONENT_Cr;
		flex_layout->planes[2].h_increment = static_cast<int32_t>(ycbcr.chroma_step);
		flex_layout->planes[2].v_increment = static_cast<int32_t>(ycbcr.cstride);
	}
	else if (is_sbwc_format(hnd->format))
	{
		flex_layout->format = FLEX_FORMAT_YCbCr;
		flex_layout->num_planes = 3;

		switch (hnd->format)
		{
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
			case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
				for (uint32_t i = 0; i < flex_layout->num_planes; i++) {
					flex_layout->planes[i].bits_per_component = 8;
					flex_layout->planes[i].bits_used = 8;
					flex_layout->planes[i].h_increment = 1;
					flex_layout->planes[i].v_increment = 1;
					flex_layout->planes[i].h_subsampling = 2;
					flex_layout->planes[i].v_subsampling = 2;
				}
				break;
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
			case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
				for (uint32_t i = 0; i < flex_layout->num_planes; i++) {
					flex_layout->planes[i].bits_per_component = 10;
					flex_layout->planes[i].bits_used = 10;
					flex_layout->planes[i].h_increment = 1;
					flex_layout->planes[i].v_increment = 1;
					flex_layout->planes[i].h_subsampling = 2;
					flex_layout->planes[i].v_subsampling = 2;
				}
		}

		flex_layout->planes[0].top_left = static_cast<uint8_t *>(ycbcr.y);
		flex_layout->planes[0].component = FLEX_COMPONENT_Y;
		flex_layout->planes[0].v_increment = static_cast<int32_t>(ycbcr.ystride);

		flex_layout->planes[1].top_left = static_cast<uint8_t *>(ycbcr.cb);
		flex_layout->planes[1].component = FLEX_COMPONENT_Cb;
		flex_layout->planes[1].h_increment = static_cast<int32_t>(ycbcr.chroma_step);
		flex_layout->planes[1].v_increment = static_cast<int32_t>(ycbcr.cstride);

		flex_layout->planes[2].top_left = static_cast<uint8_t *>(ycbcr.cr);
		flex_layout->planes[2].component = FLEX_COMPONENT_Cr;
		flex_layout->planes[2].h_increment = static_cast<int32_t>(ycbcr.chroma_step);
		flex_layout->planes[2].v_increment = static_cast<int32_t>(ycbcr.cstride);
	}
	else
	{
		AERR("Don't support to lock buffer %p: with format %" PRIx64, hnd, hnd->internal_format);
		return -EINVAL;
	}
	return 0;
}

int mali_gralloc_unlock_async(const mali_gralloc_module* m, buffer_handle_t buffer, int32_t* fence_fd)
{
	*fence_fd = -1;
	if (mali_gralloc_unlock(m, buffer) < 0)
	{
		return -EINVAL;
	}
	return 0;
}
