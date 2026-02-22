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
 * @file    EnnServiceDelegatorLib.h
 * @brief   This is EnnServiceDelegatorLib class file.
 * @details This header defines EnnServiceDelegator class.
 *          This class is implementing proxy role for Enn Runtime service.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#ifndef DRIVER_ENNSERVICEDELEGATORLIB_H_
#define DRIVER_ENNSERVICEDELEGATORLIB_H_

#include "UEnnServiceDelegator.h"

namespace android {
namespace nn {
namespace enn_driver {

class UEnnServiceDelegatorLib : public UEnnServiceDelegator {
 public:

    int32_t uennInitialize(void) override;
    int32_t uennOpenModel(const char* va, const uint32_t size, EnnModelId *model_id) override;
    int32_t uennExecuteModel(const EnnModelId model_id) override;
    int32_t uennCloseModel(const EnnModelId model_id) override;
    int32_t uennDeinitialize(void) override;

    int32_t uennAllocateAllBuffers(const EnnModelId model_id,
                                           EnnBufferPtr **buffer_set,
                                           uint32_t* numOfInputBuffers,
                                           uint32_t* numOfOutputBuffers) override;

    int32_t uennFreeBuffers(EnnBufferPtr *buffers, const EnnModelId &model_id) override;


};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_ENNSERVICEDELEGATORLIB_H_

