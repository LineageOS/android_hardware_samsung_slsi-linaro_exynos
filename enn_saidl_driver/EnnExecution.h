/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung
 * Electronics.
 */

/**
 * @file    EnnExecution.h
 * @brief   This file has definitions for AndroidNN IExecution interface
 * @details This file has definitions for AndroidNN IExecution interface
 * @author  nihar.desai/ankit.goel
 */

#ifndef ENN_SL_DRIVER_EXECUTION_H_
#define ENN_SL_DRIVER_EXECUTION_H_

#include <android-base/macros.h>
#include <nnapi/IExecution.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <memory>
#include <utility>
#include <vector>

using namespace ::android::nn;
namespace android {
namespace nn {
namespace enn_driver {

class EnnExecution final : public IExecution { // EnnExecution
    public:
      EnnExecution(SharedPreparedModel preparedModel, Request request,
                   MeasureTiming measure, OptionalDuration loopTimeoutDuration)
          : kPreparedModel(std::move(preparedModel)), kRequest(std::move(request)),
            kMeasure(measure), kLoopTimeoutDuration(loopTimeoutDuration) {
        CHECK(kPreparedModel != nullptr);
      }

      ExecutionResult<std::pair<std::vector<OutputShape>, Timing>>
      compute(const OptionalTimePoint &deadline) const override {
        return kPreparedModel->execute(kRequest, kMeasure, deadline,
                                       kLoopTimeoutDuration, {}, {});
      }

      GeneralResult<std::pair<SyncFence, ExecuteFencedInfoCallback>> computeFenced(
          const std::vector<SyncFence> &waitFor, const OptionalTimePoint &deadline,
          const OptionalDuration &timeoutDurationAfterFence) const override {
        return kPreparedModel->executeFenced(kRequest, waitFor, kMeasure, deadline,
                                             kLoopTimeoutDuration,
                                             timeoutDurationAfterFence, {}, {});
      }

    private:
      const SharedPreparedModel kPreparedModel;
      const Request kRequest;
      const MeasureTiming kMeasure;
      const OptionalDuration kLoopTimeoutDuration;
};

} // namespace enn_driver
} // namespace nn
} // namespace android

#endif
