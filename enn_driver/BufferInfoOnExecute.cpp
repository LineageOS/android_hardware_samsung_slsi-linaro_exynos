/*
 * Copyright (C) 2019 Samsung Electronics Co. LTD
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
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#include <sys/mman.h>  // munmap

#include "BufferInfoOnExecute.h"
#include "EdenModelConvertLib.h"

#include "log.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::BufferInfoOnExecute"

namespace android {
namespace nn {
namespace eden_driver {

/**
 * @brief destructor
 * @details
 * @param[in] void
 * @returns N/A
 */
BufferInfoOnExecute::~BufferInfoOnExecute() {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    for (VirtualAddressInfo& vInfo : vecVirtualAddressInfo) {
        if (vInfo.type == MMAP_FD_1) {
            LOGD(EDEN_DRIVER, "call delete[] on virtual address...\n");

            delete[] vInfo.addr;

            LOGD(EDEN_DRIVER, "call delete[] on virtual address...Done!\n");
        } else if (vInfo.type == MMAP_FD_2) {
            LOGD(EDEN_DRIVER, "Unmap on virtual address...\n");

            munmap(vInfo.addr, vInfo.size);

            LOGD(EDEN_DRIVER, "Unmap on virtual address...Done!\n");
        } else if (vInfo.type == AHWB_BLOB) {
            LOGD(EDEN_DRIVER, "Release Hardware buffer ...\n");
            AHardwareBuffer_unlock(vInfo.hwBuffer, nullptr);
            AHardwareBuffer_release(vInfo.hwBuffer);
            LOGD(EDEN_DRIVER, "Release Hardware buffer ...Done!\n");
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief
 * @details
 * @param[in] memory
 * @param[out] virtAddr
 * @returns return code
 */
int32_t BufferInfoOnExecute::loadHidlMem(const hardware::hidl_memory& memory, bool needToWrite, char*& virtAddr) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    VirtualAddressInfo vInfo;
    sp<IMemory> spIMemoryOnAshmem;
    int32_t ret = getVirtualAddressOnPool(memory, needToWrite, vInfo, spIMemoryOnAshmem);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) Fail on getVirtualAddressOnPool!", __func__);
        virtAddr = nullptr;
        return ret;
    } else {
        if (vInfo.type == ASHMEM) {
            vecIMemoryOnAshmems.push_back(spIMemoryOnAshmem);
            mapMappedAddrToIMemory[vInfo.addr] = spIMemoryOnAshmem;
        } else /* if (vInfo.type == MMAP_FD_1) */ {
            vecVirtualAddressInfo.push_back(vInfo);
        }
    }

    virtAddr = vInfo.addr;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief
 * @details
 * @param[in] virtAddr
 * @returns void
 */
void BufferInfoOnExecute::startUpdate(char* virtAddr) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    auto it = mapMappedAddrToIMemory.find(virtAddr);
    if (it != mapMappedAddrToIMemory.end()) {
        sp<IMemory> mappedMemory = it->second;
        mappedMemory->update();
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief
 * @details
 * @param[in] virtAddr
 * @returns void
 */
void BufferInfoOnExecute::endUpdate(char* virtAddr) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    auto it = mapMappedAddrToIMemory.find(virtAddr);
    if (it != mapMappedAddrToIMemory.end()) {
        sp<IMemory> mappedMemory = it->second;
        mappedMemory->commit();
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

