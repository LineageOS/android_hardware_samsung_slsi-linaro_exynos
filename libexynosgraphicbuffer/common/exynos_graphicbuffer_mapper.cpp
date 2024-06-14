/*
 * Copyright (C) 2024 The LineageOS Project
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

#include <ExynosGraphicBuffer.h>
#include <grallocusage/GrallocUsageConversion.h>
#include <ui/Gralloc.h>
#include <utils/Trace.h>

using namespace android;
using namespace vendor::graphics;

status_t ExynosGraphicBufferMapper::lockAsync(buffer_handle_t handle, uint64_t producerUsage,
                   uint64_t consumerUsage, const Rect& bounds, void** vaddr,
                   int fenceFd, int32_t* outBytesPerPixel,
                   int32_t* outBytesPerStride) {
    ATRACE_CALL();

    const uint64_t usage = static_cast<uint64_t>(
            android_convertGralloc1To0Usage(producerUsage, consumerUsage));
    return getGrallocMapper().lock(handle, usage, bounds, fenceFd, vaddr, outBytesPerPixel,
                         outBytesPerStride);
}
