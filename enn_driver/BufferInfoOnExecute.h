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
 * @file    BufferInfoOnExecute.h
 * @brief   This is BufferInfoOnExecute class file.
 * @details This header implements BufferInfoOnExecute class.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_BUFFERINFOONEXECUTE_H_
#define DRIVER_BUFFERINFOONEXECUTE_H_

#include <map>
#include <vector>
#include <cstdint>  // int32_t

#include "HalInterfaces.h"        // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
#include "EdenModelConvertLib.h"  // VirtualAddressInfo

namespace android {
namespace nn {
namespace eden_driver {

class BufferInfoOnExecute {
 public:
    explicit BufferInfoOnExecute() {}
    ~BufferInfoOnExecute();

    int32_t loadHidlMem(const hidl_memory& memory, bool needToWrite, char*& virtAddr);
    void startUpdate(char* virtAddr);
    void endUpdate(char* virtAddr);

 private:
    // Buffer information on execution
    std::vector<VirtualAddressInfo> vecVirtualAddressInfo;
    std::vector<sp<IMemory>> vecIMemoryOnAshmems;

    std::map<char*, sp<IMemory>> mapMappedAddrToIMemory;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_BUFFERINFOONEXECUTE_H_

