/*
 * Copyright 2016 The Android Open Source Project
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

#include <exynos_format.h>
#include <log/log.h>
#include <sync/sync.h>
#include <unistd.h>
/* For error codes. */
#include <hardware/gralloc1.h>
#include "GrallocMapperCamera.h"
#include "mali_gralloc_bufferaccess.h"
#include "format_info.h"

/*
 *  Returns the number of flex layout planes which are needed to represent the
 *  given buffer.
 *
 * @param m           [in]   Gralloc module.
 * @param buffer      [in]   The buffer handle for which the number of planes should be queried
 * @param num_planes  [out]  The number of flex planes required to describe the given buffer
 *
 * @return GRALLOC1_ERROR_NONE         The buffer's format can be represented in flex layout
 *         GRALLOC1_ERROR_UNSUPPORTED - The buffer's format can't be represented in flex layout
 */
int mali_gralloc_get_num_flex_planes(const buffer_handle_t buffer,
                                     uint32_t * const num_planes)
{

	private_handle_t *hnd = (private_handle_t *)buffer;
	const uint32_t base_format = hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	if ((hnd->alloc_format & MALI_GRALLOC_INTFMT_EXT_MASK) != 0)
	{
		MALI_GRALLOC_LOGE("AFBC enabled buffers can't be represented in flex layout."
		     "Internal Format:0x%" PRIx64, hnd->alloc_format);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	const int32_t format_idx = get_format_index(base_format);
	if (format_idx == -1)
	{
		MALI_GRALLOC_LOGE("Corrupted buffer format 0x%" PRIx64 " of buffer %p", hnd->alloc_format, hnd);
		return -EINVAL;
	}

	if (formats[format_idx].flex != true)
	{
		MALI_GRALLOC_LOGE("Format 0x%" PRIx64 " of %p can't be represented in flex", hnd->alloc_format, hnd);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	*num_planes = formats[format_idx].total_components();

	return GRALLOC1_ERROR_NONE;
}

/*
 *  Sets Android flex layout parameters.
 *
 * @param top_left            [in]  Pointer to the first byte of the top-left
 *                                  pixel of the plane.
 * @param component           [in]  Plane's flex format (YUVA/RGBA).
 * @param bits_per_component  [in]  Bits allocated for the component in each pixel.
 * @param bits_used           [in]  Number of the most significant bits used in
 *                                  the format for this component.
 * @param h_increment         [in]  Horizontal increment, in bytes, in plane to
 *                                  traverse to next horizontal pixel
 * @param v_increment         [in]  Vertical increment, in bytes, in plane to
 *                                  traverse to next vertical pixel
 * @param h_subsampling       [in]  Horizontal subsampling. Must be a positive power of 2.
 * @param v_subsampling       [in]  Vertical subsampling. Must be a positive power of 2.
 * @param plane               [out] Flex plane layout, to be composed.
 *
 */
static void set_flex_plane_params(uint8_t * const top_left,
                                  const android_flex_component_t component,
                                  const int32_t bits_per_component,
                                  const int32_t bits_used,
                                  const int32_t h_increment,
                                  const int32_t v_increment,
                                  const int32_t h_subsampling,
                                  const int32_t v_subsampling,
                                  android_flex_plane_t * const plane)
{
	plane->top_left = top_left;
	plane->component = component;
	plane->bits_per_component = bits_per_component;
	plane->bits_used = bits_used;
	plane->h_increment = h_increment;
	plane->v_increment = v_increment;
	plane->h_subsampling = h_subsampling;
	plane->v_subsampling = v_subsampling;

	return;
}


static int get_flexlayout_exynos_formats_only (int base_format, uint64_t usage,
		private_handle_t * const hnd, struct android_flex_layout * const flex_layout)
{
	size_t uOffset;
	size_t vOffset;
	android_ycbcr ycbcr;

	switch (base_format)
	{
	case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
	case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		uOffset = 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		ycbcr.cb = (void *)(((unsigned long)hnd->bases[1]) + uOffset);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cb = (void *)((unsigned long)hnd->bases[2]);
		}

		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		vOffset = 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)((unsigned long)hnd->bases[1]);
		ycbcr.cr = (void *)(((unsigned long)hnd->bases[1]) + vOffset);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[2]);
		}
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:

		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		uOffset = hnd->plane_info[1].offset;
		vOffset = uOffset + 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)(((unsigned long)hnd->base) + uOffset);
		ycbcr.cr = (void *)(((unsigned long)hnd->base) + vOffset);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		}
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		uOffset = hnd->plane_info[1].offset;
		vOffset = uOffset + 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)(((unsigned long)hnd->base) + uOffset);
		ycbcr.cr = (void *)(((unsigned long)hnd->base) + vOffset);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		}
		break;
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		vOffset = ycbcr.ystride * hnd->height;
		uOffset = vOffset + 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)(((unsigned long)hnd->base) + uOffset);
		ycbcr.cr = (void *)(((unsigned long)hnd->base) + vOffset);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		}
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L50:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L75:

		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		uOffset = hnd->plane_info[1].offset;
		vOffset = uOffset + 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)(((unsigned long)hnd->base) + uOffset);
		ycbcr.cr = (void *)(((unsigned long)hnd->base) + vOffset);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		}
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80:

		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		uOffset = hnd->plane_info[1].offset;
		vOffset = uOffset + 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)(((unsigned long)hnd->base) + uOffset);
		ycbcr.cr = (void *)(((unsigned long)hnd->base) + vOffset);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		}
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
	case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		uOffset = 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		ycbcr.cb = (void *)(((unsigned long)hnd->bases[1]) + uOffset);
		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cb = (void *)((unsigned long)hnd->bases[2]);
		}

		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		vOffset = 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)((unsigned long)hnd->bases[1]);
		ycbcr.cr = (void *)(((unsigned long)hnd->bases[1]) + vOffset);

		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[2]);
		}

		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P:
		ycbcr.ystride = hnd->plane_info[0].alloc_width;
		ycbcr.cstride = GRALLOC_ALIGN(ycbcr.ystride/2, 16);
		uOffset = ycbcr.ystride * hnd->height;
		vOffset = uOffset + (ycbcr.cstride * (hnd->height / 2));
		ycbcr.chroma_step = 1;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)(((unsigned long)hnd->base) + uOffset);
		ycbcr.cr = (void *)(((unsigned long)hnd->base) + vOffset);
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED:
		if (usage & GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER)
		{
			ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].byte_stride;
			ycbcr.chroma_step   = 1;
			ycbcr.y  = (void *)((unsigned long)hnd->base);
			ycbcr.cb = (void *)((unsigned long)hnd->bases[1]);
			ycbcr.cr = NULL;
		} else {
			MALI_GRALLOC_LOGE("Unexpected internal format %" PRIx64, hnd->internal_format);
			return -EINVAL;
		}
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
		ycbcr.ystride = hnd->plane_info[0].alloc_width;
		ycbcr.cstride = GRALLOC_ALIGN(ycbcr.ystride/2, 16);
		ycbcr.chroma_step = 1;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)((unsigned long)hnd->bases[1]);
		ycbcr.cr = (void *)((unsigned long)hnd->bases[2]);
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
		ycbcr.ystride = hnd->plane_info[0].alloc_width;
		ycbcr.cstride = GRALLOC_ALIGN(ycbcr.ystride/2, 16);
		ycbcr.chroma_step = 1;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		ycbcr.cb = (void *)((unsigned long)hnd->bases[2]);
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		uOffset = hnd->plane_info[1].offset;
		vOffset = uOffset + 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)(((unsigned long)hnd->base) + uOffset);
		ycbcr.cr = (void *)(((unsigned long)hnd->base) + vOffset);
		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		}
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		uOffset = hnd->plane_info[1].offset;
		vOffset = uOffset + 1;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)(((unsigned long)hnd->base) + uOffset);
		ycbcr.cr = (void *)(((unsigned long)hnd->base) + vOffset);
		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[1]);
		}
		break;
	case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
		ycbcr.ystride = ycbcr.cstride = hnd->plane_info[0].alloc_width;
		vOffset = 2;
		ycbcr.chroma_step = 2;
		ycbcr.y  = (void *)((unsigned long)hnd->base);
		ycbcr.cb = (void *)((unsigned long)hnd->bases[1]);
		ycbcr.cr = (void *)((unsigned long)hnd->bases[1] + vOffset);
		if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA
			&& usage & GRALLOC_USAGE_VIDEO_PRIVATE_DATA)
		{
			ycbcr.cr = (void *)((unsigned long)hnd->bases[2]);
		}
		break;
	default:
		MALI_GRALLOC_LOGE("not exynos format: format(0x%" PRIx32 ") usage(0x%" PRIx64 ")", base_format, usage);
		return -1;
	}

	if (hnd->flags & private_handle_t::PRIV_FLAGS_USES_HFR_MODE)
	{
		ycbcr.y = ycbcr.cb = ycbcr.cr = 0;
	}

	flex_layout->format = FLEX_FORMAT_YCbCr;
	flex_layout->num_planes = 3;
	for (uint32_t i = 0; i < flex_layout->num_planes; i++) {
		switch (base_format)
		{
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
			case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60:
			case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80:
				/* Mapper2.0 expoects bits_per_componect and bits_used to be 8
				 * or else locking fails at Mapper2.0
				 */
				flex_layout->planes[i].bits_per_component = 8;
				flex_layout->planes[i].bits_used = 8;
				break;
			default:
				flex_layout->planes[i].bits_per_component = 8;
				flex_layout->planes[i].bits_used = 8;
		}

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

	return 0;
}


/*
 *  Locks the Gralloc 1.0 buffer, for the specified CPU usage, asynchronously.
 *  This function can be called on any format but populates layout parameters
 *  only for formats compatible with Android Flex Format.
 *
 * @param m           [in]   Gralloc module.
 * @param buffer      [in]   The buffer to lock.
 * @param usage       [in]   Producer and consumer combined usage.
 * @param l           [in]   Access region left offset (in pixels).
 * @param t           [in]   Access region top offset (in pixels).
 * @param w           [in]   Access region requested width (in pixels).
 * @param h           [in]   Access region requested height (in pixels).
 * @param flex_layout [out]  Describes flex YUV format for consumption by applications.
 * @param fence_fd    [in]   Refers to an acquire sync fence object.
 *
 * @return 0, when the locking is successful;
 *         Appropriate error, otherwise
 */
static int mali_gralloc_lock_flex_async(const buffer_handle_t buffer,
                                 const uint64_t usage, const int l, const int t,
                                 const int w, const  int h,
                                 struct android_flex_layout * const flex_layout,
                                 const int32_t fence_fd)
{

	if (fence_fd >= 0)
	{
		const int fenceStatus = sync_wait(fence_fd, -1);
		close(fence_fd);

		if (fenceStatus != 0)
		return -EINVAL;
	}

	private_handle_t * const hnd = (private_handle_t *)buffer;
	const uint32_t base_format = hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK;

	/* Validate input parameters for lock request */
	int status = validate_lock_input_parameters(buffer, l, t, w, h, usage);
	if (status != 0)
	{
		return status;
	}

	const int32_t format_idx = get_format_index(base_format);
	if (format_idx == -1)
	{
		MALI_GRALLOC_LOGE("Corrupted buffer format 0x%" PRIx64 " of buffer %p", hnd->alloc_format, hnd);
		return -EINVAL;
	}

	if (formats[format_idx].flex != true)
	{
		MALI_GRALLOC_LOGE("Format 0x%" PRIx64 " of %p can't be represented in flex", hnd->alloc_format, hnd);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	if (hnd->fd_count <= 0 || hnd->fd_count > MAX_PLANES)
	{
        return -EINVAL;
    }

    for (int i = 0; i < hnd->fd_count; i++)
    {
        if (hnd->bases[i] == 0)
        {
            return -EINVAL;
        }
    }

	if (is_exynos_format((uint32_t)base_format))
	{
		if (get_flexlayout_exynos_formats_only(base_format, usage, hnd, flex_layout) < 0)
		{
			MALI_GRALLOC_LOGE("Can't lock buffer %p: format %" PRIx64 " not handled", hnd, hnd->alloc_format);
			return GRALLOC1_ERROR_UNSUPPORTED;
		}

		buffer_sync(hnd, get_tx_direction(usage));

		return GRALLOC1_ERROR_NONE;
	}

	flex_layout->num_planes = formats[format_idx].total_components();
	switch (base_format)
	{
	case MALI_GRALLOC_FORMAT_INTERNAL_Y8:
		flex_layout->format = FLEX_FORMAT_Y;
		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_Y16:
		flex_layout->format = FLEX_FORMAT_Y;
		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 16, 16, 2,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_NV12:
		/* Y:UV 4:2:0 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cb, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 1,
		                      FLEX_COMPONENT_Cr, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_NV21:
		/* Y:VU 4:2:0 ordering. The flex format plane order must still
		 * follow YCbCr order (as defined by 'android_flex_component_t').
		 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 1,
		                      FLEX_COMPONENT_Cb, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cr, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_YV12:
		/* Y:V:U 4:2:0 . The flex format plane order must still follow YCbCr
		 * order (as defined by 'android_flex_component_t').
		 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[2].offset,
		                      FLEX_COMPONENT_Cb, 8, 8, 1,
		                      hnd->plane_info[2].byte_stride, 2, 2,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cr, 8, 8, 1,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_P010:
		/* Y:UV 4:2:0 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 16, 10, 2,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cb, 16, 10, 4,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 2,
		                      FLEX_COMPONENT_Cr, 16, 10, 4,
		                      hnd->plane_info[1].byte_stride, 2, 2,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_P210:
		/* Y:UV 4:2:2 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 16, 10, 2,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cb, 16, 10, 4,
		                      hnd->plane_info[1].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 2,
		                      FLEX_COMPONENT_Cr, 16, 10, 4,
		                      hnd->plane_info[1].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_YUV422_8BIT:
		/* YUYV 4:2:2 */
		flex_layout->format = FLEX_FORMAT_YCbCr;
#ifdef GRALLOC_7885_WA
		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_Cb, 8, 8, 2,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 3, FLEX_COMPONENT_Cr, 8, 8, 2,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);
#else
		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 2,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_Cb, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 3, FLEX_COMPONENT_Cr, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);
#endif

		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_NV16:
		/* Y:UV 4:2:2 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 8, 8, 1,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset,
		                      FLEX_COMPONENT_Cb, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + hnd->plane_info[1].offset + 1,
		                      FLEX_COMPONENT_Cr, 8, 8, 2,
		                      hnd->plane_info[1].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);

		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_Y210:
		/* YUYV 4:2:2 */
		flex_layout->format = FLEX_FORMAT_YCbCr;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_Y, 16, 10, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_Cb, 16, 10, 8,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 6, FLEX_COMPONENT_Cr, 16, 10, 8,
		                      hnd->plane_info[0].byte_stride, 2, 1,
		                      &flex_layout->planes[2]);

		break;

	/* 64-bit format that has 16-bit R, G, B, and A components, in that order */
	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_16161616:
		flex_layout->format = FLEX_FORMAT_RGBA;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_R, 16, 16, 8,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_G, 16, 16, 8,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 4, FLEX_COMPONENT_B, 16, 16, 8,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		set_flex_plane_params((uint8_t *)hnd->base + 6, FLEX_COMPONENT_A, 16, 16, 8,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[3]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_RGBA_8888:
		/* 32-bit format that has 8-bit R, G, B, and A components, in that order */
		flex_layout->format = FLEX_FORMAT_RGBA;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_R, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_G, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_B, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		set_flex_plane_params((uint8_t *)hnd->base + 3, FLEX_COMPONENT_A, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[3]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_RGBX_8888:
		/* 32-bit format that has 8-bit R, G, B, and unused components, in that order */
		flex_layout->format = FLEX_FORMAT_RGB;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_R, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_G, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_B, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_RGB_888:
		/* 24-bit format that has 8-bit R, G, and B components, in that order */
		flex_layout->format = FLEX_FORMAT_RGB;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_R, 8, 8, 3,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_G, 8, 8, 3,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_B, 8, 8, 3,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		break;

	case MALI_GRALLOC_FORMAT_INTERNAL_BGRA_8888:
		/* 32-bit format that has 8-bit B, G, R, and A components, in that order.
		 * The flex format plane order must still follow FLEX_FORMAT_RGBA
		 * order (as defined by 'android_flex_component_t').
		 */
		flex_layout->format = FLEX_FORMAT_RGBA;

		set_flex_plane_params((uint8_t *)hnd->base, FLEX_COMPONENT_B, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[2]);
		set_flex_plane_params((uint8_t *)hnd->base + 1, FLEX_COMPONENT_G, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[1]);
		set_flex_plane_params((uint8_t *)hnd->base + 2, FLEX_COMPONENT_R, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[0]);
		set_flex_plane_params((uint8_t *)hnd->base + 3, FLEX_COMPONENT_A, 8, 8, 4,
		                      hnd->plane_info[0].byte_stride, 1, 1,
		                      &flex_layout->planes[3]);
		break;

	default:
		MALI_GRALLOC_LOGE("Can't lock buffer %p: format 0x%" PRIx64 " not handled", hnd, hnd->alloc_format);
		return GRALLOC1_ERROR_UNSUPPORTED;
	}

	buffer_sync(hnd, get_tx_direction(usage));

	return GRALLOC1_ERROR_NONE;
}

namespace android {

namespace GrallocMapperCamera {

static bool toYCbCrLayout(const android_flex_layout& flex, YCbCrLayout* outLayout) {
    // must be YCbCr
    if (flex.format != FLEX_FORMAT_YCbCr || flex.num_planes < 3) {
        return false;
    }

    for (int i = 0; i < 3; i++) {
        const auto& plane = flex.planes[i];
        // must have 8-bit depth
        if (plane.bits_per_component != 8 || plane.bits_used != 8) {
            return false;
        }

        if (plane.component == FLEX_COMPONENT_Y) {
            // Y must not be interleaved
            if (plane.h_increment != 1) {
                return false;
            }
        } else {
            // Cb and Cr can be interleaved
            if (plane.h_increment != 1 && plane.h_increment != 2) {
                return false;
            }
        }

        if (!plane.v_increment) {
            return false;
        }
    }

    if (flex.planes[0].component != FLEX_COMPONENT_Y ||
        flex.planes[1].component != FLEX_COMPONENT_Cb ||
        flex.planes[2].component != FLEX_COMPONENT_Cr) {
        return false;
    }

    const auto& y = flex.planes[0];
    const auto& cb = flex.planes[1];
    const auto& cr = flex.planes[2];

    if (cb.h_increment != cr.h_increment || cb.v_increment != cr.v_increment) {
        return false;
    }

    outLayout->y = y.top_left;
    outLayout->cb = cb.top_left;
    outLayout->cr = cr.top_left;
    outLayout->yStride = y.v_increment;
    outLayout->cStride = cb.v_increment;
    outLayout->chromaStep = cb.h_increment;

    return true;
}

Error Mapper::lock(buffer_handle_t bufferHandle, uint64_t usage,
        const IMapper::Rect& accessRegion,
        int acquireFence, void** outData) const
{
    if (acquireFence >= 0) {
        const int fenceStatus = sync_wait(acquireFence, -1);
        close(acquireFence);

        if (fenceStatus != 0)
            return Error::BAD_VALUE;
    }
	int ret = mali_gralloc_lock(bufferHandle, usage,
			accessRegion.left,
			accessRegion.top,
			accessRegion.width,
			accessRegion.height,
			outData);

	if (ret)
	{
		MALI_GRALLOC_LOGE("Locking failed with error: %d", ret);
		return Error::BAD_VALUE;
	}

    return Error::NONE;
}

Error Mapper::lock(buffer_handle_t bufferHandle, uint64_t usage,
        const IMapper::Rect& accessRegion,
        int acquireFence, YCbCrLayout* outLayout) const
{
	android_flex_layout flex {};
	int error;

	error = mali_gralloc_get_num_flex_planes(bufferHandle, &flex.num_planes);
	if (error != 0) {
        MALI_GRALLOC_LOGE("Cannot get number of flex planes");
        if (acquireFence >= 0)
            close(acquireFence);
        return Error::BAD_VALUE;
    }

	std::vector<android_flex_plane_t> flexPlanes(flex.num_planes);
    flex.planes = flexPlanes.data();

	error = mali_gralloc_lock_flex_async(bufferHandle, usage,
			accessRegion.left,
			accessRegion.top,
			accessRegion.width,
			accessRegion.height,
			&flex,
			acquireFence);

	if (error != 0)
        return Error::BAD_VALUE;

    if (!toYCbCrLayout(flex, outLayout)) {
        MALI_GRALLOC_LOGE("unable to convert android_flex_layout to YCbCrLayout");
        unlock(bufferHandle);
		return Error::BAD_VALUE;
    }

	return Error::NONE;
}

int Mapper::unlock(buffer_handle_t bufferHandle) const
{
	mali_gralloc_unlock(bufferHandle);

	/* Current gralloc does not create release fence for unlock. return -1 */
	return -1;
}

} // namespace GrallocWrapper

} // namespace android
