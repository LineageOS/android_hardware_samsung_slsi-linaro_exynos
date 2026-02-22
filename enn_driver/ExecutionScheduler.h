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

#include <android/hardware/neuralnetworks/1.2/IExecutionCallback.h>
#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc

#include "BufferInfoOnExecute.h"
#include "PriorityQueueWithLock.h"  // PriorityQueueWithLock
#include "CompilerManager.h"

namespace android {
namespace nn {
namespace eden_driver {

class EdenServiceDelegator;
class EnnServiceDelegator;
class EdenPreparedModel;
class SchedulePolicy;

enum class EXECUTION_MODE {
    ASYNC = 0,
    SYNC = 1,
    FENCED = 2,
};

typedef struct __NNTimeStamp {
    std::chrono::system_clock::time_point requestedTime;
    std::chrono::system_clock::time_point startedTime;
    std::chrono::system_clock::time_point completedTime;
    std::chrono::system_clock::time_point respondedTime;
} NNTimeStamp;

typedef struct __NNRequestData {
    int32_t priority;
    EXECUTION_MODE execMode;
    NNTimeStamp timeStamp;

    std::mutex m;
    std::condition_variable cv;

    const EdenPreparedModel* preparedModel;
    const V1_3::Request* request;
    V1_3::IExecutionCallback* callback;
} NNRequestData;

class Compare {
 public:
    bool operator() (const std::shared_ptr<NNRequestData> a, const std::shared_ptr<NNRequestData> b) {
        return  a->priority > b->priority;
    }
};

class ExecutionScheduler {
 public:
    ExecutionScheduler(void);
    ~ExecutionScheduler(void);

    void setEdenServiceDelegator(std::shared_ptr<EdenServiceDelegator> edenServiceDelegator);
    void setEnnServiceDelegator(std::shared_ptr<EnnServiceDelegator> ennServiceDelegator);
    void setCompilerManager(std::shared_ptr<CompilerManager> compilerManager);

    int32_t requestOneExecution(const EdenPreparedModel* edenPreparedModel,
                                const V1_3::Request& request,
                                BufferInfoOnExecute& bufInfoOnExecute,
                                V1_2::MeasureTiming measure,
                                std::chrono::steady_clock::time_point driverStart,
                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                sp<V1_0::IExecutionCallback> cb,
                                EXECUTION_MODE executionMode);

    int32_t requestOneExecution(const EdenPreparedModel* edenPreparedModel,
                                const V1_3::Request& request,
                                BufferInfoOnExecute& bufInfoOnExecute,
                                V1_2::MeasureTiming measure,
                                std::chrono::steady_clock::time_point driverStart,
                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                sp<V1_2::IExecutionCallback> cb,
                                EXECUTION_MODE executionMode);

    int32_t requestOneExecution(const EdenPreparedModel* edenPreparedModel,
                                const V1_3::Request& request,
                                BufferInfoOnExecute& bufInfoOnExecute,
                                V1_2::MeasureTiming measure,
                                std::chrono::steady_clock::time_point driverStart,
                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                sp<V1_3::IExecutionCallback> cb,
                                EXECUTION_MODE executionMode);

    int32_t requestOneExecution(const EdenPreparedModel* edenPreparedModel,
                                const V1_3::Request& request,
                                BufferInfoOnExecute& bufInfoOnExecute,
                                V1_2::MeasureTiming measure,
                                std::chrono::steady_clock::time_point driverStart,
                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                hardware::neuralnetworks::V1_3::IPreparedModel::executeFenced_cb cb,
                                EXECUTION_MODE executionMode);
 private:
    void threadMainForRequestExecution(void);
    void threadMainForRespondExecution(void);

    int32_t preProcessOnInputs(void* addr, int32_t size, int32_t numOfDims, int32_t* dims,
                               DATA_TYPE dataType, HwPreference hwPreference, bool isNNCModel);
    int32_t postProcessOnOutputs(void* addr, int32_t size, int32_t numOfDims, int32_t* dims,
                                 DATA_TYPE dataType, HwPreference hwPreference, bool isNNCModel);

    std::mutex mutex_executeThread;
    std::mutex mutex_responseThread;

    pthread_mutex_t lock_executeThread;
    pthread_mutex_t lock_responseThread;
    pthread_cond_t cond_executeThread;
    pthread_cond_t cond_responseThread;

    std::thread executeThread;
    std::thread responseThread;

    PriorityQueueWithLock<std::shared_ptr<NNRequestData>, Compare> requestQueue_;
    PriorityQueueWithLock<std::shared_ptr<NNRequestData>, Compare> completeQueue_;

    std::shared_ptr<EdenServiceDelegator> edenServiceDelegator_;
    std::shared_ptr<EnnServiceDelegator> ennServiceDelegator_;
    std::shared_ptr<SchedulePolicy> schedulePolicy_;

    template <typename T_Callback>
    friend int32_t requestOneExecutionInAsyncBase(ExecutionScheduler* executionScheduler,
                                                  const EdenPreparedModel* edenPreparedModel,
                                                  const V1_3::Request& request,
                                                  BufferInfoOnExecute& bufInfoOnExecute,
                                                  V1_2::MeasureTiming measure,
                                                  std::chrono::steady_clock::time_point driverStart,
                                                  std::chrono::steady_clock::time_point driverStartAfterFence,
                                                  T_Callback& callback,
                                                  EXECUTION_MODE executionMode);

    std::shared_ptr<CompilerManager> compilerManager_;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_EXECUTIONSCHEDULER_H_

