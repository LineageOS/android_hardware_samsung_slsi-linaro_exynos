/*
 * Copyright (C) 2018 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/**
 * @file    Edenruntime.h
 * @brief   This is EDEN Service header.
 * @details This is the EDEN Service header file.
 */

#ifndef VENDOR_SAMSUNG_SLSI_HARDWARE_EDEN_RUNTIME_V1_0_EDENRUNTIME_H
#define VENDOR_SAMSUNG_SLSI_HARDWARE_EDEN_RUNTIME_V1_0_EDENRUNTIME_H

#include <memory>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

#include <vendor/samsung_slsi/hardware/eden_runtime/1.0/IEdenruntime.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace eden_runtime {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using ::android::hidl::base::V1_0::IBase;

using ::vendor::samsung_slsi::hardware::eden_runtime::V1_0::IEdenruntime;

class EdenruntimeDeathRecipient;

struct Edenruntime : public IEdenruntime {
    Edenruntime();
    virtual ~Edenruntime();

    // Methods from ::vendor::samsung_slsi::hardware::eden_runtime::V1_0::IEdenruntime follow.
    Return<uint32_t> init(int32_t pid, const sp<IEdenruntimeCallback>& cb) override;
    Return<void> openModel(int32_t pid, const EdenModelHidl& eModelHidl, const EdenModelOptionsHidl& mOptionsHidl, openModel_cb _hidl_cb) override;
    Return<void> openModelFromMemory(int32_t pid, const EdenModelFromMemoryHidl& eModelfromMemoryHidl, const EdenModelOptionsHidl& mOptionsHidl, openModelFromMemory_cb _hidl_cb) override;
    Return<void> openModelFromFd(int32_t pid, const EdenModelFromFdHidl& eModelfromFdHidl, const EdenModelOptionsHidl& mOptionsHidl, openModelFromFd_cb _hidl_cb) override;
    Return<uint32_t> executeReq(int32_t pid, const EdenRequestHidl& eReqHidl, const sp<IEdenruntimeCallback>& eCallback, const EdenRequestOptionsHidl& rOptionsHidl) override;
    Return<uint32_t> closeModel(int32_t pid, uint32_t modelId) override;
    Return<uint32_t> shutdown(int32_t pid) override;
    Return<void> allocateInputBuffer(int32_t pid, uint32_t modelId, allocateInputBuffer_cb _hidl_cb) override;
    Return<void> allocateOutputBuffer(int32_t pid, uint32_t modelId, allocateOutputBuffer_cb _hidl_cb) override;
    Return<void> loadInputBuffers(int32_t pid, uint32_t modelId, uint32_t numOfBuffers, const hidl_vec<UserBufferHidl>& userBuffersHidl, loadInputBuffers_cb _hidl_cb) override;
    Return<void> loadOutputBuffers(int32_t pid, uint32_t modelId, uint32_t numOfBuffers, const hidl_vec<UserBufferHidl>& userBuffersHidl, loadOutputBuffers_cb _hidl_cb) override;
    Return<void> freeBuffer(int32_t pid, uint32_t modelId, uint64_t bufferAddr, freeBuffer_cb _hidl_cb) override;
    Return<void> getInputBufferShape(int32_t pid, uint32_t modelId, int32_t inputIndex, getInputBufferShape_cb _hidl_cb) override;
    Return<void> getOutputBufferShape(int32_t pid, uint32_t modelId, int32_t outputIndex, getOutputBufferShape_cb _hidl_cb) override;
    Return<uint32_t> notifyDead(int32_t pid, int32_t signo) override;

    Return<void> getEdenVersion(int32_t pid, uint32_t modelId, getEdenVersion_cb _hidl_cb) override;
    Return<void> getCompileVersion(int32_t pid, uint32_t modelId, const EdenModelHidl& eModelHidl, getCompileVersion_cb _hidl_cb) override;
    Return<void> getCompileVersionFromFd(int32_t pid, const EdenModelFromFdHidl& eModelfromFdHidl, getCompileVersionFromFd_cb _hidl_cb) override;
    Return<void> getCompileVersionFromMemory(int32_t pid, const EdenModelFromMemoryHidl& eModelfromMemoryHidl, getCompileVersionFromMemory_cb _hidl_cb) override;

    std::thread aliveMonitor_;

    friend void processAliveMointorMain();

    sp<EdenruntimeDeathRecipient> death_recipient_;

    Return<void> debug(const hidl_handle& fd, const hidl_vec<hidl_string>& options) override;
  private:
    std::function<void(sp<EdenruntimeDeathRecipient>&)> unlink_cb_;
};

// FIXME: most likely delete, this is only for passthrough implementations

}  // namespace implementation
}  // namespace V1_0
}  // namespace eden_runtime
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor

#endif  // VENDOR_SAMSUNG_SLSI_HARDWARE_EDEN_RUNTIME_V1_0_EDENRUNTIME_H

