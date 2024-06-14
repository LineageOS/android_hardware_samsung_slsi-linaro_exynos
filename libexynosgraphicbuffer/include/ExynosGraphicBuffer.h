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

#ifndef EXYNOS_GRAPHIC_BUFFER_H_
#define EXYNOS_GRAPHIC_BUFFER_H_

#include "ExynosGraphicBufferCore.h"
#include <cstdint>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicTypes.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <utils/Trace.h>

/* Gralloc1 usages enum is provided here to ensure backward compatibility
 * This enum will be deprecated in Android S so please consider switching to
 * BufferUsage::<USAGE> found in
 * hardware/interfaces/1.0(or 1.1, 1.2)/types.hal
 */
#include <hardware/gralloc1.h>

namespace vendor {
namespace graphics {

/* Android default usages */
typedef android::hardware::graphics::common::V1_2::BufferUsage BufferUsage;

/* Mapper extension class to allow locking with 64-bit usages */
class ExynosGraphicBufferMapper : public android::GraphicBufferMapper {
  public:
    static inline ExynosGraphicBufferMapper &get() {
        return static_cast<ExynosGraphicBufferMapper &>(getInstance());
    }

    android::status_t lockAsync(buffer_handle_t handle, uint64_t producerUsage,
                       uint64_t consumerUsage, const android::Rect& bounds, void** vaddr,
                       int fenceFd, int32_t* outBytesPerPixel = nullptr,
                       int32_t* outBytesPerStride = nullptr);

    android::status_t lock64(buffer_handle_t handle, uint64_t usage, const android::Rect &bounds, void **vaddr,
                             int32_t *outBytesPerPixel = nullptr, int32_t *outBytesPerStride = nullptr);

    android::status_t lockYCbCr64(buffer_handle_t handle, uint64_t usage, const android::Rect &bounds,
                                  android_ycbcr *ycbcr);
};

typedef class android::GraphicBufferAllocator ExynosGraphicBufferAllocator;

/* libion helper for use by OMX only */
namespace ion {

typedef int ion_user_handle_t;

int get_ion_fd();

} /* namespace ion */

} /* namespace graphics */
} /* namespace vendor */

#endif /* EXYNOS_GRAPHIC_BUFFER_H_ */
