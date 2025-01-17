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

#include <log/log.h>
#include "GrallocMapperCamera.h"
#include "mali_gralloc_bufferaccess.h"

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
	int ret = mali_gralloc_lock_async(nullptr, bufferHandle, usage,
			accessRegion.left,
			accessRegion.top,
			accessRegion.width,
			accessRegion.height,
			outData,
			acquireFence);

	if (ret)
	{
		AERR("Locking failed with error: %d", ret);
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

	error = mali_gralloc_get_num_flex_planes(nullptr, bufferHandle, &flex.num_planes);
	if (error != GRALLOC1_ERROR_NONE) {
        AERR("Cannot get number of flex planes");
        return Error::BAD_VALUE;
    }

	std::vector<android_flex_plane_t> flexPlanes(flex.num_planes);
    flex.planes = flexPlanes.data();

	error = mali_gralloc_lock_flex_async(nullptr, bufferHandle, usage,
			accessRegion.left,
			accessRegion.top,
			accessRegion.width,
			accessRegion.height,
			&flex,
			acquireFence);

	if (error == GRALLOC1_ERROR_NONE && !toYCbCrLayout(flex, outLayout)) {
        AERR("unable to convert android_flex_layout to YCbCrLayout");
        unlock(bufferHandle);
		return Error::BAD_VALUE;
    }

	return Error::NONE;
}

int Mapper::unlock(buffer_handle_t bufferHandle) const
{
	mali_gralloc_unlock(nullptr, bufferHandle);

	/* Current gralloc does not create release fence for unlock. return -1 */
	return -1;
}

} // namespace GrallocWrapper

} // namespace android
