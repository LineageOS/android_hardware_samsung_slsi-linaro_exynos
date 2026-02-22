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
 * @file    BufferInfoOnExecute.cpp
 * @brief   This is BufferInfoOnExecute class file.
 * @details This header implements BufferInfoOnExecute class.
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#include <sys/mman.h>  // munmap
#include <nnapi/SharedMemory.h>

#include "BufferInfoOnExecute.h"
#include "log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EnnDriver::BufferInfoOnExecute"

namespace android {
namespace nn {
namespace enn_driver {

/**
 * @brief destructor
 * @details
 * @param[in] void
 * @returns N/A
 */
BufferInfoOnExecute::~BufferInfoOnExecute() {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    for (VirtualAddressInfo& vInfo:vecVirtualAddressInfo) {
        if(vInfo.type == MMAP_FD_1) {
           delete[] vInfo.addr;
           LOGD(ENN_DRIVER, "Delete Vitual Address..Done!\n");
	} else if (vInfo.type == MMAP_FD_2 || vInfo.type == ASHMEM) {
           munmap(vInfo.addr, vInfo.size);
           LOGD(ENN_DRIVER, "Unmap Vitual Address..Done!\n");
        }
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Load Shared Memory from Fd/AShMem/Haredware buffer
 * @details
 * @param[in] memory SharedMemory
 * @param[in] needToWrite
 * @param[out] virtAddr address of Shared Memory pointer
 * @returns return code
 */
int32_t BufferInfoOnExecute::loadSharedMem(const SharedMemory& memory, bool needToWrite, char*& virtAddr, uint32_t& size) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    VirtualAddressInfo vInfo;
    auto mapping = android::nn::map(memory);
    if (!mapping.has_value()) {
        LOGE(ENN_DRIVER, "Can't map shared memory: ");
        return INVALID_PARAMS;
    }
    int32_t ret = getVirtualAddressOnPool(memory, needToWrite, vInfo, std::move(mapping).value());

    if (ret != RET_OK) {
        LOGE(ENN_DRIVER, "%s(-) Fail on getVirtualAddressOnPool!", __func__);
        virtAddr = nullptr;
        return ret;
    } else {
        vecVirtualAddressInfo.push_back(vInfo);
    }

    virtAddr = vInfo.addr;
    size = vInfo.size;
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}


}  // namespace enn_driver
}  // namespace nn
}  // namespace android

