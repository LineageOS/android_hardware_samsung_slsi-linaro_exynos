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

#include <string.h>
#include "mali_gralloc_bufferallocation.h"

static int gralloc_alloc_rgb(int ionfd, int w, int h, int format, uint64_t consumer_usage, uint64_t producer_usage,
		unsigned int ion_flags, uint32_t layer_count, private_handle_t **hnd)
{
	size_t ext_size=256, size = 0, size1 = 0, afbc_header_size = 0;
	int bpp = 0, stride = 0, vstride = 0, byte_align_64 = 0, byte_stride = 0;
	int fd = -1, fd1 = -1;
	uint32_t nblocks = 0;

	unsigned int heap_mask = _select_heap(consumer_usage, producer_usage);
	int is_compressible = check_for_compression(w, h, format, consumer_usage, producer_usage);

	bpp = get_bytes_per_pixel(format);
	if (bpp == 0)
		return -EINVAL;

	byte_align_64 = get_byte_align_64(bpp);
	if (byte_align_64 == 0)
		return -EINVAL;

	if (format == HAL_PIXEL_FORMAT_BLOB)
	{

		byte_stride = w * bpp;
		stride = byte_stride / bpp;
		vstride = h;
		size = byte_stride * vstride;
	}
	else
	{
		// Set default stride
		stride = GRALLOC_ALIGN(w, 16);
		vstride = GRALLOC_ALIGN(h, 16);

		byte_stride = GRALLOC_ALIGN_NON_BASE2(stride * bpp, byte_align_64);
		stride = byte_stride / bpp;

		// Calculate plane size
		if (vstride < h + 2)
			size = byte_stride * (h + 2) * layer_count + ext_size;
		else
			size = byte_stride * vstride * layer_count + ext_size;

		if (is_compressible)
		{
			/* if is_compressible = 1, width is alread 16 align so we can use width instead of w_aligned*/
			nblocks = stride / AFBC_PIXELS_PER_BLOCK * vstride / AFBC_PIXELS_PER_BLOCK;
			afbc_header_size = GRALLOC_ALIGN(nblocks * AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY, AFBC_BODY_BUFFER_BYTE_ALIGNMENT);

			// Re-calculate plane size with header size
			size = (byte_stride * vstride + afbc_header_size) * layer_count + ext_size;

			/* Memory must be zeroed for using mmap during afbc header initialization */
			ion_flags &= ~ION_FLAG_NOZEROED;
		}

		// Add MSCL_EXT_SIZE
		if (w % MSCL_ALIGN)
			size += MSCL_EXT_SIZE;
	}

	if (producer_usage & GRALLOC1_PRODUCER_USAGE_PROTECTED)
	{
		ion_flags |= ION_FLAG_PROTECTED;
	}

	if (size <= SIZE_4K)
		size += SIZE_4K;

	fd = exynos_ion_alloc(ionfd, size, heap_mask, ion_flags);
	if (fd < 0)
	{
		AERR("failed to get fd from exynos_ion_alloc, %s, %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (is_compressible)
	{
		// Alloc for AFBC data
		size1 = AFBC_INFO_SIZE;
		fd1 = exynos_ion_alloc(ionfd, size1, EXYNOS_ION_HEAP_SYSTEM_MASK, 0);
		if (fd1 < 0)
		{
			AERR("failed to get fd from exynos_ion_alloc, %s, %d\n", __func__, __LINE__);
			goto error_close_fd_and_return;
		}

		/* Initialize AFBC header */
		unsigned char *afbc_buf =
		    (unsigned char *)mmap(NULL, afbc_header_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		if (MAP_FAILED == afbc_buf)
		{
			AERR("afbc header init mmap failed: fd ( %d ) error: %s", fd, strerror(errno));
			goto error_close_fd_and_return;
		}

		uint32_t headers[4] = { (uint32_t)afbc_header_size, 0x1, 0x10000, 0x0 };

		for (uint32_t i = 0; i < nblocks; i++)
			memcpy(afbc_buf + sizeof(headers) * i, headers, sizeof(headers));

		munmap(afbc_buf, afbc_header_size);
	}

	*hnd = new private_handle_t(fd, fd1, -1, size, size1, 0,
			private_handle_t::PRIV_FLAGS_USES_ION, producer_usage, consumer_usage,
			w, h, format, format, format, stride, byte_stride, vstride, is_compressible, layer_count);

	return 0;

error_close_fd_and_return:
	if (fd  >= 0)
		close(fd);

	if (fd1 >= 0)
		close(fd1);

	return -EINVAL;
}

static int gralloc_alloc_framework_yuv(int ionfd, int w, int h, int format, int frameworkFormat,
		uint64_t consumer_usage, uint64_t producer_usage, unsigned int ion_flags, uint32_t layer_count, private_handle_t **hnd)
{
	size_t plane_size = 0, ext_size = 256;
	int err = 0, fd = -1, stride = 0, byte_stride = 0, size = 0;
	unsigned int heap_mask = _select_heap(consumer_usage, producer_usage);
	int flags = private_handle_t::PRIV_FLAGS_USES_ION;

	// Calculate plane sizes
	switch (format) {
		case HAL_PIXEL_FORMAT_YV12:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P:
			stride = GRALLOC_ALIGN(w, 16);
			byte_stride = stride;
			plane_size = PLANE_SIZE(stride, h, 0) + PLANE_SIZE(GRALLOC_ALIGN(stride / 2, 16), h, ext_size);
			break;
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
			stride = w;
			byte_stride = stride;
			plane_size = PLANE_SIZE(stride , h * 3 / 2, ext_size);
			break;
		case HAL_PIXEL_FORMAT_Y8:
			stride = GRALLOC_ALIGN(w, 16);
			plane_size = PLANE_SIZE(stride, h, 0);
			break;
		case HAL_PIXEL_FORMAT_Y16:
			stride = GRALLOC_ALIGN(w, 16);
			plane_size = PLANE_SIZE(stride, h * 2, 0);
			break;
		default:
			AERR("invalid yuv format %x\n", format);
			return -EINVAL;
	}

	// Add MSCL_EXT_SIZE
	if (w % MSCL_ALIGN)
		plane_size += MSCL_EXT_SIZE;

	if (frameworkFormat == HAL_PIXEL_FORMAT_YCbCr_420_888)
		stride = 0;

	// Allocate & Get Fds
	err = getYUVFdsAndSizes(ionfd, 1, plane_size, 0, producer_usage, consumer_usage, heap_mask, ion_flags,
			flags, &fd, NULL, NULL, &size, NULL, NULL, layer_count);

	if (err)
		return err;

	*hnd = new private_handle_t(fd, -1, -1, size, 0, 0, flags, producer_usage, consumer_usage,
				w, h, format, format, frameworkFormat, stride, byte_stride, h, 0, layer_count);

	return err;
}

static int gralloc_alloc_yuv(int ionfd, int w, int h, int format,
		uint64_t consumer_usage, uint64_t producer_usage, unsigned int ion_flags, uint32_t layer_count, private_handle_t **hnd)
{
	size_t luma_size=0, chroma_size=0, ext_size=256;
	int err = 0, planes = 0, fd = -1, fd1 = -1, fd2 = -1;
	int stride = 0, byte_stride = 0;
	size_t luma_vstride = 0;
	unsigned int heap_mask = _select_heap(consumer_usage, producer_usage);
	// Keep around original requested format for later validation
	int frameworkFormat = format;
	int size = 0, size1 = 0, size2 = 0;
	int flags = private_handle_t::PRIV_FLAGS_USES_ION;

	// Set default stride
	stride = GRALLOC_ALIGN(w, 16);
	luma_vstride = GRALLOC_ALIGN(h, 16);
	byte_stride = stride;

	if ((consumer_usage & GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER) || (producer_usage & GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER))
	{
		consumer_usage |= GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA;
	}

	// adjust Frameworkformat to format
	if ((format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) || (format == HAL_PIXEL_FORMAT_YCbCr_420_888))
	{
		format = adjustFrameworkFormats(format, consumer_usage, producer_usage);
	}

	// DRM or Secure Camera
	if ((producer_usage & GRALLOC1_PRODUCER_USAGE_PROTECTED) || GRALLOC1_SLSI_USAGE_CHECK(producer_usage, GRALLOC1_PRODUCER_USAGE_SECURE_CAMERA_RESERVED))
	{
		ion_flags |= ION_FLAG_PROTECTED;
	}

	/* SWBC Formats have special size requirements */
	switch (format)
	{
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
			{
				luma_size = (SBWC_8B_Y_SIZE(w, h) + SBWC_8B_Y_HEADER_SIZE(w, h));
				chroma_size = (SBWC_8B_CBCR_SIZE(w, h) + SBWC_8B_CBCR_HEADER_SIZE(w, h));

				byte_stride = SBWC_8B_STRIDE(w);
				stride = GRALLOC_ALIGN(w, 32);
				luma_vstride = __ALIGN_UP(h, 8);

				break;
			}
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
			{
				luma_size = (SBWC_10B_Y_SIZE(w, h) + SBWC_10B_Y_HEADER_SIZE(w, h));
				chroma_size = (SBWC_10B_CBCR_SIZE(w, h) + SBWC_10B_CBCR_HEADER_SIZE(w, h));

				byte_stride = SBWC_10B_STRIDE(w);
				stride = GRALLOC_ALIGN(w, 32);
				luma_vstride = __ALIGN_UP(h, 8);

				break;
			}
	}

	// Decide plane count and sizes
	switch (format)
	{
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
			{
				planes = 2;
				if (GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
				{
					flags |= private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA;
					planes++;
				}
				break;
			}
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
			{
				luma_size += chroma_size;
				chroma_size = 0;

				planes = 1;
				if (GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
				{
					flags |= private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA;
					planes++;
				}
				break;
			}
		case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
			{
				stride = GRALLOC_ALIGN(w, 32);
				byte_stride = stride;
				luma_size = PLANE_SIZE(stride, luma_vstride, ext_size);
#ifdef EXYNOS_CHROMA_VSTRIDE_ALIGN
				chroma_size = PLANE_SIZE(GRALLOC_ALIGN(stride / 2, 16), GRALLOC_ALIGN(luma_vstride / 2, CHROMA_VALIGN), ext_size);
#else
				chroma_size = PLANE_SIZE(GRALLOC_ALIGN(stride / 2, 16), (luma_vstride / 2), ext_size);
#endif
				planes = 3;
				break;
			}
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
			{
				size_t chroma_vstride = GRALLOC_ALIGN(h / 2, 32);
				luma_vstride = GRALLOC_ALIGN(h, 32);
				luma_size = PLANE_SIZE(stride, luma_vstride, ext_size);
				chroma_size = PLANE_SIZE(stride, chroma_vstride, ext_size);
				byte_stride = stride * 16;
				planes = 2;
				break;
			}
		case HAL_PIXEL_FORMAT_YV12:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P:
		case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		case HAL_PIXEL_FORMAT_Y8:
		case HAL_PIXEL_FORMAT_Y16:
			return gralloc_alloc_framework_yuv(ionfd, w, h, format, frameworkFormat, consumer_usage, producer_usage, ion_flags, layer_count, hnd);
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
			{
				if (GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
				{
					luma_vstride = GRALLOC_ALIGN(h, 32);
					flags |= private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA;
				}
				luma_size = PLANE_SIZE(stride, luma_vstride, ext_size);
#ifdef EXYNOS_CHROMA_VSTRIDE_ALIGN
				chroma_size = PLANE_SIZE(stride, GRALLOC_ALIGN(luma_vstride / 2, CHROMA_VALIGN), ext_size);
#else
				chroma_size = PLANE_SIZE(stride, luma_vstride / 2, ext_size);
#endif
				planes = GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA) ? 3 : 2;
				break;
			}
		case HAL_PIXEL_FORMAT_YCbCr_422_I:
			{
				luma_vstride = h;
				luma_size = PLANE_SIZE(stride * 2, luma_vstride, ext_size);
				planes = 1;
				break;
			}
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
			{
#ifdef EXYNOS_CHROMA_VSTRIDE_ALIGN
				chroma_size = NV12N_S8B_CHROMA_SIZE(stride, GRALLOC_ALIGN(luma_vstride / 2, CHROMA_VALIGN), ext_size);
#else
				chroma_size = NV12N_S8B_CHROMA_SIZE(stride, luma_vstride / 2, ext_size);
#endif
				luma_size = NV12N_S8B_LUMA_SIZE(stride, luma_vstride, ext_size) + chroma_size;
				planes = GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA) ? 2 : 1;
				if (GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
					flags |= private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA;
				break;
			}
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
			{
				stride = GRALLOC_ALIGN(w, BOARD_EXYNOS_S10B_FORMAT_ALIGN);
				luma_size = NV12M_S8B_SIZE(stride, luma_vstride, ext_size) + NV12M_S2B_LUMA_SIZE(w, h, ext_size);
				chroma_size = NV12M_S8B_SIZE(stride, luma_vstride / 2, ext_size) + NV12M_S2B_CHROMA_SIZE(w, h, ext_size);
				planes = GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA) ? 3 : 2;
				if (GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
					flags |= private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA;
				break;
			}
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
			{
				stride = GRALLOC_ALIGN(w, BOARD_EXYNOS_S10B_FORMAT_ALIGN);
				chroma_size = NV12N_S8B_CHROMA_SIZE(stride, luma_vstride / 2, ext_size) + NV12N_S2B_SIZE(w, luma_vstride / 2);
				luma_size = NV12N_S8B_LUMA_SIZE(stride, luma_vstride, ext_size) + NV12N_S2B_SIZE(w, luma_vstride) + chroma_size;
				planes = GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA) ? 2 : 1;
				if (GRALLOC1_SLSI_USAGE_CHECK(consumer_usage, GRALLOC1_CONSUMER_USAGE_VIDEO_PRIVATE_DATA))
					flags |= private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA;
				break;
			}
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
			{
				chroma_size = P010_PLANE_SIZE(stride, luma_vstride / 2, ext_size);
				luma_size = P010_PLANE_SIZE(stride, luma_vstride, ext_size);
				flags |= private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA;
				planes = 3;
				break;
			}
		default:
			AERR("invalid yuv format %x\n", format);
			return -EINVAL;
	}

	// Add MSCL_EXT_SIZE
	if (w % MSCL_ALIGN)
	{
		luma_size += MSCL_EXT_SIZE;
		chroma_size += MSCL_EXT_SIZE/2;
	}

	// Allocate & Get Fds
	err = getYUVFdsAndSizes(ionfd, planes, luma_size, chroma_size, producer_usage, consumer_usage, heap_mask, ion_flags,
			flags, &fd, &fd1, &fd2, &size, &size1, &size2, layer_count);

	if (err)
		return err;

	*hnd = new private_handle_t(fd, fd1, fd2, size, size1, size2, flags, producer_usage, consumer_usage, w, h,
			format, format, frameworkFormat, stride, byte_stride, luma_vstride, 0, layer_count);

	return err;
}

int mali_gralloc_buffer_allocate(mali_gralloc_module const* m, int format, uint64_t consumer_usage, uint64_t producer_usage, int w, int h, uint32_t layer_count, buffer_handle_t* pHandle)
{
	private_handle_t *hnd = NULL;
	uint64_t usage = consumer_usage | producer_usage;
	unsigned int ion_flags = 0;
	int err = 0;

	if (!pHandle || w <= 0 || h <= 0)
		return -EINVAL;

	if ((usage & GRALLOC1_USAGE_SW_READ_MASK) == GRALLOC1_USAGE_READ_OFTEN)
	{
		ion_flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;

		if (producer_usage & GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET)
		{
			ion_flags |= ION_FLAG_SYNC_FORCE;
		}
	}

	if (producer_usage & GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET)
	{
		ion_flags |= ION_FLAG_MAY_HWRENDER;
	}

	if (GRALLOC1_SLSI_USAGE_CHECK(producer_usage, GRALLOC1_PRODUCER_USAGE_NOZEROED))
	{
		ion_flags |= ION_FLAG_NOZEROED;
	}

	err = gralloc_alloc_rgb(m->ionfd, w, h, format, consumer_usage, producer_usage, ion_flags, layer_count, &hnd);

	if (err)
		err = gralloc_alloc_yuv(m->ionfd, w, h, format, consumer_usage, producer_usage, ion_flags, layer_count, &hnd);

	if (err)
		goto err;

	*pHandle = hnd;
	return 0;
err:
	if (!hnd)
		return err;
	close(hnd->fd);
	if (hnd->fd1 >= 0)
		close(hnd->fd1);
	if (hnd->fd2 >= 0)
		close(hnd->fd2);
	delete hnd;
	return err;
}

int mali_gralloc_buffer_free(mali_gralloc_module const* m, buffer_handle_t handle)
{
	if (private_handle_t::validate(handle) < 0)
		return -EINVAL;

	private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(handle);

	grallocUnmap(const_cast<private_handle_t*>(hnd));

	if (hnd->handle)
		exynos_ion_free_handle(getIonFd(m), hnd->handle);
	if (hnd->handle1)
		exynos_ion_free_handle(getIonFd(m), hnd->handle1);
	if (hnd->handle2)
		exynos_ion_free_handle(getIonFd(m), hnd->handle2);

	close(hnd->fd);
	if (hnd->fd1 >= 0)
		close(hnd->fd1);
	if (hnd->fd2 >= 0)
		close(hnd->fd2);

	return 0;
}
