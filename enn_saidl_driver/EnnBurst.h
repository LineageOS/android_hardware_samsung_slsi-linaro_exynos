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
 * @file    ESLDevice.h
 * @brief   This file has definitions for AndroidNN IDevice interface
 * @details This file has definitions for AndroidNN IDevice interface
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#ifndef ENN_SL_DRIVER_BURST_H_
#define ENN_SL_DRIVER_BURST_H_

#include <nnapi/IBurst.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <optional>
#include <utility>
#include <string>

#include "EnnPreparedModel.h"

using namespace ::android::nn;
namespace android {
namespace nn {
namespace enn_driver {//namespace enn_sl

class EnnBurst final : public IBurst {//ESLBurst
public:
    explicit EnnBurst(std::shared_ptr<const EnnPreparedModel> preparedModel);//ESLBurst

    OptionalCacheHold cacheMemory(const SharedMemory& memory) const override;

    ExecutionResult<std::pair<std::vector<OutputShape>, Timing>> execute(
        const Request& request, MeasureTiming measure, const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& loopTimeoutDuration,
        const std::vector<nn::TokenValuePair>& hints,
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const override;

    GeneralResult<SharedExecution> createReusableExecution(
        const Request& request, MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration,
        const std::vector<nn::TokenValuePair>& hints,
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const override;

    private:
    const std::shared_ptr<const EnnPreparedModel> kPreparedModel;
};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // ENN_SL_DRIVER_BURST_H_

