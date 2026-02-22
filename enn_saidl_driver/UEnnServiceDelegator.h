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
 *          This class is implementing proxy role for Enn Runtime service.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#ifndef DRIVER_ENNSERVICEDELEGATOR_H_
#define DRIVER_ENNSERVICEDELEGATOR_H_

#include <cstdint>  // int32_t

#include "../enn/include/enn_api-type.h"  // EnnModelFile, EdenPreference, ModelTypeInMemory etc

namespace android {
namespace nn {
namespace enn_driver {

class UEnnServiceDelegator {
 public:
    virtual ~UEnnServiceDelegator() {}
    virtual int32_t uennInitialize(void) = 0;
    virtual int32_t uennOpenModel(const char* va, const uint32_t size, EnnModelId *model_id) = 0;
    virtual int32_t uennExecuteModel(const EnnModelId model_id) = 0;
    virtual int32_t uennCloseModel(const EnnModelId model_id) = 0;
    virtual int32_t uennDeinitialize(void) = 0;

    virtual int32_t uennAllocateAllBuffers(const EnnModelId model_id,
                                           EnnBufferPtr **buffer_set,
		                           uint32_t* numOfInputBuffers,
                                           uint32_t* numOfOutputBuffers);

    virtual int32_t uennFreeBuffers(EnnBufferPtr *buffers, const EnnModelId &model_id) = 0;
};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_ENNSERVICEDELEGATOR_H_

