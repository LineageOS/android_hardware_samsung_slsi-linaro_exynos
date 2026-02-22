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
 * @file    ModelUtils.h
 * @brief   This file defines model utility functions
 * @details This header defines ModelConverter class.
 *          This header file contains NNC Model related utilities.
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#ifndef DRIVER_MODELUTILS_H_
#define DRIVER_MODELUTILS_H_

#include <vector>
#include <set>
#include <cstdint>  // int32_t
#include <nnapi/SharedMemory.h>
#include "GraphGenManager.h"

#include "../enn/include/enn_api-type.h"

//using namespace enn::nn;
using namespace ::android::nn;
namespace android {
namespace nn {
namespace enn_driver {

class CompilerManager;
class GraphGenManager;

enum {
    TARGET_DEVICE_NPU = 0,
    TARGET_DEVICE_GPU = 1,
    TARGET_DEVICE_CPU = 2,
};

enum shared_mem_type {
    ASHMEM,
    MMAP_FD_1,
    MMAP_FD_2,
    AHWB_BLOB,
};

typedef struct __VirtualAddressInfo {
    shared_mem_type type;  // 0:(ashmem), 1,2:(mmap_fd), 3: (hardware_buffer_blob)
    char* addr;
    uint32_t size;
} VirtualAddressInfo;

class ModelConverter {
 public:
    void setGraphGenManager(std::shared_ptr<GraphGenManager> graphgenManager);
    int32_t convertToNNC(const Model& model, NNCBuf& nncBuffer, NNCOpInfo& nncOpInfo);

 private:
    std::shared_ptr<GraphGenManager> graphgenManager_;
};

int32_t getVirtualAddressOnPool(const SharedMemory& memory, bool needToWrite, VirtualAddressInfo& vInfo,
                                android::nn::Mapping mapping) ;

bool verifyDimension(const Request& request, const Model& model);
}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_MODELUTILS_H_
