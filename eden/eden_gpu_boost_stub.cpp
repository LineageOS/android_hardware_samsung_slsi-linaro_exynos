/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/**
 * @file    eden_gpu_boost_stub.cpp
 * @brief   This is EDEN GPU boost stub
 * @details This file invokes GPU Boost service.
 * @version 1.0 Basic scenario support.
 */

#include "eden_gpu_boost_stub.h"

#include <cutils/native_handle.h>

#include <hidl/LegacySupport.h>
#include <hidl/Status.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <vendor/samsung_slsi/hardware/enn_aux/1.0/IENN_AUX.h>

namespace eden {
namespace rt {

using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::Void;

using namespace vendor::samsung_slsi::hardware::enn_aux;
using namespace vendor::samsung_slsi::hardware::enn_aux::V1_0;
using android::sp;

using ::vendor::samsung_slsi::hardware::enn_aux::V1_0::IENN_AUX;
using ::android::hidl::base::V1_0::IBase;

int RunGpuBoost(void) {
    pid_t stub_pid = getpid();
    sp<IENN_AUX> gpu_boost_service;

    int error = 0;
    gpu_boost_service = IENN_AUX::getService();
    if (gpu_boost_service != nullptr) {
        auto ret = gpu_boost_service->_apply_GPU_boost(stub_pid);
        if (!ret.isOk())
            error = -1;
    } else {
        error = -2;
    }
    return error;
}

} // namespace rt
} // namespace eden
