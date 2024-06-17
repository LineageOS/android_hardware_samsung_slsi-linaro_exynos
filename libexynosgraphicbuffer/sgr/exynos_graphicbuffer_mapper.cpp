/*
 * Copyright (C) 2020 Samsung Electronics Co. Ltd.
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

#include "ExynosGraphicBuffer.h"
#include "exynos_format.h"
#include "metadata_gralloc.h"
#include "private_handle.h"
#include <log/log.h>
#include <ui/Gralloc.h>

using namespace android;
using namespace vendor::graphics;

status_t ExynosGraphicBufferMapper::lock64(buffer_handle_t handle, uint64_t usage, const Rect &bounds, void **vaddr,
                                           int32_t *outBytesPerPixel, int32_t *outBytesPerStride) {
    return lockAsync(handle, usage, usage, bounds, vaddr, -1);
}

status_t ExynosGraphicBufferMapper::lockYCbCr64(buffer_handle_t handle, uint64_t usage, const Rect &bounds,
                                                android_ycbcr *ycbcr) {
    status_t err = getGrallocMapper().lock(handle, usage, bounds, -1, ycbcr);

    if (!(usage & ExynosGraphicBufferUsage::VIDEO_PRIVATE_DATA))
        return err;

    const struct sgr_metadata *metadata = sgr_get_metadata(handle);
    void *v_metadata = sgr_get_metadata_video(handle);

    switch (metadata->alloc_format) {
        case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
            ycbcr->cb = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_YCrCb_420_SP:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L50:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC_L75:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L40:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L60:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC_L80:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
        case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
            ycbcr->cb = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
            ycbcr->cr = v_metadata;
            break;
        case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M:
            ycbcr->cr = v_metadata;
            break;
        default:
            break;
    }

    return err;
}
