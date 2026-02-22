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

#include <EnnBurst.h>//ESLBurst.h

#include <DefaultExecution.h>
#include <android-base/logging.h>
#include <nnapi/IBurst.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#ifdef GPU_HW
#define LOG_TAG "EnnDriver::GPU" //ENN_SL
#elif NPU_HW
#define LOG_TAG "EnnDriver::NPU" //ENN_SL
#endif

using namespace ::android::nn;

namespace android {
namespace nn {
namespace enn_driver {//enn_sl

EnnBurst::EnnBurst(std::shared_ptr<const EnnPreparedModel> preparedModel)//ESLBurst
    : kPreparedModel(std::move(preparedModel)) {
    CHECK(kPreparedModel != nullptr);
}

EnnBurst::OptionalCacheHold EnnBurst::cacheMemory(const SharedMemory& /*memory*/) const {
    return nullptr;
}

ExecutionResult<std::pair<std::vector<OutputShape>, Timing>> EnnBurst::execute(
        const Request& request, MeasureTiming measure, const nn::OptionalTimePoint& deadline,
        const nn::OptionalDuration& loopTimeoutDuration,
        const std::vector<nn::TokenValuePair>& hints,
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const {
    return kPreparedModel->execute(request, measure, deadline, loopTimeoutDuration, hints, extensionNameToPrefix);
}

GeneralResult<SharedExecution> EnnBurst::createReusableExecution(
        const Request& request, MeasureTiming measure,
        const nn::OptionalDuration& loopTimeoutDuration,
        const std::vector<nn::TokenValuePair>& hints,
        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const {
    return std::make_shared<DefaultExecution>(kPreparedModel, request, measure,
                                              loopTimeoutDuration);
}


}  // namespace enn_sl
}  // namespace nn
}  // namespace android


