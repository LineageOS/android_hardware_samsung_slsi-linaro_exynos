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
#include <variant>
#include <vector>
#include "log.h"

#include "Common.h"
#include "UEnnServiceDelegatorLib.h"
#include "EnnPreparedModel.h"
#include "ModelUtils.h"
#include "ExecutionScheduler.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EnnDriver::ExecutionScheduler"

namespace android {
namespace nn {
namespace enn_driver {

OptionalDuration  microsecondsDuration(std::chrono::steady_clock::time_point end, std::chrono::steady_clock::time_point start) {
    return end - start;
}


void userNotify(addr_t* addr, addr_t value);
int32_t userWait(addr_t* addr, uint32_t value, uint32_t timeout);

/**
 * @brief request execution.
 * @details
 * @param[in] ExecutionScheduler ExecutionScheduler instance
 * @param[in] preparedModel IPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] BufferInfoOnExecute
 * @param[in] MeasureTiming measure execution Time
 * @param[in] time_point start time when execution request srtarts in driver
 * @param[in] time_point driverStartAfterFence
 * @param[in] executionMode Mode of Execution SYNC/ASYNC/Fenced/Burst
 * @param[out] OutputShape shape of Output returned by Enn Framework
 * @param[out] Timing total execution Time
 * @return error code
*/
int32_t requestOneExecutionInAsyncBase(ExecutionScheduler* executionScheduler,
                                       const EnnPreparedModel* ennPreparedModel,
                                       const Request& request,
                                       BufferInfoOnExecute& bufInfoOnExecute,
                                       MeasureTiming measure,
                                       std::chrono::steady_clock::time_point driverStart,
                                       std::chrono::steady_clock::time_point driverStartAfterFence,
                                       EXECUTION_MODE executionMode,
				       std::vector<OutputShape>& outputShapes,
				       Timing& timing) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    std::chrono::steady_clock::time_point deviceStart, deviceEnd, driverEnd;

    int32_t retCode = RET_OK;

    do {
        if (ennPreparedModel == nullptr) {
            LOGE(ENN_DRIVER, "Invalied Params.\n");
            retCode = INVALID_PARAMS;
            break;
        }

        // To skip DynamicOutputShapeTest
        // TODO: to support dynamic output
        int32_t outputSizeSum = 0;
        bool skipZeroSize = true;
        for (size_t idx = 0; idx < ennPreparedModel->model.main.outputIndexes.size(); idx++) {
            int32_t outputSizeMul = 1;
            int32_t outputIndex = ennPreparedModel->model.main.outputIndexes[idx];
            const Operand& androidOperand = ennPreparedModel->model.main.operands[outputIndex];
            for (size_t i = 0; i < androidOperand.dimensions.size(); i++) {
                outputSizeSum += androidOperand.dimensions[i];
                outputSizeMul *= androidOperand.dimensions[i];
            }
            if (outputSizeMul != 0) {
                skipZeroSize = false;
            }
        }
        if (outputSizeSum == 0) {
            LOGE(ENN_DRIVER, "Invalid OutputParams.\n");
            retCode = OUTPUT_FULLY_NOT_SPECIFIED;
            break;
        }

        if (skipZeroSize) {
            LOGD(ENN_DRIVER, "all the output is zero size, don't need to execute.\n");
            retCode = RET_OK;
            break;
        }

        // # of inputs between Android NN Model and Enn Model would be different.
        int32_t numOfInputs = ennPreparedModel->inputNumOfBuffers;
        // # of outputs between Android NN Model and Enn Model would be same until now.
        int32_t numOfOutputs = request.outputs.size();
        LOGD(ENN_DRIVER, "numOfInputs: %d, numOfOutputs: %d\n", numOfInputs, numOfOutputs);

        if (numOfOutputs != ennPreparedModel->outputNumOfBuffers) {
            LOGE(ENN_DRIVER, "Invalied Params.\n");
            retCode = INVALID_PARAMS;
            break;
        }

        const EnnModelId modelId = ennPreparedModel->modelId;
        HwType hwPreference = ennPreparedModel->hwPreference;
        EnnBufferPtr* inputBuffers = reinterpret_cast<EnnBufferPtr*>(ennPreparedModel->inputAddr);
        EnnBufferPtr* outputBuffers = reinterpret_cast<EnnBufferPtr*>(ennPreparedModel->outputAddr);

        // @todo it is better to look at input enn operand's data type, not hwPreference
        if (hwPreference == CPU_GPU) {
               LOGD(ENN_DRIVER, "Not apply preProcessOnInputs! skip it!");
        } else {
              LOGD(ENN_DRIVER, "Not Apply preProcessOnInputs!");
	}

        //////////////////////////////////////////
        ////////////// Call execute //////////////
        {
            if (measure == MeasureTiming::YES) deviceStart = std::chrono::steady_clock::now();
            const EnnModelId model_id = ennPreparedModel->modelId;

            int32_t ret = executionScheduler->getUEnnServiceDelegator()->uennExecuteModel(model_id);
            if (ret != RET_OK) {
                LOGE(ENN_DRIVER, "ennServiceDelegator_->ExecuteReq() is failed.\n");
                return FAIL_ON_ENN_EXECUTE_REQ;
            }
        }
        std::vector<char*> vecMappedPtr;
        std::vector<bool> isDeviceMemory(numOfOutputs, false);
        // Convert output data layout
        for (int32_t idx = 0; idx < numOfOutputs; idx++) {
            Request::Argument outputs = request.outputs[idx];

            auto poolIndex = outputs.location.poolIndex;
            //auto bufferSize = outputs.location.length;

            if (const auto* sharedMemory = std::get_if<SharedMemory>(&request.pools[poolIndex])) {
                /* get memory from shared memory */
                char* mappedPtr = nullptr;
		uint32_t msize;
                int32_t ret = bufInfoOnExecute.loadSharedMem(*sharedMemory, true, mappedPtr, msize);
                if (ret != RET_OK) {
                    LOGE(ENN_DRIVER, "%s(-) Fail on getVirtualAddressOnPool!", __func__);
                    return ret;
                }
                vecMappedPtr.push_back(mappedPtr);
            } else {
                isDeviceMemory[idx] = true;
                vecMappedPtr.push_back(nullptr);
            }
        }

        LOGD(ENN_DRIVER, "Execution Done.\n");

        if (measure == MeasureTiming::YES) deviceEnd = std::chrono::steady_clock::now();

        int32_t width;
        int32_t height;
        int32_t channel;
        int32_t number;
        int32_t ret;
        // Convert output data layout
        for (int32_t idx = 0; idx < numOfOutputs; idx++) {

            Request::Argument outputs = request.outputs[idx];

            //auto poolIdx = outputs.location.poolIndex;
            auto bufferSize = outputs.location.length;

            char* mappedPtr = vecMappedPtr[idx];
            if (mappedPtr != nullptr || (isDeviceMemory[idx] == true)) {
                if (mappedPtr != nullptr) {
                    LOGD(ENN_DRIVER, "Loading data from %p to %p \n", outputBuffers[idx]->va , mappedPtr + outputs.location.offset);
                    std::memcpy(mappedPtr + outputs.location.offset, outputBuffers[idx]->va, bufferSize);
                }

                if (hwPreference == CPU_GPU) {
                    LOGD(ENN_DRIVER, "Not apply postProcessOnOutputs! skip it!");
                } else {
                    LOGD(ENN_DRIVER, " Not Apply postProcessOnOutputs!");
                }
                if (isDeviceMemory[idx] == true) {
                    const uint32_t poolIndex = request.outputs[idx].location.poolIndex;
                    const auto& pool = request.pools[poolIndex];
		    const auto* isToken = std::get_if<Request::MemoryDomainToken>(&pool);
                    if (ennPreparedModel->initializeManagedBuffer(*isToken) != true) {
                        return FAIL_ON_ENN_EXECUTE_REQ;
                    }
                }
            } else {
                LOGE(ENN_DRIVER, "Memory is nullptr!\n");
                return FAIL_ON_ENN_EXECUTE_REQ;
            }
        }

        // @todo Below is workaround code to avoid unexpected behavier after executing NPU.
        // Below code should be removed after fixing NPU issue.
        if (ennPreparedModel->hwPreference == NPU_DSP) {
            LOGD(ENN_DRIVER, "ennPreparedModel->hwPreference is NPU_ONLY");
        }
    } while (0);

    // When outputShapes is aquired, NN HAL driver execution is complete
    if (measure == MeasureTiming::YES) {
        driverEnd = std::chrono::steady_clock::now();
        LOGI(ENN_DRIVER, "executionMode is %d (0:ASYNC, 1:SYNC, 2:FENCED)\n", executionMode);

        Timing timing = {
                .timeOnDevice = microsecondsDuration(deviceEnd, deviceStart),
                .timeInDriver = microsecondsDuration(driverEnd, driverStart)
        };
        return retCode;
    } else {
        return retCode;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Constructor
 * @details Constructor
 * @param void
 */
ExecutionScheduler::ExecutionScheduler(void) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    uennServiceDelegator_ = nullptr;

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Destructor
 * @details Destructor
 * @param void
 */
ExecutionScheduler::~ExecutionScheduler(void) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

void ExecutionScheduler::setUEnnServiceDelegator(std::shared_ptr<UEnnServiceDelegator> uennServiceDelegator) {
    uennServiceDelegator_ = uennServiceDelegator;
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
int32_t ExecutionScheduler::requestOneExecution(const EnnPreparedModel* ennPreparedModel,
                                                const Request& request,
                                                BufferInfoOnExecute& bufInfoOnExecute,
                                                MeasureTiming measure,
                                                std::chrono::steady_clock::time_point driverStart,
                                                std::chrono::steady_clock::time_point driverStartAfterFence,
                                                EXECUTION_MODE executionMode,
						std::vector<OutputShape>& outputshape, Timing& timing) {
    return requestOneExecutionInAsyncBase(this, ennPreparedModel, request, bufInfoOnExecute,
                                          measure, driverStart, driverStartAfterFence,  executionMode, outputshape,timing);
}

#if 0
//@ToDo: nihar.desai check if below functions are needed in Syncd Fence
void userNotify(addr_t* addr, addr_t value) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    *addr = value;
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

int32_t userWait(addr_t* addr, uint32_t /*value*/, uint32_t timeout) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    while (--timeout > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        if (*addr != INVALID_REQUEST_ID) break;
    }

    if (timeout == 0) {
        return -1;
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return 0;
}
#endif
}  // namespace enn_driver
}  // namespace nn
}  // namespace android

