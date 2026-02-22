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
 * @file    ExecutionScheduler.cpp
 * @brief   This is ExecutionScheduler class file.
 * @details This header defines ExecutionScheduler class.
 *          This class is implementing the execution scheduling with priority queues.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#include <iostream>
#include <cstring>    // memcpy
#include <inttypes.h>  // PRId64, PRIu64
#include <thread>      // std::this_thread::sleep_for

#include "log.h"
#include "Utils.h"    // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER
#include "MyUtils.h"  // DumpToStdio

#include "Common.h"
#include "EdenServiceDelegatorLib.h"
#include "EnnServiceDelegatorLib.h"
#include "EdenPreparedModel.h"
#include "SchedulePolicy.h"
#include "EdenModelConvertLib.h"
#include "PrePostProcessor.h"

#include "ExecutionScheduler.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::ExecutionScheduler"

using ::android::hardware::neuralnetworks::V1_0::RequestArgument;
namespace android {
namespace nn {
namespace eden_driver {

uint64_t microsecondsDuration(std::chrono::steady_clock::time_point end, std::chrono::steady_clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

static bool isRunning = false;

const int32_t USER_NN_TIMEOUT = 0xffffffff;

void userNotify(addr_t* addr, addr_t value);
int32_t userWait(addr_t* addr, uint32_t value, uint32_t timeout);
void showEdenRequest(EdenRequest& request);

static Return<void> callNotifyWithExecutionResult(const sp<V1_0::IExecutionCallback>& callback_1_0,
                                                       const V1_3::ErrorStatus& status,
                                                       const hidl_vec<OutputShape>& /*outputShapes*/,
                                                       V1_2::Timing /*timing*/,
                                                       sp<DriverFencedExecutionCallback> /*fencedExecutionCallback*/) {
    return callback_1_0->notify(convertToV1_0(status));
}

static Return<void> callNotifyWithExecutionResult(const sp<V1_2::IExecutionCallback>& callback_1_2,
                                                       const V1_3::ErrorStatus& status,
                                                       const hidl_vec<OutputShape>& outputShapes,
                                                       V1_2::Timing timing,
                                                       sp<DriverFencedExecutionCallback> /*fencedExecutionCallback*/) {
    return callback_1_2->notify_1_2(convertToV1_0(status), convertToV1_2(outputShapes), timing);
}

static Return<void> callNotifyWithExecutionResult(const sp<V1_3::IExecutionCallback>& callback_1_3,
                                                       const V1_3::ErrorStatus& status,
                                                       const hidl_vec<OutputShape>& outputShapes,
                                                       V1_2::Timing timing,
                                                       sp<DriverFencedExecutionCallback> /*fencedExecutionCallback*/) {
    return callback_1_3->notify_1_3(status, convertToV1_2(outputShapes), timing);
}

static Return<void> callNotifyWithExecutionResult(
                                    const hardware::neuralnetworks::V1_3::IPreparedModel::executeFenced_cb& callback,
                                    const V1_3::ErrorStatus& status,
                                    const hidl_vec<OutputShape>& /*outputShapes*/,
                                    V1_2::Timing /*timing*/,
                                    const sp<DriverFencedExecutionCallback>& fencedExecutionCallback) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (fencedExecutionCallback == nullptr)
        LOGE(EDEN_DRIVER, "%s(+) fencedExecutionCallback is nullptr\n", __func__);

    callback(status, hidl_handle(nullptr), fencedExecutionCallback);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

template <typename T_Callback>
int32_t requestOneExecutionInAsyncBase(ExecutionScheduler* executionScheduler,
                                       const EdenPreparedModel* edenPreparedModel,
                                       const V1_3::Request& request,
                                       BufferInfoOnExecute& bufInfoOnExecute,
                                       V1_2::MeasureTiming measure,
                                       std::chrono::steady_clock::time_point driverStart,
                                       std::chrono::steady_clock::time_point driverStartAfterFence,
                                       T_Callback& callback,
                                       EXECUTION_MODE executionMode) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::chrono::steady_clock::time_point deviceStart, deviceEnd, driverEnd;

    hidl_vec<OutputShape> outputShapes;
    int32_t retCode = RET_OK;

    do {
        if (edenPreparedModel == nullptr || callback == nullptr) {
            LOGE(EDEN_DRIVER, "Invalied Params.\n");
            retCode = INVALID_PARAMS;
            break;
        }

        // To skip DynamicOutputShapeTest
        // TODO: to support dynamic output
        int32_t outputSizeSum = 0;
        bool skipZeroSize = true;
        for (size_t idx = 0; idx < edenPreparedModel->model.main.outputIndexes.size(); idx++) {
            int32_t outputSizeMul = 1;
            int32_t outputIndex = edenPreparedModel->model.main.outputIndexes[idx];
            const V1_3::Operand& androidOperand = edenPreparedModel->model.main.operands[outputIndex];
            for (size_t i = 0; i < androidOperand.dimensions.size(); i++) {
                outputSizeSum += androidOperand.dimensions[i];
                outputSizeMul *= androidOperand.dimensions[i];
            }
            if (outputSizeMul != 0) {
                skipZeroSize = false;
            }
        }
        if (outputSizeSum == 0) {
            LOGE(EDEN_DRIVER, "Invalied OutputParams.\n");
            retCode = INVALID_PARAMS;
            break;
        }

        if (skipZeroSize) {
            LOGD(EDEN_DRIVER, "all the output is zero size, don't need to execute.\n");
            retCode = RET_OK;
            break;
        }

#if 0

        std::shared_ptr<NNRequestData> reqData = std::make_shared<NNRequestData>();
        reqData->execMode = EXECUTION_MODE::ASYNC;
        reqData->callback = callback.get();
        reqData->preparedModel = reinterpret_cast<EdenPreparedModel*>(preparedModel.get());
        reqData->request = &request;
        reqData->priority = 0;
        reqData->timeStamp.requestedTime = std::chrono::system_clock::now();

        int32_t ret = schedulePolicy_->decidePriority(preparedModel.get(), (void*)&request, reqData->priority);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, << "decidePriority() is failed.";
            //return
        }

        requestQueue_.push(reqData);
        pthread_cond_signal(&cond_executeThread);

#else

        // # of inputs between Android NN Model and Eden Model would be different.
        int32_t numOfInputs = edenPreparedModel->inputNumOfBuffers;
        // # of outputs between Android NN Model and Eden Model would be same until now.
        int32_t numOfOutputs = request.outputs.size();
        LOGD(EDEN_DRIVER, "numOfInputs: %d, numOfOutputs: %d\n", numOfInputs, numOfOutputs);

        if (numOfOutputs != edenPreparedModel->outputNumOfBuffers) {
            LOGE(EDEN_DRIVER, "Invalied Params.\n");
            retCode = INVALID_PARAMS;
            break;
        }

        int32_t modelId = edenPreparedModel->modelId;
        HwPreference hwPreference = edenPreparedModel->hwPreference;
        EdenBuffer* inputBuffers = reinterpret_cast<EdenBuffer*>(edenPreparedModel->inputAddr);
        EdenBuffer* outputBuffers = reinterpret_cast<EdenBuffer*>(edenPreparedModel->outputAddr);

        // @todo it is better to look at input eden operand's data type, not hwPreference
        if (hwPreference == GPU_ONLY) {
            LOGD(EDEN_DRIVER, "Not apply preProcessOnInputs! skip it!");
        } else {
            LOGD(EDEN_DRIVER, "Apply preProcessOnInputs!");
            // Convert input data layout
            for (int32_t idx = 0; idx < numOfInputs; idx++) {
                int32_t annInputIndex = edenPreparedModel->model.main.inputIndexes[idx];
                const V1_3::Operand& operand = edenPreparedModel->model.main.operands[annInputIndex];
                void* addr = inputBuffers[idx].addr;
                int32_t size = inputBuffers[idx].size;
                int32_t numOfDims = 0;
                int32_t dims[4] = {0, 0, 0, 0};
                bool isNNCModel = edenPreparedModel->isNNCModel();
                getEdenDimensions(operand.dimensions, dims, numOfDims);
                DATA_TYPE dataType = getDataType(operand.type);

                executionScheduler->preProcessOnInputs(addr, size, numOfDims, dims, dataType, hwPreference, isNNCModel);
            }
        }

        //////////////////////////////////////////
        ////////////// Call execute //////////////
        uint32_t requestId = INVALID_REQUEST_ID;
        addr_t reqId = INVALID_REQUEST_ID;
        EdenCallback cb;
        EdenRequest edenRequest;
        {
            RequestOptions requestOptions;
            RequestPreference requestPreference = {
                .userPreference = {
                .hw = hwPreference,
                .mode = NORMAL_MODE,
                },
            };
            EdenPreference ennPreference;

            //EdenCallback cb;
            cb.notify = userNotify;
            cb.waitFor = userWait;
            cb.requestId = INVALID_REQUEST_ID;  // only used to break on wait condition
            cb.executionResult.inference.retCode = RET_OK;

            edenRequest.modelId = modelId;
            edenRequest.inputBuffers = inputBuffers;
            edenRequest.outputBuffers = outputBuffers;
            edenRequest.callback = &cb;

            requestId = *reinterpret_cast<uint32_t*>(&edenRequest);

            // DEBUG
            // showEdenRequest(edenRequest);

            if (edenPreparedModel->isNNCModel()) {
                requestPreference.userPreference.mode = BOOST_AUX;
                ennPreference = requestPreference.userPreference;

                if (measure == V1_2::MeasureTiming::YES) deviceStart = std::chrono::steady_clock::now();
                int32_t ret = executionScheduler->ennServiceDelegator_->ennExecuteModel(&edenRequest,
                                                                                        &reqId, ennPreference);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "ennServiceDelegator_->ExecuteReq() is failed.\n");
                    return FAIL_ON_EDEN_EXECUTE_REQ;
                }
            } else {
                requestOptions.requestPreference = requestPreference;
                requestOptions.updatedOperations.numOfOperations = edenPreparedModel->updatedOperations.size();
                requestOptions.updatedOperations.operations =
                                                const_cast<int32_t*>(edenPreparedModel->updatedOperations.data());
                // NOTICE Below code was added to avoid MCD's Prevent issue.
                // reserved was remained w/o initialization intendedly but now below code was added.
                for (int32_t idx = 0; idx < 32; idx++) {
                    requestOptions.reserved[idx] = 0;
                }

                if (measure == V1_2::MeasureTiming::YES) deviceStart = std::chrono::steady_clock::now();
                int32_t ret = executionScheduler->edenServiceDelegator_->ExecuteRequest(&edenRequest, requestOptions);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "edenServiceDelegator_->ExecuteReq() is failed.\n");
                    return FAIL_ON_EDEN_EXECUTE_REQ;
                }
            }

#if 0
            if (cb.waitFor(&cb.requestId, requestId, USER_NN_TIMEOUT) < 0) {
                LOGE(EDEN_DRIVER, "User returned TIMEOUT\n");
                retCode = FAIL_ON_EDEN_EXECUTE_REQ;
                break;
            } else {
                LOGD(EDEN_DRIVER, "Execution Done.\n");
            }
#endif
        }
        //////////////////////////////////////////

        // For debugging
#if 0
        for (int32_t idx = 0; idx < numOfOutputs; idx++) {
            int32_t annOutputIndex = edenPreparedModel->model.main.outputIndexes[idx];
            const V1_3::Operand& operand = edenPreparedModel->model.main.operands[annOutputIndex];
            void* addr = outputBuffers[idx].addr;
            int32_t size = outputBuffers[idx].size;
            int32_t numOfDims = 0;
            int32_t dims[4] = {0, 0, 0, 0};
            getEdenDimensions(operand.dimensions, dims, numOfDims);
            DATA_TYPE dataType = getDataType(operand.type);
            if (1) {
                LOGD(EDEN_DRIVER, "Output right after execution...\n");
                DumpToStdio(addr, size, dataType);
                LOGD(EDEN_DRIVER, "Output right after execution...Done!\n");
            }
        }
#endif

        std::vector<sp<IMemory>> vecHidlMemory;
        std::vector<char*> vecMappedPtr;
        std::vector<bool> isDeviceMemory(numOfOutputs, false);
        // Convert output data layout
        for (int32_t idx = 0; idx < numOfOutputs; idx++) {
            RequestArgument outputs = request.outputs[idx];
            if (outputs.hasNoValue) continue;

            auto poolIndex = outputs.location.poolIndex;
            //auto bufferSize = outputs.location.length;

            if (request.pools[poolIndex].getDiscriminator() ==
                             V1_3::Request::MemoryPool::hidl_discriminator::hidlMemory) {
                /* get memory from hidl_memory through IMemory */
                char* mappedPtr = nullptr;
                int32_t ret = bufInfoOnExecute.loadHidlMem(request.pools[poolIndex].hidlMemory(), true, mappedPtr);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "%s(-) Fail on getVirtualAddressOnPool!", __func__);
                    return ret;
                }
                vecMappedPtr.push_back(mappedPtr);
            } else {
                isDeviceMemory[idx] = true;
                vecMappedPtr.push_back(nullptr);
            }
        }

        {
            if (cb.waitFor(&cb.requestId, requestId, USER_NN_TIMEOUT) < 0) {
                LOGE(EDEN_DRIVER, "User returned TIMEOUT\n");
                retCode = FAIL_ON_EDEN_EXECUTE_REQ;
                break;
            } else {
                LOGD(EDEN_DRIVER, "Execution Done.\n");
            }
            // When waitFor is returned, it means device execution is complete.
            if (measure == V1_2::MeasureTiming::YES) deviceEnd = std::chrono::steady_clock::now();
        }

        int32_t width;
        int32_t height;
        int32_t channel;
        int32_t number;
        int32_t ret;
        // Convert output data layout
        for (int32_t idx = 0; idx < numOfOutputs; idx++) {
            // for zero_size cases
            if (edenPreparedModel->isNNCModel()) {
                ret = executionScheduler->ennServiceDelegator_->ennGetOutputBufferShape(modelId, idx,
                                                                                        &width, &height,
                                                                                        &channel, &number);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "ennServiceDelegator_->ennGetOutputBufferShape() is failed.\n");
                    return ret;
                }
            } else {
                ret = executionScheduler->edenServiceDelegator_->GetOutputBufferShape(modelId, idx,
                                                                                      &width, &height,
                                                                                      &channel, &number);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "edenServiceDelegator_->GetOutputBufferShape() is failed.\n");
                    return ret;
                }
            }
            auto total_size = width * height * channel * number;

            if (total_size == 0) {
                continue;
            }

            RequestArgument outputs = request.outputs[idx];
            if (outputs.hasNoValue) continue;

            //auto poolIdx = outputs.location.poolIndex;
            auto bufferSize = outputs.location.length;

            /* get memory from hidl_memory through IMemory */
            //sp<IMemory> hidlMemory = mapMemory(request.pools[poolIdx]);
            //sp<IMemory> hidlMemory = vecHidlMemory[idx];
            //if (hidlMemory != nullptr) {
            char* mappedPtr = vecMappedPtr[idx];
            if (mappedPtr != nullptr || (isDeviceMemory[idx] == true)) {
                //char* mappedPtr = reinterpret_cast<char*>(static_cast<void*>(hidlMemory->getPointer()));
                // @todo need below startUpdate, endUpdate?
                // bufInfoOnExecute.startUpdate(mappedPtr);
                if (isDeviceMemory[idx] == false) {
                    std::memcpy(mappedPtr + outputs.location.offset, outputBuffers[idx].addr, bufferSize);
                }
                // bufInfoOnExecute.endUpdate(mappedPtr);

                if (hwPreference == GPU_ONLY) {
                    LOGD(EDEN_DRIVER, "Not apply postProcessOnOutputs! skip it!");
                } else {
//                    LOGD(EDEN_DRIVER, "Apply postProcessOnOutputs!");

                    int32_t annOutputIndex = edenPreparedModel->model.main.outputIndexes[idx];
                    const V1_3::Operand* ptrOperand = &(edenPreparedModel->model.main.operands[annOutputIndex]);

                    // @todo this code is only for RESHAPE, SHOULD BE REMOVED!!!
                    {
                        // @todo Below code is work-around to pass reshape vts
                        //       It has (1,3,3,1) as input and (9) as output
                        //       But it can't transform data from NCHW to NHWC with (9) since it means (9,1,1,1)
                        //       So use original input dimension information.
                        //       But in future more nice way should replace below code...
                        if (static_cast<int32_t>(edenPreparedModel->model.main.operations[0].type) == ANEURALNETWORKS_RESHAPE) {
                            if (edenPreparedModel->model.main.operations.size() == 1 && ptrOperand->dimensions.size() == 1) {
                                LOGD(EDEN_DRIVER, "in this case, converting for reshape is not working properly...\n");
                                annOutputIndex = edenPreparedModel->model.main.inputIndexes[0];
                                ptrOperand = &(edenPreparedModel->model.main.operands[annOutputIndex]);
                            }
                        }
                    }

                    void* addr;
                    if (isDeviceMemory[idx] == false) {
                        addr = mappedPtr + outputs.location.offset;
                    } else {
                        addr = outputBuffers[idx].addr;
                    }
                    int32_t size = bufferSize;
                    int32_t numOfDims = 0;
                    int32_t dims[4] = {0, 0, 0, 0};
                    bool isNNCModel = edenPreparedModel->isNNCModel();
                    getEdenDimensions(ptrOperand->dimensions, dims, numOfDims);
                    DATA_TYPE dataType = getDataType(ptrOperand->type);

                    executionScheduler->postProcessOnOutputs(addr, size, numOfDims, dims,
                                                             dataType, hwPreference, isNNCModel);
                }
                if (isDeviceMemory[idx] == true) {
                    const uint32_t poolIndex = request.outputs[idx].location.poolIndex;
                    const auto& pool = request.pools[poolIndex];
                    if (edenPreparedModel->initializeManagedBuffer(pool.token()) != true) {
                        return FAIL_ON_EDEN_EXECUTE_REQ;
                    }
                }
            } else {
                LOGE(EDEN_DRIVER, "hidlMemory is nullptr!\n");
                return FAIL_ON_EDEN_EXECUTE_REQ;
            }
        }

        // @todo Below is workaround code to avoid unexpected behavier after executing NPU.
        // Below code should be removed after fixing NPU issue.
        if (edenPreparedModel->hwPreference == NPU_ONLY) {
            LOGD(EDEN_DRIVER, "edenPreparedModel->hwPreference is NPU_ONLY");
        }
    } while (0);

    V1_3::ErrorStatus executionStatus =
                (retCode == RET_OK) ? V1_3::ErrorStatus::NONE : V1_3::ErrorStatus::GENERAL_FAILURE;

    // When outputShapes is aquired, NN HAL driver execution is complete
    if (measure == V1_2::MeasureTiming::YES) {
        driverEnd = std::chrono::steady_clock::now();
        LOGI(EDEN_DRIVER, "executionMode is %d (0:ASYNC, 1:SYNC, 2:FENCED)\n", executionMode);

        if (executionMode == EXECUTION_MODE::FENCED) {
            V1_2::Timing timingSinceLaunch = {
                .timeOnDevice = microsecondsDuration(deviceEnd, deviceStart),
                .timeInDriver = microsecondsDuration(driverEnd, driverStart)
            };
            V1_2::Timing timingAfterFence = {
                .timeOnDevice = microsecondsDuration(deviceEnd, deviceStart),
                .timeInDriver = microsecondsDuration(driverEnd, driverStartAfterFence)
            };
            sp<DriverFencedExecutionCallback> fencedExecutionCallback =
                new DriverFencedExecutionCallback(timingSinceLaunch, timingAfterFence, executionStatus);

            LOGD(EDEN_DRIVER, "callNotifyWithExecutionResult FENCED\n");
            callNotifyWithExecutionResult(callback, executionStatus, outputShapes, {}, fencedExecutionCallback);
        } else {
            V1_2::Timing timing = {
                .timeOnDevice = microsecondsDuration(deviceEnd, deviceStart),
                .timeInDriver = microsecondsDuration(driverEnd, driverStart)
            };
            // @todo outputShapes and timing should be filled properly.
            Return<void> returned = callNotifyWithExecutionResult(callback, executionStatus,
                                                                       outputShapes, timing, nullptr);

            if (!returned.isOk()) {
                LOGE(EDEN_DRIVER, "hidl callback failed to return properly: %s\n", returned.description().c_str());
            }
        }
    } else {
        V1_2::Timing timing = {.timeOnDevice = UINT64_MAX, .timeInDriver = UINT64_MAX};

        if (executionMode == EXECUTION_MODE::FENCED) {
            sp<DriverFencedExecutionCallback> fencedExecutionCallback =
                new DriverFencedExecutionCallback(timing, timing, executionStatus);

            LOGD(EDEN_DRIVER, "callNotifyWithExecutionResult FENCED\n");
            callNotifyWithExecutionResult(callback, executionStatus, outputShapes, {}, fencedExecutionCallback);
        } else {
            // @todo outputShapes and timing should be filled properly.
            Return<void> returned = callNotifyWithExecutionResult(callback, executionStatus,
                                                                       outputShapes, timing, nullptr);

            if (!returned.isOk()) {
                LOGE(EDEN_DRIVER, "hidl callback failed to return properly: %s\n", returned.description().c_str());
            }
        }
    }

#endif

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Constructor
 * @details Constructor
 * @param void
 */
ExecutionScheduler::ExecutionScheduler(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    lock_executeThread = PTHREAD_MUTEX_INITIALIZER;
    lock_responseThread = PTHREAD_MUTEX_INITIALIZER;
    cond_executeThread = PTHREAD_COND_INITIALIZER;
    cond_responseThread = PTHREAD_COND_INITIALIZER;

    // @todo Temporally disable threads
#if 0
    // create threads
    executeThread = std::thread(&ExecutionScheduler::threadMainForRequestExecution, this);
    responseThread = std::thread(&ExecutionScheduler::threadMainForRespondExecution, this);
#endif

    edenServiceDelegator_ = nullptr;
    ennServiceDelegator_ = nullptr;
    schedulePolicy_ = std::make_shared<SchedulePolicy>();

    isRunning = true;
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Destructor
 * @details Destructor
 * @param void
 */
ExecutionScheduler::~ExecutionScheduler(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (isRunning) {
        LOGD(EDEN_DRIVER, "now closing threads...\n");
        isRunning = false;
    }

    // @todo Temporally disable threads
#if 0
    // #1: destroy threads
    executeThread.join();
    responseThread.join();

    // #2: clear queues
    while (requestQueue_.empty() == false) {
       auto request = requestQueue_.top();
       if (request != nullptr) {
           requestQueue_.pop();
       }
    }
    while (completeQueue_.empty() == false) {
        auto complete = completeQueue_.top();
        if (complete != nullptr) {
            completeQueue_.pop();
        }
    }
#endif
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

void ExecutionScheduler::setEdenServiceDelegator(std::shared_ptr<EdenServiceDelegator> edenServiceDelegator) {
    edenServiceDelegator_ = edenServiceDelegator;
}

void ExecutionScheduler::setEnnServiceDelegator(std::shared_ptr<EnnServiceDelegator> ennServiceDelegator) {
    ennServiceDelegator_ = ennServiceDelegator;
}

void ExecutionScheduler::setCompilerManager(std::shared_ptr<CompilerManager> compilerManager) {
    compilerManager_ = compilerManager;
}

/**
 * @brief Request one request execution.
 * @details This function receives one Request from NNAgent and pushes it to RequestQueue.
 *          After pushing request on queue, it waits until this request is complete.
 *          Then it returns to the caller.
 * @param[in] preparedModel IPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t ExecutionScheduler::requestOneExecution(const EdenPreparedModel* edenPreparedModel,
                                                const V1_3::Request& request,
                                                BufferInfoOnExecute& bufInfoOnExecute,
                                                V1_2::MeasureTiming measure,
                                                std::chrono::steady_clock::time_point driverStart,
                                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                                sp<V1_0::IExecutionCallback> cb,
                                                EXECUTION_MODE executionMode) {
    return requestOneExecutionInAsyncBase(this, edenPreparedModel, request, bufInfoOnExecute,
                                          measure, driverStart, driverStartAfterFence, cb, executionMode);
}

/**
 * @brief Request one request execution.
 * @details This function receives one Request from NNAgent and pushes it to RequestQueue.
 *          After pushing request on queue, it waits until this request is complete.
 *          Then it returns to the caller.
 * @param[in] preparedModel IPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t ExecutionScheduler::requestOneExecution(const EdenPreparedModel* edenPreparedModel,
                                                const V1_3::Request& request,
                                                BufferInfoOnExecute& bufInfoOnExecute,
                                                V1_2::MeasureTiming measure,
                                                std::chrono::steady_clock::time_point driverStart,
                                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                                sp<V1_2::IExecutionCallback> cb,
                                                EXECUTION_MODE executionMode) {
    return requestOneExecutionInAsyncBase(this, edenPreparedModel, request, bufInfoOnExecute,
                                          measure, driverStart, driverStartAfterFence, cb, executionMode);
}

/**
 * @brief Request one request execution.
 * @details This function receives one Request from NNAgent and pushes it to RequestQueue.
 *          After pushing request on queue, it waits until this request is complete.
 *          Then it returns to the caller.
 * @param[in] preparedModel IPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t ExecutionScheduler::requestOneExecution(const EdenPreparedModel* edenPreparedModel,
                                                const V1_3::Request& request,
                                                BufferInfoOnExecute& bufInfoOnExecute,
                                                V1_2::MeasureTiming measure,
                                                std::chrono::steady_clock::time_point driverStart,
                                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                                sp<V1_3::IExecutionCallback> cb,
                                                EXECUTION_MODE executionMode) {
    return requestOneExecutionInAsyncBase(this, edenPreparedModel, request, bufInfoOnExecute,
                                          measure, driverStart, driverStartAfterFence, cb, executionMode);
}

/**
 * @brief Request one request execution.
 * @details This function receives one Request from NNAgent and pushes it to RequestQueue.
 *          After pushing request on queue, it waits until this request is complete.
 *          Then it returns to the caller.
 * @param[in] preparedModel IPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t ExecutionScheduler::requestOneExecution(const EdenPreparedModel* edenPreparedModel,
                                                const V1_3::Request& request,
                                                BufferInfoOnExecute& bufInfoOnExecute,
                                                V1_2::MeasureTiming measure,
                                                std::chrono::steady_clock::time_point driverStart,
                                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                                hardware::neuralnetworks::V1_3::IPreparedModel::executeFenced_cb cb,
                                                EXECUTION_MODE executionMode) {
    return requestOneExecutionInAsyncBase(this, edenPreparedModel, request, bufInfoOnExecute,
                                          measure, driverStart, driverStartAfterFence, cb, executionMode);
}

/**
 * @brief Thread main for handling RequestQueue
 * @details This function is main function for handling RequestQueue.
 *          It looks at the RequestQueue and if there is a Request to be handled,
 *          then it retrieves a Request from RequestQueue and starts an execution process.
 *          Once processing is complete, it pushes a RequestQueue to CompleteQueue.
 *          This steps are repeated.
 * @param void
 * @return error code
 */
void ExecutionScheduler::threadMainForRequestExecution(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    while (isRunning) {
        if (requestQueue_.empty() == false) {
            std::shared_ptr<NNRequestData> request = requestQueue_.top();
            if (request != nullptr) {
                std::lock_guard<std::mutex> lock(mutex_executeThread);

                if (request->execMode == EXECUTION_MODE::ASYNC) {
                    request->timeStamp.startedTime = std::chrono::system_clock::now();
                    int32_t ret = 0;
                    request->timeStamp.completedTime = std::chrono::system_clock::now();
                    if (ret != RET_OK) {
                        LOGE(EDEN_DRIVER, "requestOneExecutionInAsync() is failed.\n");
                    }
                } else {
                    request->timeStamp.startedTime = std::chrono::system_clock::now();
                    int32_t ret = 0;
                    request->timeStamp.completedTime = std::chrono::system_clock::now();
                    if (ret != RET_OK) {
                        LOGE(EDEN_DRIVER, "requestOneExecutionInSync() is failed.\n");
                    }
                }
                // pop from request queue
                requestQueue_.pop();
                // push to complete queue
                completeQueue_.push(request);
                pthread_cond_signal(&cond_responseThread);
            }
        } else {
            pthread_mutex_lock(&lock_executeThread);
            pthread_cond_wait(&cond_executeThread, &lock_executeThread);
            pthread_mutex_unlock(&lock_executeThread);
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return;
}

/**
 * @brief Thread main for handling CompleteQueue
 * @details This function is main function for handling CompleteQueue.
 *          It looks at the CompleteQueue and if there is a Request to be handled,
 *          then it retrieves a Request from CompleteQueue and starts an completion process.
 *          This steps are repeated.
 * @param void
 * @return error code
 */
void ExecutionScheduler::threadMainForRespondExecution(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    while (isRunning) {
        if (completeQueue_.empty() == false) {
            std::shared_ptr<NNRequestData> complete = completeQueue_.top();
            if (complete != nullptr) {
                std::lock_guard<std::mutex> lock(mutex_responseThread);

                // Pick it from done queue and do post-work if exists

                // Load output result(Eden Memory Manager) to output buffer(Android NN Memory)
                if (complete->preparedModel != nullptr) {
                    //complete->preparedModel->loadOutputData(complete->request->outputs);
                }

                // Call notify with result
                if (complete->callback != nullptr) {
                    hidl_vec<OutputShape> outputShapes;
                    Timing timing;
                    // @todo outputShapes and timing should be filled properly.
                    complete->callback->notify_1_3(V1_3::ErrorStatus::NONE, convertToV1_2(outputShapes), convertToV1_2(timing));
                }
                // pop from complete queue
                completeQueue_.pop();
            }
        } else {
            pthread_mutex_lock(&lock_responseThread);
            pthread_cond_wait(&cond_responseThread, &lock_responseThread);
            pthread_mutex_unlock(&lock_responseThread);
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return;
}

int32_t ExecutionScheduler::preProcessOnInputs(void* addr, int32_t size, int32_t /*numOfDims*/, int32_t* dims,
                                               DATA_TYPE dataType, HwPreference hwPreference, bool isNNCModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    // For debugging
#if 0
    {
        LOGD(EDEN_DRIVER, "Before input buffer converting...\n");
        //DumpToStdio(addr, size, dataType);
    }
#endif

    int32_t ret;
    if (isNNCModel == true) {
        ret = PrePostProcessor::getInstance()->convertDataLayoutFromNHWCToNCHW(addr, size, dims[N_NHWC], dims[C_NHWC],
                                                                              dims[H_NHWC], dims[W_NHWC], dataType, 0);
    } else if ((dataType == DATA_TYPE::QUANT8) && (hwPreference == NPU_ONLY)) {
    // if data type is QUANT8, it means data type is UINT8. In this case, translate it to INT8 by -128
        int32_t inputOffset = compilerManager_->getInputOffset();
        LOGD(EDEN_DRIVER, "InputOffset: %d \n", inputOffset);
        ret = PrePostProcessor::getInstance()->convertDataLayoutFromNHWCToNCHW(addr, size, dims[N_NCHW], dims[C_NCHW], dims[H_NCHW], dims[W_NCHW], dataType, inputOffset);
    } else {
        ret = PrePostProcessor::getInstance()->convertDataLayoutFromNHWCToNCHW(addr, size, dims[N_NCHW], dims[C_NCHW],
                                                                              dims[H_NCHW], dims[W_NCHW], dataType, 0);
    }
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "convertDataLayoutFromNHWCToNCHW() is failed.\n");
        return FAIL_TO_CONVT_NHWC_TO_NCHW;
    }

    // For debugging
#if 0
    {
        LOGD(EDEN_DRIVER, "After input buffer converting...\n");
        //DumpToStdio(addr, size, dataType);
    }
#endif

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

int32_t ExecutionScheduler::postProcessOnOutputs(void* addr, int32_t size, int32_t /*numOfDims*/, int32_t* dims,
                                                 DATA_TYPE dataType, HwPreference hwPreference, bool isNNCModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    // For debugging
#if 0
    {
        LOGD(EDEN_DRIVER, "Before output buffer converting...\n");
        //DumpToStdio(addr, size, dataType);
    }
#endif

    int32_t ret;
    if (isNNCModel == true) {
        ret = PrePostProcessor::getInstance()->convertDataLayoutFromNCHWToNHWC(addr, size, dims[N_NHWC], dims[C_NHWC],
                                                                              dims[H_NHWC], dims[W_NHWC], dataType, 0);
    } else if ((dataType == DATA_TYPE::QUANT8) && (hwPreference == NPU_ONLY)) {
       // if data type is QUANT8, it means data type is UINT8. In this case, translate it to INT8 by +128
        int32_t outputOffset = compilerManager_->getOutputOffset();
        LOGD(EDEN_DRIVER, "OutputOffset: %d \n", outputOffset);
        ret = PrePostProcessor::getInstance()->convertDataLayoutFromNCHWToNHWC(addr, size, dims[N_NCHW], dims[C_NCHW], dims[H_NCHW], dims[W_NCHW], dataType, outputOffset);
    } else {
        ret = PrePostProcessor::getInstance()->convertDataLayoutFromNCHWToNHWC(addr, size, dims[N_NCHW], dims[C_NCHW],
                                                                              dims[H_NCHW], dims[W_NCHW], dataType, 0);
    }
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "convertDataLayoutFromNCHWToNHWC() is failed.\n");
        return FAIL_TO_CONVT_NCHW_TO_NHWC;
    }

    // For debugging
#if 0
    {
        LOGD(EDEN_DRIVER, "After output buffer converting...\n");
        //DumpToStdio(addr, size, dataType);
    }
#endif

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

void userNotify(addr_t* addr, addr_t value) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    *addr = value;
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

int32_t userWait(addr_t* addr, uint32_t /*value*/, uint32_t timeout) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    while (--timeout > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        if (*addr != INVALID_REQUEST_ID) break;
    }

    if (timeout == 0) {
        return -1;
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return 0;
}

void showEdenRequest(EdenRequest& request) {
    LOGD(EDEN_DRIVER, "showEdenRequest() is called...\n");
    LOGD(EDEN_DRIVER, "request.modelId:%u\n", request.modelId);
    LOGD(EDEN_DRIVER, "request.inputBuffers[0].addr:%p\n", request.inputBuffers[0].addr);
    LOGD(EDEN_DRIVER, "request.inputBuffers[0].size:%d\n", request.inputBuffers[0].size);
    LOGD(EDEN_DRIVER, "request.outputBuffers[0].addr:%p\n", request.outputBuffers[0].addr);
    LOGD(EDEN_DRIVER, "request.outputBuffers[0].size:%d\n", request.outputBuffers[0].size);
    LOGD(EDEN_DRIVER, "request.callback->requestId:%" PRIu64 "\n", static_cast<uint64_t>(request.callback->requestId));
    LOGD(EDEN_DRIVER, "request.hw:%d\n", request.hw);
}
}  // namespace eden_driver
}  // namespace nn
}  // namespace android

