/*
 * Copyright (C) 2020 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/**
 * @file    EnnServiceDelegator.h
 * @brief   This is EnnServiceDelegator class file.
 * @details This header defines EnnServiceDelegator class.
 *          This class is implementing proxy role for Eden Runtime service.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#ifndef DRIVER_ENNSERVICEDELEGATOR_H_
#define DRIVER_ENNSERVICEDELEGATOR_H_

#include <cstdint>  // int32_t

#include "../include/eden_types.h"  // EdenModelFile, EdenPreference, ModelTypeInMemory etc

namespace android {
namespace nn {
namespace eden_driver {

class EnnServiceDelegator {
 public:
    virtual ~EnnServiceDelegator() {}
    virtual int32_t ennInitialize(void) = 0;
    virtual int32_t ennOpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                                        bool encrypted, uint32_t* modelId, EdenModelOptions& options) = 0;
    virtual int32_t ennExecuteModel(EdenRequest* request, addr_t* requestId, EdenPreference preference) = 0;
    virtual int32_t ennCloseModel(uint32_t modelId) = 0;
    virtual int32_t ennShutdown(void) = 0;

    virtual int32_t ennAllocateInputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) = 0;
    virtual int32_t ennAllocateOutputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) = 0;
    virtual int32_t ennFreeBuffers(uint32_t modelId, EdenBuffer* buffers) = 0;
//    virtual int32_t ennGetState(EdenState* state) = 0;
    virtual int32_t ennGetInputBufferShape(uint32_t modelId, int32_t inputIndex,
                                        int32_t* width, int32_t* height, int32_t* channel, int32_t* number) = 0;
    virtual int32_t ennGetOutputBufferShape(uint32_t modelId, int32_t outputIndex,
                                         int32_t* width, int32_t* height, int32_t* channel, int32_t* number) = 0;
    virtual int32_t ennGetEdenVersion(uint32_t modelId, int32_t* versions) = 0;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_ENNSERVICEDELEGATOR_H_

