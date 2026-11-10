/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "GrallocWrapper4"

#include <unistd.h>

#include <log/log.h>
#include <cutils/native_handle.h>
#include <gralloctypes/Gralloc4.h>
#include <android/hardware/graphics/mapper/4.0/IMapper.h>

#include "GrallocWrapper.h"

namespace android {

namespace GrallocWrapper {

namespace V4 = hardware::graphics::mapper::V4_0;
using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::PlaneLayoutComponent;

static constexpr Error kTransactionError = Error::NO_RESOURCES;

static Error toV2Error(V4::Error e)
{
    switch (e) {
    case V4::Error::NONE:          return Error::NONE;
    case V4::Error::BAD_DESCRIPTOR:return Error::BAD_DESCRIPTOR;
    case V4::Error::BAD_BUFFER:    return Error::BAD_BUFFER;
    case V4::Error::BAD_VALUE:     return Error::BAD_VALUE;
    case V4::Error::NO_RESOURCES:  return Error::NO_RESOURCES;
    default:                       return Error::UNSUPPORTED;
    }
}

static bool hasType(const PlaneLayoutComponent& c,
        const aidl::android::hardware::graphics::common::ExtendableType& t)
{
    return c.type.name == t.name && c.type.value == t.value;
}

Mapper::Mapper()
{
    mMapper = V4::IMapper::getService();
    if (mMapper == nullptr || mMapper->isRemote()) {
        LOG_ALWAYS_FATAL("gralloc-mapper must be in passthrough mode");
    }
}

static Error doLock(const sp<V4::IMapper>& mapper, buffer_handle_t bufferHandle,
        uint64_t usage, const IMapper::Rect& accessRegion, int acquireFence,
        void** outData)
{
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    V4::IMapper::Rect rect;
    rect.left = accessRegion.left;
    rect.top = accessRegion.top;
    rect.width = accessRegion.width;
    rect.height = accessRegion.height;

    hardware::hidl_handle acquireFenceHandle;
    NATIVE_HANDLE_DECLARE_STORAGE(acquireFenceStorage, 1, 0);
    if (acquireFence >= 0) {
        auto h = native_handle_init(acquireFenceStorage, 1, 0);
        h->data[0] = acquireFence;
        acquireFenceHandle = h;
    }

    Error error = kTransactionError;
    auto ret = mapper->lock(buffer, usage, rect, acquireFenceHandle,
            [&](const auto& tmpError, const auto& tmpData) {
                error = toV2Error(tmpError);
                if (error != Error::NONE) {
                    return;
                }
                *outData = tmpData;
            });

    // ownership of acquireFence is transferred even on errors
    if (acquireFence >= 0) {
        close(acquireFence);
    }

    return ret.isOk() ? error : kTransactionError;
}

Error Mapper::lock(buffer_handle_t bufferHandle, uint64_t usage,
        const IMapper::Rect& accessRegion,
        int acquireFence, void** outData) const
{
    return doLock(mMapper, bufferHandle, usage, accessRegion, acquireFence, outData);
}

Error Mapper::lock(buffer_handle_t bufferHandle, uint64_t usage,
        const IMapper::Rect& accessRegion,
        int acquireFence, YCbCrLayout* outLayout) const
{
    void* base = nullptr;
    Error error = doLock(mMapper, bufferHandle, usage, accessRegion, acquireFence, &base);
    if (error != Error::NONE) {
        ALOGE("lock(%p) failed with %d", bufferHandle, error);
        return error;
    }

    auto buffer = const_cast<native_handle_t*>(bufferHandle);
    std::vector<PlaneLayout> planes;
    V4::Error v4err = V4::Error::NO_RESOURCES;
    hardware::hidl_vec<uint8_t> raw;
    auto ret = mMapper->get(buffer, gralloc4::MetadataType_PlaneLayouts,
            [&](const auto& tmpError, const auto& tmpVec) {
                v4err = tmpError;
                if (tmpError == V4::Error::NONE) {
                    raw = tmpVec;
                }
            });
    if (!ret.isOk() || v4err != V4::Error::NONE ||
            gralloc4::decodePlaneLayouts(raw, &planes) != OK) {
        ALOGE("failed to get plane layouts for %p (err %d)", bufferHandle, (int)v4err);
        unlock(bufferHandle);
        return Error::BAD_VALUE;
    }

    const PlaneLayout* y = nullptr;
    const PlaneLayout* cb = nullptr;
    const PlaneLayout* cr = nullptr;
    for (const auto& plane : planes) {
        for (const auto& comp : plane.components) {
            if (hasType(comp, gralloc4::PlaneLayoutComponentType_Y)) y = &plane;
            else if (hasType(comp, gralloc4::PlaneLayoutComponentType_CB)) cb = &plane;
            else if (hasType(comp, gralloc4::PlaneLayoutComponentType_CR)) cr = &plane;
        }
    }

    if (!y || !cb || !cr) {
        ALOGE("buffer %p is not YCbCr (%zu planes)", bufferHandle, planes.size());
        unlock(bufferHandle);
        return Error::BAD_VALUE;
    }

    auto* b = static_cast<uint8_t*>(base);
    outLayout->y = b + y->offsetInBytes;
    outLayout->cb = b + cb->offsetInBytes;
    outLayout->cr = b + cr->offsetInBytes;
    outLayout->yStride = static_cast<uint32_t>(y->strideInBytes);
    outLayout->cStride = static_cast<uint32_t>(cb->strideInBytes);
    // pixel step between two chroma samples: sampleIncrementInBits / 8
    outLayout->chromaStep = static_cast<uint32_t>(cb->sampleIncrementInBits / 8);
    if (outLayout->chromaStep == 0) {
        outLayout->chromaStep = 1;
    }

    ALOGV("YCbCr y=%p cb=%p cr=%p ys=%u cs=%u step=%u", outLayout->y, outLayout->cb,
            outLayout->cr, outLayout->yStride, outLayout->cStride, outLayout->chromaStep);
    return Error::NONE;
}

int Mapper::unlock(buffer_handle_t bufferHandle) const
{
    auto buffer = const_cast<native_handle_t*>(bufferHandle);

    int releaseFence = -1;
    V4::Error error = V4::Error::NONE;
    auto ret = mMapper->unlock(buffer,
            [&](const auto& tmpError, const auto& tmpReleaseFence) {
                error = tmpError;
                if (error != V4::Error::NONE) {
                    return;
                }
                auto fenceHandle = tmpReleaseFence.getNativeHandle();
                if (fenceHandle && fenceHandle->numFds == 1) {
                    int fd = dup(fenceHandle->data[0]);
                    if (fd >= 0) {
                        releaseFence = fd;
                    } else {
                        ALOGD("failed to dup unlock release fence");
                    }
                }
            });

    if (!ret.isOk() || error != V4::Error::NONE) {
        ALOGE("unlock(%p) failed with %d", buffer, (int)error);
    }

    return releaseFence;
}

} // namespace GrallocWrapper

} // namespace android
