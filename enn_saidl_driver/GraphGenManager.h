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
 * @file    GraphGenManger.h
 * @brief   This is GraphGenManger class file.
 * @details This header defines GraphGenManger class.
 *          This class is implementing handshaking with on-device GraphGen component.
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#ifndef DRIVER_GRAPHGENMANAGER_H_
#define DRIVER_GRAPHGENMANAGER_H_

// #include <vector>
// #include <cstdint>  // int32_t
#include <memory>   // shared_ptr

#include "Common.h"         // DATA_TYPE
#include "EnnDevice.h"
using namespace ::android::nn;

namespace graphgen {
class GraphGen;
}

namespace android {
namespace nn {
namespace enn_driver {

class UEnnServiceDelegator;

typedef struct {
    int8_t* addr = 0;
    int32_t size = 0;
} NNCBuf;

typedef struct {
    bool hasNpuOps = false;
    bool hasDspOps = false;
} NNCOpInfo;

class GraphGenManager {
 public:
    GraphGenManager();
    ~GraphGenManager() {}
    int32_t initGraphGenManager(std::shared_ptr<UEnnServiceDelegator> uennServiceDelegator);
    bool isGraphGenSupported();
    int32_t getSupportedOperations(const Model& model, std::vector<bool> &supportedOpertions);
    int32_t generateNNC(const Model& model, NNCBuf& buffer, NNCOpInfo& nncOpInfo);
    void freeMemory(NNCBuf& buffer);

 private:
    std::shared_ptr<graphgen::GraphGen> graphgen_;
    std::shared_ptr<UEnnServiceDelegator> uennServiceDelegator_;
    bool isSupported = false;
    int32_t socType_;
    std::vector<int32_t> npu_op_list;
};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_GRAPHGENMANAGER_H_
