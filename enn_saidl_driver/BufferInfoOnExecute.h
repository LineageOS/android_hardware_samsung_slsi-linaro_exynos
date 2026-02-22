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

#ifndef ENNDRIVER_BUFFERINFOONEXECUTE_H_
#define ENNDRIVER_BUFFERINFOONEXECUTE_H_

#include <map>
#include <vector>
#include <cstdint>  // int32_t

#include "ModelUtils.h"  // VirtualAddressInfo

namespace android {
namespace nn {
namespace enn_driver {

class BufferInfoOnExecute {
 public:
    explicit BufferInfoOnExecute() {}
    ~BufferInfoOnExecute();

    int32_t loadSharedMem(const SharedMemory& memory, bool needToWrite, char*& virtAddr, uint32_t& size);

 private:
    // Buffer information on execution
    std::vector<VirtualAddressInfo> vecVirtualAddressInfo;
};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // ENNDRIVER_BUFFERINFOONEXECUTE_H_

