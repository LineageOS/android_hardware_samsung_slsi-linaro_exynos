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
 * @file    ExecutionScheduler.h
 * @brief   This is ExecutionScheduler class file.
 * @details This header defines ExecutionScheduler class.
 *          This class is implementing the execution scheduling with priority queues.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#ifndef DRIVER_EXECUTIONSCHEDULER_H_
#define DRIVER_EXECUTIONSCHEDULER_H_

#include <condition_variable>
#include <thread>
#include <cstdint>  // int32_t


//#include "BufferInfoOnExecute.h"

namespace android {
namespace nn {
namespace enn_driver {

class UEnnServiceDelegator;
class EnnPreparedModel;

enum class EXECUTION_MODE {
    ASYNC = 0,
    SYNC = 1,
    FENCED = 2,
};

class ExecutionScheduler {
 public:
    ExecutionScheduler(void);
    ~ExecutionScheduler(void);

    void setUEnnServiceDelegator(std::shared_ptr<UEnnServiceDelegator> uennServiceDelegator);
    std::shared_ptr<UEnnServiceDelegator> getUEnnServiceDelegator() {return uennServiceDelegator_;}

    int32_t requestOneExecution(const EnnPreparedModel* ennPreparedModel,
                                const Request& request,
                                BufferInfoOnExecute& bufInfoOnExecute,
                                MeasureTiming measure,
                                std::chrono::steady_clock::time_point driverStart,
                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                EXECUTION_MODE executionMode,
				std::vector<OutputShape>& outputShapes,
				Timing& timing);

//ToDO: nihar.desai Update when Sync Fence implemented
#if 0
    int32_t requestOneExecution(const EnnPreparedModel* ennPreparedModel,
                                const Request& request,
                                BufferInfoOnExecute& bufInfoOnExecute,
                                MeasureTiming measure,
                                std::chrono::steady_clock::time_point driverStart,
                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                hardware::neuralnetworks::IPreparedModel::executeFenced_cb cb,
                                EXECUTION_MODE executionMode);
#endif
 private:
    std::shared_ptr<UEnnServiceDelegator> uennServiceDelegator_;

    friend int32_t requestOneExecutionInAsyncBase(ExecutionScheduler* executionScheduler,
                                                  const EnnPreparedModel* ennPreparedModel,
                                                  const Request& request,
                                                  BufferInfoOnExecute& bufInfoOnExecute,
                                                  MeasureTiming measure,
                                                  std::chrono::steady_clock::time_point driverStart,
                                                  std::chrono::steady_clock::time_point driverStartAfterFence,
                                                  EXECUTION_MODE executionMode,
						  std::vector<OutputShape>& outputShapes,
						  Timing& timing);

};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_EXECUTIONSCHEDULER_H_

