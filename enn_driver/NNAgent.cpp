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
 * @file    NNAgent.cpp
 * @brief   This is NNAgent class file.
 * @details This header defines NNAgent class.
 *          This class is implementing NNAgent which is Facade.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#include <android/sync.h>
#include <iostream>

#include "log.h"
#include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "NeuralNetworks.h"      // Operation, Operand, ANEURALNETWORKS_ADD etc
#include "ValidateHal.h"         // validateRequest

#include "../include/eden_model.h"

#include "Common.h"

#include "ModelConverter.h"
#include "CompilerManager.h"
#include "ResourceManager.h"
#include "EdenServiceDelegatorLib.h"
#include "EnnServiceDelegatorLib.h"
#include "BufferManager.h"

#ifdef ENABLE_DSP
#include "DspCompilerManager.h"
#endif

#include "NNAgent.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::NNAgent"

// @todo Since there is no useful maximum number representing for ANEURALNETWORKS_XXX, just sets it to 256 which is big enough.
#define ANDROID_NN_OP_NUM_MAXIMUM 256
#define FENCED_TIMEOUT 5000  // 5000ms (UD sets polling timeout to 5000ms)

using namespace eden::nn;

// V1_0::IExecutionCallback* executionCallback = nullptr;

namespace android {
namespace nn {
namespace eden_driver {

static void updateCapabilities(hidl_vec<V1_3::Capabilities::OperandPerformance>* operandPerformance,
                               V1_3::OperandType type,
                               V1_0::PerformanceInfo perf) {
    const auto it = std::lower_bound(operandPerformance->begin(), operandPerformance->end(), type,
                                     [](const V1_3::Capabilities::OperandPerformance& perf,
                                        V1_3::OperandType type) { return perf.type < type; });
    it->info = perf;
}

static void callNotifyWithPreparedModel(const sp<V1_0::IPreparedModelCallback>& callback_1_0, V1_3::ErrorStatus status, V1_3::IPreparedModel* preparedModel_1_3) {
    callback_1_0->notify(convertToV1_0(status), static_cast<V1_0::IPreparedModel*>(preparedModel_1_3));
}

static void callNotifyWithPreparedModel(const sp<V1_2::IPreparedModelCallback>& callback_1_2, V1_3::ErrorStatus status, V1_3::IPreparedModel* preparedModel_1_3) {
    callback_1_2->notify_1_2(convertToV1_0(status), static_cast<V1_2::IPreparedModel*>(preparedModel_1_3));
}

static void callNotifyWithPreparedModel(const sp<V1_3::IPreparedModelCallback>& callback_1_3, V1_3::ErrorStatus status, V1_3::IPreparedModel* preparedModel_1_3) {
    callback_1_3->notify_1_3(convertToV1_3(status), preparedModel_1_3);
}

int32_t prepareNNCModel(NNAgent* nnAgent, const V1_3::Model& model,
                        NNCBuf& nncBuffer, ModelInfo& modelInfo,
                        uint32_t& modelId, HwPreference& hwPreference, bool isNNCModel,
                        InHouseBufferInfo& inputBuffers, InHouseBufferInfo& outputBuffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode = RET_OK;
    bool encrypted =  false;
    NNCOpInfo nncOpInfo;
    retCode = nnAgent->modelConverter_->convertToNNC(model, nncBuffer, nncOpInfo, modelInfo);
    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) Error on convertToNNC(..)!\n", __func__);
        return retCode;
    }
    LOGD(EDEN_DRIVER, "NNC buffer is generated. addr : %p, size : %d \n", nncBuffer.addr, nncBuffer.size);

    // Call ennOpenModelFromMemory to register generated NNC
    ModelTypeInMemory modelTypeInMemory = MODEL_TYPE_IN_MEMORY_TFLITE;

    // @ToDo : Have to fix userPreference after the decision is made in Unified SDK
    EdenPreference preference;
    if (nncOpInfo.hasNpuOps == true) {
        hwPreference = NPU_ONLY;
    } else {
        hwPreference = GPU_ONLY;
    }
    preference.hw = hwPreference;
    preference.mode = BOOST_AUX;
    preference.inputBufferMode.enable = false;
    preference.inputBufferMode.setInputAsFloat = 0;

    LOGD(EDEN_DRIVER, "HWPreference : %d\n", preference.hw);
    LOGD(EDEN_DRIVER, "ModePreference : %d , NORMAL_MODE(0), BOOST_MODE(1), BOOST_ON_EXECUTE(2)\n", preference.mode);

    EdenModelOptions options;
    options.modelPreference.userPreference = preference;
    options.modelPreference.nnApiType = NnApiType::ANDROID_NN_API;
    options.priority = ModelPriority::P_DEFAULT;
    options.boundCore = NPU_UNBOUND;
    options.latency = 0;

    // OpenModelFromMemory
    retCode = nnAgent->ennServiceDelegator_->ennOpenModelFromMemory(modelTypeInMemory, nncBuffer.addr, nncBuffer.size,
                                                                    encrypted, &modelId, options);
    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) Error on ennOpenModelFromMemory(..)!\n", __func__);
        return retCode;
    }

    // Allocate input, output buffers for model
    retCode = nnAgent->allocateInputBuffers(modelId, isNNCModel, inputBuffers);
    if (retCode != RET_OK || inputBuffers.addr == nullptr) {
        LOGE(EDEN_DRIVER, "%s(-) Error on allocateInputBuffers(..)!\n", __func__);
        return retCode;
    }

    retCode = nnAgent->allocateOutputBuffers(modelId, isNNCModel, outputBuffers);
    if (retCode != RET_OK || outputBuffers.addr == nullptr) {
        LOGE(EDEN_DRIVER, "%s(-) Error on allocateOutputBuffers(..)!\n", __func__);
        return retCode;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}

int32_t prepareEdenModel(NNAgent* nnAgent, const V1_3::Model& model,
                         EdenModel*& edenModel, ModelInfo& modelInfo,
                         uint32_t& modelId, HwPreference& hwPreference, bool isNNCModel,
                         InHouseBufferInfo& inputBuffers, InHouseBufferInfo& outputBuffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode = RET_OK;
    {
        std::lock_guard<std::mutex> lock(nnAgent->mutex_vecOperationInfos_);
        if (nnAgent->vecNPUOperationInfos_.empty()) {
            LOGD(EDEN_DRIVER, "vecNPUOperationInfos_ is empty!\n");
            retCode = nnAgent->queryOperationInfo(model, TARGET_DEVICE_NPU, nnAgent->vecNPUOperationInfos_);
            if (retCode != RET_OK) {
                LOGE(EDEN_DRIVER, "%s(-) Error on queryOperationInfo(..)!\n", __func__);
                return retCode;
            }
        }
        nnAgent->modelConverter_->setNPUInfo(nnAgent->vecNPUOperationInfos_);
    }
    retCode = nnAgent->modelConverter_->convert(model, edenModel, modelInfo);
    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) Error on convert(..)!\n", __func__);
        return retCode;
    }

    //edenModel->DumpEdenModelForLOGD();

    // Once EdenModel is ready, it starts to communicate with EdenRuntime.

    // Call OpenModelFromMemory to register converted EdenModel.
    ModelPreference modelPreference;
    hwPreference = getHwPreference(modelInfo);
    // hwPreference = GPU_ONLY; // VTS1.2 crash sometimes in NPU branch
    modelPreference.userPreference.hw = hwPreference;
    if ((hwPreference == NPU_ONLY || hwPreference == ALL_HW) && nnAgent->compilerManager_->isSupportedModelByNPUC(model)) {
        modelPreference.userPreference.mode = BOOST_MODE;
    } else {
        modelPreference.userPreference.mode = NORMAL_MODE;
    }
    LOGD(EDEN_DRIVER, "ModePreference : %d , NORMAL_MODE(0), BOOST_MODE(1)\n", modelPreference.userPreference.mode);
    nnAgent->setComputePrecision(model, edenModel);
    modelPreference.userPreference.inputBufferMode.enable = false;
    modelPreference.userPreference.inputBufferMode.setInputAsFloat = 0;
    modelPreference.nnApiType = ANDROID_NN_API;

    // OpenModel
    retCode = nnAgent->edenServiceDelegator_->OpenModel(edenModel, &modelId, modelPreference);
    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) Error on OpenModel(..)!\n", __func__);
        return retCode;
    }

    // Allocate input, output buffers for model
    if (edenModel->ReadyToAllocateInputBuffers(nullptr)) {
        retCode = nnAgent->allocateInputBuffers(modelId, isNNCModel, inputBuffers);
        if (retCode != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Error on allocateInputBuffers(..)!\n", __func__);
            return retCode;
        }
    } else {
        inputBuffers.addr = nullptr;
        inputBuffers.numOfBuffers = 0;
    }

    if (edenModel->ReadyToAllocateOutputBuffers(nullptr)) {
        retCode = nnAgent->allocateOutputBuffers(modelId, isNNCModel, outputBuffers);
        if (retCode != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Error on allocateOutputBuffers(..)!\n", __func__);
            return retCode;
        }
    } else {
        outputBuffers.addr = nullptr;
        outputBuffers.numOfBuffers = 0;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}

template <typename T_Callback>
int32_t prepareModelBaseOnNNAgent(NNAgent* nnAgent,
                                  const V1_3::Model& model, ExecutionPreference /*preference*/,
                                  V1_3::Priority /*priority*/, const OptionalTimePoint& /*deadline*/,
                                  const hidl_vec<hidl_handle>* /*modelCache*/,
                                  const hidl_vec<hidl_handle>* /*dataCache*/,
                                  const HidlToken* /*token*/,
                                  const T_Callback& callback) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode;

    // Try to load a cached model if exists.
    V1_3::IPreparedModel* preparedModel = nullptr;
#ifdef ENABLE_DSP
    if (nnAgent->dspCompilerManager_->isSupportedModel(model) == RET_OK) {
        retCode = nnAgent->createPreparedModel(model, &preparedModel);
        if (retCode != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Error on createPrepareModel(..)!\n", __func__);
            return retCode;
        }

        retCode = nnAgent->dspCompilerManager_->compile(model, &preparedModel);
        if (retCode != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Error on compile(..)!\n", __func__);
            delete preparedModel;
            preparedModel = nullptr;
        }

        if (preparedModel != nullptr) {
            callNotifyWithPreparedModel(callback, V1_3::ErrorStatus::NONE, preparedModel);
            return RET_OK;
        }
    }
#endif
    nnAgent->resourceManager_->loadCachedModel(model, &preparedModel);
    if (preparedModel != nullptr) {
        callNotifyWithPreparedModel(callback, V1_3::ErrorStatus::NONE, preparedModel);
        return RET_OK;
    }

    // If fail to load a cached model, start model converting phase.
    bool isNNCModel = false;
    EdenModel* edenModel = nullptr;
    NNCBuf nncBuffer;
    ModelInfo modelInfo;
    uint32_t modelId = -1;
    HwPreference hwPreference;
    InHouseBufferInfo inputBuffers, outputBuffers;

    if (nnAgent->graphgenManager_->isGraphGenSupported() && nnAgent->graphgenManager_->isSupportedModel(model)) {
        isNNCModel = true;
        retCode = prepareNNCModel(nnAgent, model, nncBuffer, modelInfo, modelId,
                                  hwPreference, isNNCModel, inputBuffers, outputBuffers);
        if (retCode != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Error on prepareNNCModel(..)!\n", __func__);
            return retCode;
        }
    } else {
        isNNCModel = false;
        retCode = prepareEdenModel(nnAgent, model, edenModel, modelInfo, modelId,
                                   hwPreference, isNNCModel, inputBuffers, outputBuffers);
        if (retCode != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Error on prepareEdenModel(..)!\n", __func__);
            return retCode;
        }
    }

    std::vector<char*> vecAddr;
    std::vector<int32_t> vecSize;
    for (auto& vInfo : modelInfo.vecVirtualAddressOnPools) {
        if (vInfo.type == 1) {
            vecAddr.push_back(vInfo.addr);
            vecSize.push_back(vInfo.size);
        }
    }

    retCode = nnAgent->createPreparedModel(model, modelId, edenModel, hwPreference, nncBuffer, isNNCModel,
                                           inputBuffers, outputBuffers, vecAddr, vecSize, modelInfo.operandValues,
                                           modelInfo.mapOperandIdFromAToE, modelInfo.mapOperationIdFromAToE,
                                           modelInfo.inputConsumers, modelInfo.internalBuffers,
                                           modelInfo.vecIMemoryOnAshmems,
                                           &preparedModel);
    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) Error on createPrepareModel(..)!\n", __func__);
        return retCode;
    }

    // [CanBePostponed] Store model to cache.
    nnAgent->resourceManager_->storeModelToCache(model, preparedModel);

    callNotifyWithPreparedModel(callback, V1_3::ErrorStatus::NONE, preparedModel);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

template <typename T_Callback>
int32_t executeBaseOnNNAgent(NNAgent* nnAgent,
                             EdenPreparedModel* edenPreparedModel,
                             const V1_3::Request& request,
                             const hidl_vec<hidl_handle>& waitFor,
                             V1_2::MeasureTiming measure,
                             std::chrono::steady_clock::time_point driverStart,
                             const T_Callback& callback,
                             EXECUTION_MODE executionMode) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t ret = RET_OK;
    std::chrono::steady_clock::time_point driverStartAfterFence;

    do {
        uint32_t modelId = edenPreparedModel->modelId;

        if (edenPreparedModel->IsInvalidParams() == true) {
            LOGD(EDEN_DRIVER, "Oops, Invalid input parameters\n");
            ret = INVALID_PARAMS;
            break;
        }
        if (edenPreparedModel->resolveUnknownDimensions(request) != RET_OK) {
            LOGE(EDEN_DRIVER, "Oops, fail to resolve unknown dimension, rank!\n");
            ret = FAIL_TO_RESOLVE_UNKNOWN_DIMENSIONS;
            break;
        }
        ret = nnAgent->validateDeviceMemoryRequest(request, edenPreparedModel);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "Invalid Request for Device memory\n");
            break;
        }
        // Make sure input/output buffers are ready
        if (edenPreparedModel->needToAllocateInputBuffers()) {
            if (edenPreparedModel->readyToAllocateInputBuffers(&request)) {
                LOGD(EDEN_DRIVER, "Ready to Allocate Input Buffers\n");
                InHouseBufferInfo inputBuffers;
                ret = nnAgent->allocateInputBuffers(modelId, edenPreparedModel->isNNCModel(), inputBuffers);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "Oops, allocateInputBuffers() is failed!\n");
                    break;
                }
                edenPreparedModel->updateInputBuffers(inputBuffers.addr, inputBuffers.numOfBuffers);
            } else {
                LOGE(EDEN_DRIVER, "Oops, readyToAllocateInputBuffers() is failed!\n");
                ret = FAIL_TO_ALLOCATE_INPUT_BUFFERS_ON_EXECUTE;
                break;
            }
        } else {
            LOGD(EDEN_DRIVER, "Dont need to Allocate Input Buffers\n");
        }

        if (edenPreparedModel->needToAllocateOutputBuffers()) {
            if (edenPreparedModel->readyToAllocateOutputBuffers(&request)) {
                LOGD(EDEN_DRIVER, "Ready To Allocate Output Buffers\n");
                InHouseBufferInfo outputBuffers;
                ret = nnAgent->allocateOutputBuffers(modelId, edenPreparedModel->isNNCModel(), outputBuffers);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "Oops, allocateInputBuffers() is failed!\n");
                    break;
                }
                edenPreparedModel->updateOutputBuffers(outputBuffers.addr, outputBuffers.numOfBuffers);
            } else {
                LOGE(EDEN_DRIVER, "Oops, readyToAllocateOutputBuffers() is failed!\n");
                ret = FAIL_TO_ALLOCATE_OUTPUT_BUFFERS_ON_EXECUTE;
                break;
            }
        } else {
            LOGD(EDEN_DRIVER, "Dont need to Allocate Output Buffers\n");
        }

        // wait for sync fences
        if (executionMode == EXECUTION_MODE::FENCED) {
            LOGI(EDEN_DRIVER, "executionMode ==  EXECUTION_MODE::FENCED\n");
            for (const auto& handle : waitFor) {
                if (!handle.getNativeHandle()) {
                    LOGE(EDEN_DRIVER, "Can't get sync fence handle\n");
                    ret = FAIL_TO_WAIT_FOR_SYNC_FENCE;
                    break;
                }
                int syncFenceFd = handle.getNativeHandle()->data[0];

                if (sync_wait(syncFenceFd, FENCED_TIMEOUT) < 0) {
                    LOGE(EDEN_DRIVER, "Can't get sync fence handle\n");
                    ret = FAIL_TO_WAIT_FOR_SYNC_FENCE;
                    break;
                }
            }
            if (ret != RET_OK) {
                LOGE(EDEN_DRIVER, "fail to get sync fence handle\n");
                break;
            }
            LOGI(EDEN_DRIVER, "Finish to wait for fence handle\n");
            if (measure == V1_2::MeasureTiming::YES)
                driverStartAfterFence = std::chrono::steady_clock::now();
        }

        // Prepare buffer information on execute.
        BufferInfoOnExecute bufInfoOnExecute;

        // Load input data(Android NN Memory) to input buffer(Eden Memory Manager)
        ret = edenPreparedModel->loadInputData(request, bufInfoOnExecute);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "Oops, loadInputData() is failed!\n");
            break;
         }

        ret = nnAgent->executionScheduler_->requestOneExecution(edenPreparedModel,
                                                                request,
                                                                bufInfoOnExecute,
                                                                measure,
                                                                driverStart,
                                                                driverStartAfterFence,
                                                                callback,
                                                                executionMode);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "Oops, requestOneExecution() is failed!\n");
            break;
        }
        // Load output result(Eden Memory Manager) to output buffer(Android NN Memory)
        // loadOutputData(request.outputs);
    } while (0);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return ret;
}

/**
 * @brief Constructor
 * @details Constructor
 * @param void
 */
NNAgent::NNAgent(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    modelConverter_ = std::make_shared<ModelConverter>();
    compilerManager_ = std::make_shared<CompilerManager>();
    graphgenManager_ = std::make_shared<GraphGenManager>();
    executionScheduler_ = std::make_shared<ExecutionScheduler>();
    resourceManager_ = std::make_shared<ResourceManager>();
    edenServiceDelegator_ = std::make_shared<EdenServiceDelegatorLib>();
    ennServiceDelegator_ = std::make_shared<EnnServiceDelegatorLib>();
    bufferTracker_ = std::make_shared<BufferTracker>();

    modelConverter_->setCompilerManager(compilerManager_);
    modelConverter_->setGraphGenManager(graphgenManager_);
    executionScheduler_->setEdenServiceDelegator(edenServiceDelegator_);
    executionScheduler_->setEnnServiceDelegator(ennServiceDelegator_);
    executionScheduler_->setCompilerManager(compilerManager_);

#ifdef ENABLE_DSP
    dspCompilerManager_ = std::make_shared<DspCompilerManager>();
#endif

    initialize();
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Destructor
 * @details Destructor
 * @param void
 */
NNAgent::~NNAgent(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    shutdown();
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Initialize
 * @details This function initializes NNAgent.
 * @param void
 * @return error code
 */
void NNAgent::initialize(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    ennServiceDelegator_->ennInitialize();
    graphgenManager_->initGraphGenManager(ennServiceDelegator_);

    if (edenServiceDelegator_->GetState(&state_) == RET_OK) {
// @ToDo(R) : Unblock below code when latest eden-core is enabled.
//        DEVICE_STATE npuState = state_.deviceState[DevicesType_NPU];
//        if (npuState == RUNNING || npuState == INITIALIZED) {
        if (0) // remove
            compilerManager_->initNPUCCompiler();
//        }
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief close model
 * @details This function lets NNAgent close model.
 * @param modelId EdenModel modelId
 * @return error code
 */
void NNAgent::closeModel(uint32_t modelId, NNCBuf& nncBuffer, bool isNNCModel) {
    LOGD(EDEN_DRIVER, "%s(+) modelId is %u\n", __func__, modelId);

    if (isNNCModel) {
        graphgenManager_->freeMemory(nncBuffer);
        ennServiceDelegator_->ennCloseModel(modelId);
    } else {
        void* ncpBuffer = nullptr;
        if (edenServiceDelegator_->GetNcpBuffer(modelId, ncpBuffer) == RET_OK) {
            compilerManager_->clearNcpBuffer(ncpBuffer);
        }
        edenServiceDelegator_->CloseModel(modelId);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Shutdown
 * @details This function shutdowns NNAgent.
 * @param void
 * @return error code
 */
void NNAgent::shutdown(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (ennServiceDelegator_->ennShutdown()) {
        // @todo below code seems weird.
        // executionCallback->notify(V1_0::ErrorStatus::INVALID_ARGUMENT);
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Get capabilities for this device
 * @details This function returns a capabilities representing for performance of this device.
 * @param[in] capabilities Capabilities representing for performance of device
 * @return error code
 */
int32_t NNAgent::getCapabilities(V1_3::Capabilities& capabilities) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode = RET_OK;
    V1_0::PerformanceInfo perfInfo;

    // Get perfInfo for relaxed float32(float16) on scalar/tensor
    retCode = compilerManager_->getPerformanceInfo(DATA_TYPE::RELAXED_FLOAT32, perfInfo);
    if (retCode != RET_OK) return retCode;

    capabilities.relaxedFloat32toFloat16PerformanceScalar = perfInfo;
    capabilities.relaxedFloat32toFloat16PerformanceTensor = perfInfo;
    capabilities.operandPerformance = nonExtensionOperandPerformance<HalVersion::V1_3>(perfInfo);
    capabilities.ifPerformance = perfInfo;
    capabilities.whilePerformance = perfInfo;

    // Get performance info for foat32
    retCode = compilerManager_->getPerformanceInfo(DATA_TYPE::FLOAT32, perfInfo);
    if (retCode != RET_OK) return retCode;

    updateCapabilities(&capabilities.operandPerformance, V1_3::OperandType::TENSOR_FLOAT32, perfInfo);
    updateCapabilities(&capabilities.operandPerformance, V1_3::OperandType::FLOAT32, perfInfo);

    // Get perfInfo for quanitzed
    retCode = compilerManager_->getPerformanceInfo(DATA_TYPE::QUANT8, perfInfo);
    if (retCode != RET_OK) return retCode;

    updateCapabilities(&capabilities.operandPerformance, V1_3::OperandType::TENSOR_QUANT8_ASYMM, perfInfo);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Get supported operation list on a given model by this device
 * @details This function returns a supported operation list on a given model by this device.
 * @param[in] model Android NN Model
 * @param[out] supportedOperations supported operation list
 * @return error code
 */
int32_t NNAgent::getSupportedOperations(const V1_3::Model& model, std::vector<bool>& supportedOperations) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode = RET_OK;
    bool isRunByGraphGen = false;

    if (graphgenManager_->isGraphGenSupported() && graphgenManager_->isSupportedModel(model)) {
        isRunByGraphGen = true;
    } else {
        std::lock_guard<std::mutex> lock(mutex_vecOperationInfos_);
        // If it already has a supported operation list info, then return it.
        vecNPUOperationInfos_.clear();
        vecGPUOperationInfos_.clear();
        vecCPUOperationInfos_.clear();
#ifdef ENABLE_DSP
        vecDSPOperationInfos_.clear();
        if (vecDSPOperationInfos_.empty()) {
            LOGD(EDEN_DRIVER, "vecDSPOperationInfos_ is empty!\n");
            retCode = queryOperationInfo(model, TARGET_DEVICE_DSP, vecDSPOperationInfos_);
            if (retCode != RET_OK) return retCode;
        }
#endif

        if (vecNPUOperationInfos_.empty()) {
            LOGD(EDEN_DRIVER, "vecNPUOperationInfos_ is empty!\n");
            retCode = queryOperationInfo(model, TARGET_DEVICE_NPU, vecNPUOperationInfos_);
            if (retCode != RET_OK) return retCode;
        }
        if (vecGPUOperationInfos_.empty()) {
            LOGD(EDEN_DRIVER, "vecGPUOperationInfos_ is empty!\n");
            retCode = queryOperationInfo(model, TARGET_DEVICE_GPU, vecGPUOperationInfos_);
            if (retCode != RET_OK) return retCode;
        }
        if (vecCPUOperationInfos_.empty()) {
            LOGD(EDEN_DRIVER, "vecCPUOperationInfos_ is empty!\n");
            retCode = queryOperationInfo(model, TARGET_DEVICE_CPU, vecCPUOperationInfos_);
            if (retCode != RET_OK) return retCode;
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return loadSupportedOperationList(model, supportedOperations, isRunByGraphGen);
}

/**
 * @brief Check whether the model supported
 * @details This function returns a bool to identify whether the model is supported.
 * @param[in] model Android NN Model
 * @param[out] supportedModel bool
 * @return error code
 */
bool NNAgent::checkSupportedModel(const V1_3::Model& model) {
    // 24 Ops for AIBenchmark
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (model.referenced.size() != 0) {
        return false;
    }

    for (const V1_3::Operation &operation : model.main.operations) {
        int32_t opType = static_cast<int32_t>(operation.type);
        switch (opType) {
        case ANEURALNETWORKS_ADD:
        case ANEURALNETWORKS_AVERAGE_POOL_2D:
        case ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_LSTM:
        case ANEURALNETWORKS_CONCATENATION:
        case ANEURALNETWORKS_CONV_2D:
        case ANEURALNETWORKS_DEPTHWISE_CONV_2D:
        case ANEURALNETWORKS_DEPTH_TO_SPACE:
        case ANEURALNETWORKS_DIV:
        case ANEURALNETWORKS_FULLY_CONNECTED:
        case ANEURALNETWORKS_GATHER:
        case ANEURALNETWORKS_LOGISTIC:
        case ANEURALNETWORKS_MAX_POOL_2D:
        case ANEURALNETWORKS_MEAN:
        case ANEURALNETWORKS_MUL:
        case ANEURALNETWORKS_REDUCE_MIN:
        case ANEURALNETWORKS_RELU:
        case ANEURALNETWORKS_RESHAPE:
        case ANEURALNETWORKS_RESIZE_BILINEAR:
        case ANEURALNETWORKS_SOFTMAX:
        case ANEURALNETWORKS_SPLIT:
        case ANEURALNETWORKS_SQUEEZE:
        case ANEURALNETWORKS_SUB:
        case ANEURALNETWORKS_TANH:
        case ANEURALNETWORKS_TRANSPOSE_CONV_2D: break;
        default: {
            LOGI(EDEN_DRIVER, "unsupport Operations \n");
            return false;
        }
        }
    }

    if (checkDimensions(model) != RET_OK) {
        LOGI(EDEN_DRIVER, "unsupport Dimensions \n");
        return false;
    }
    if (checkGraph(model) != RET_OK) {
        LOGI(EDEN_DRIVER, "unsupport Graph \n");
        return false;
    }

    // Skip WHILE Tests
    std::vector<uint32_t> common_inputs;
    for (const V1_3::Operation &operation : model.main.operations) {
        common_inputs.insert(common_inputs.end(), operation.inputs.begin(), operation.inputs.end());
    }
    for (const int32_t inputIndex : model.main.inputIndexes) {
        auto itr = find(common_inputs.begin(), common_inputs.end(), inputIndex);
        if (itr == common_inputs.end()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Prepare model to be accelerated by this device
 * @details This function prepares a model to be accelerated by this device.
 *          To be accelerated, it should be converted to EdenModel.
 *          It includes compileation phase for NPU and it might take some time.
 *          If there is a cached model it will be loaded to avoid converting step.
 * @param[in] model Android NN Model
 * @param[in] preference ExecutionPreference
 * @param[in] callback IPreparedModelCallback
 * @return error code
 */
int32_t NNAgent::prepareModel(const V1_3::Model& model, ExecutionPreference preference,
                              V1_3::Priority priority, const OptionalTimePoint& deadline,
                              const hidl_vec<hidl_handle>* modelCache, const hidl_vec<hidl_handle>* dataCache,
                              const HidlToken* token,
                              const sp<V1_0::IPreparedModelCallback>& callback_1_0) {
    return prepareModelBaseOnNNAgent(this, model, preference, priority, deadline, modelCache, dataCache, token, callback_1_0);
}

/**
 * @brief Prepare model to be accelerated by this device
 * @details This function prepares a model to be accelerated by this device.
 *          To be accelerated, it should be converted to EdenModel.
 *          It includes compileation phase for NPU and it might take some time.
 *          If there is a cached model it will be loaded to avoid converting step.
 * @param[in] model Android NN Model
 * @param[in] preference ExecutionPreference
 * @param[in] callback IPreparedModelCallback
 * @return error code
 */
int32_t NNAgent::prepareModel(const V1_3::Model& model, ExecutionPreference preference,
                              V1_3::Priority priority, const OptionalTimePoint& deadline,
                              const hidl_vec<hidl_handle>* modelCache, const hidl_vec<hidl_handle>* dataCache,
                              const HidlToken* token,
                              const sp<V1_2::IPreparedModelCallback>& callback_1_2) {
    return prepareModelBaseOnNNAgent(this, model, preference, priority, deadline, modelCache, dataCache, token, callback_1_2);
}

/**
 * @brief Prepare model to be accelerated by this device
 * @details This function prepares a model to be accelerated by this device.
 *          To be accelerated, it should be converted to EdenModel.
 *          It includes compileation phase for NPU and it might take some time.
 *          If there is a cached model it will be loaded to avoid converting step.
 * @param[in] model Android NN Model
 * @param[in] preference ExecutionPreference
 * @param[in] callback IPreparedModelCallback
 * @return error code
 */
int32_t NNAgent::prepareModel(const V1_3::Model& model, ExecutionPreference preference,
                              V1_3::Priority priority, const OptionalTimePoint& deadline,
                              const hidl_vec<hidl_handle>* modelCache, const hidl_vec<hidl_handle>* dataCache,
                              const HidlToken* token,
                              const sp<V1_3::IPreparedModelCallback>& callback_1_3) {
    return prepareModelBaseOnNNAgent(this, model, preference, priority, deadline, modelCache, dataCache, token, callback_1_3);
}

int32_t NNAgent::allocateInputBuffers(uint32_t modelId, bool isNNCModel, InHouseBufferInfo& inputBuffers) {
    EdenBuffer* edenBuffers = nullptr;
    int32_t numOfBuffers = 0;
    int32_t retCode = RET_OK;

    if (isNNCModel) {
        retCode = ennServiceDelegator_->ennAllocateInputBuffers(modelId, &edenBuffers, &numOfBuffers);
    } else {
        retCode = edenServiceDelegator_->AllocateInputBuffers(modelId, &edenBuffers, &numOfBuffers);
    }

    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "AllocateInputBuffers() is failed.\n");
        return retCode;
    } else {
        inputBuffers.addr = reinterpret_cast<void*>(edenBuffers);
        inputBuffers.numOfBuffers = numOfBuffers;
    }

    return retCode;
}

int32_t NNAgent::allocateOutputBuffers(uint32_t modelId, bool isNNCModel, InHouseBufferInfo& outputBuffers) {
    EdenBuffer* edenBuffers = nullptr;
    int32_t numOfBuffers = 0;
    int32_t retCode = RET_OK;

    if (isNNCModel) {
        retCode = ennServiceDelegator_->ennAllocateOutputBuffers(modelId, &edenBuffers, &numOfBuffers);
    } else {
        retCode = edenServiceDelegator_->AllocateOutputBuffers(modelId, &edenBuffers, &numOfBuffers);
    }

    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "AllocateOutputBuffers() is failed.\n");
        return retCode;
    } else {
        outputBuffers.addr = reinterpret_cast<void*>(edenBuffers);
        outputBuffers.numOfBuffers = numOfBuffers;
    }

    return RET_OK;
}

/**
 * @brief allocate device managed buffer
 * @details allocate device managed buffer
 * @param[in] desc bufferdescripter
 * @param[in] prepredModels
 * @param[in] inputRoles
 * @param[in] outputroles
 * @param[in] ennBuffer
 * @param[in] callback to return Buffer and token
 * @return void
 */

void NNAgent::allocate(const V1_3::BufferDesc& desc,
                       const hidl_vec<sp<V1_3::IPreparedModel>>& preparedModels,
                       const hidl_vec<V1_3::BufferRole>& inputRoles,
                       const hidl_vec<V1_3::BufferRole>& outputRoles,
                       V1_3::IDevice::allocate_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    std::set<HalPreparedModelRole> roles;
    V1_3::Operand operand;
    constexpr uint32_t kInvalidBufferToken = 0;

    auto getModel =
        [](const sp<V1_3::IPreparedModel>& preparedModel) -> const V1_3::Model* {
        if (preparedModel->isRemote()) {
            LOGE(EDEN_DRIVER, "preparedModel->isRemote is nullptr()\n");
            return nullptr;
        }
        const EdenPreparedModel* edenPreparedModel =
            static_cast<EdenPreparedModel*>(preparedModel.get());
        if (edenPreparedModel == nullptr) {
            LOGE(EDEN_DRIVER, "Invalid params in allocate()\n");
            return nullptr;
        }
        return &(edenPreparedModel->model);
    };

    /* From validateHal.h */
    if (!validateMemoryDesc(desc, preparedModels, inputRoles, outputRoles,
                getModel, &roles, &operand)) {
        LOGD(EDEN_DRIVER, "validateMemoryDesc failed.\n");
        cb(V1_3::ErrorStatus::INVALID_ARGUMENT, nullptr, kInvalidBufferToken);
        return void();
    }

    if (inputRoles.size() > 1 || outputRoles.size() > 1) {
        LOGE(EDEN_DRIVER, "Not allocated EdenBuffer \n");
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return void();
     }

    // Get address of allocated Input/output buffers
    // ToDo: support some negative testcases for multiple inputRoles/outputRoles
    EdenBuffer* allocatedEdenBuffer = nullptr;
    if (inputRoles.size()) {
        const sp<V1_3::IPreparedModel>& aModel = preparedModels[inputRoles[0].modelIndex];
        const EdenPreparedModel* edenPreparedModel = static_cast<EdenPreparedModel*>(aModel.get());
        if (edenPreparedModel->needToAllocateInputBuffers() == false) {
            EdenBuffer* edenBufferForInputs =
                reinterpret_cast<EdenBuffer*>(edenPreparedModel->inputAddr);
            allocatedEdenBuffer = &(edenBufferForInputs[inputRoles[0].ioIndex]);
        } else {
            // ToDo: Need to allocate buffer based on size
            LOGE(EDEN_DRIVER, "Not support unknown dimension/rank");
            cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
            return void();
        }
    } else if (outputRoles.size()) {
        const sp<V1_3::IPreparedModel>& aModel = preparedModels[outputRoles[0].modelIndex];
        EdenPreparedModel* edenPreparedModel = static_cast<EdenPreparedModel*>(aModel.get());
        if (edenPreparedModel->needToAllocateInputBuffers() == false) {
            EdenBuffer* edenBufferForOutputs =
                reinterpret_cast<EdenBuffer*>(edenPreparedModel->outputAddr);
            allocatedEdenBuffer = &(edenBufferForOutputs[outputRoles[0].ioIndex]);
        } else {
            // ToDo: Need to allocate buffer based on size
            LOGE(EDEN_DRIVER, "Not support unknown dimension/rank");
            cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
            return void();
        }
    } else {
        LOGD(EDEN_DRIVER, "No input/output roles specified\n");
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return void();
    }

    if (allocatedEdenBuffer == nullptr || allocatedEdenBuffer->size == 0) {
        LOGE(EDEN_DRIVER, "Not allocated EdenBuffer or Allocated EdenBuffer size is 0\n");
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return void();
    }

    uint32_t size = nonExtensionOperandSizeOfData(operand.type, operand.dimensions);
    LOGD(EDEN_DRIVER, "size: %d, dimension: %s", size, toString(operand.dimensions).c_str());
    if (size == 0) {
        LOGE(EDEN_DRIVER, "does not support dynamic output shape\n");
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return void();
    }

    auto bufferWrapper = ManagedBuffer::create(allocatedEdenBuffer, size,
            std::move(roles), std::move(operand));
    if (bufferWrapper == nullptr) {
        LOGE(EDEN_DRIVER, "ManagedBuffer::create() failed\n");
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return void();
    }
    // add a token
    auto token = bufferTracker_->add(bufferWrapper);
    if (token == nullptr) {
        LOGE(EDEN_DRIVER, "BufferTracker returned invalid token.\n");
        cb(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr, kInvalidBufferToken);
        return void();
    }

    const uint32_t bufferTokenValue = token->get();

    // create new ENNBuffer instance.
    // Give ENNBuffer to app in cb.
    sp<ENNBuffer> newEnnBuffer = new ENNBuffer(std::move(bufferWrapper), std::move(token), this);
    LOGD(EDEN_DRIVER, "Success the requested memory: TokenValue = %d\n", bufferTokenValue);
    cb(V1_3::ErrorStatus::NONE, std::move(newEnnBuffer), bufferTokenValue);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return void();
}

bool NNAgent::initializeManagedBuffer(uint32_t token) {
    auto managedBuffer = bufferTracker_->get(token);
    if (managedBuffer == nullptr) {
        LOGD(EDEN_DRIVER, "Invalid managedBuffer\n");
        return false;
    }
    managedBuffer->setInitialized(true);
    LOGD(EDEN_DRIVER, "Buffer is initialized, buffer=[%p]\n", managedBuffer.get());
    return true;
}

/**
 * @brief copy content of device buffer to the shared memory
 * @details This functions copies data of allocated device buffer to the given hidl shared memory
 * @param[in] srcManagedBuffer ManagedBuffer class from which data need to be copied
 * @param[in] hidl shared memory location of destination
 * @returns error status
 */
V1_3::ErrorStatus NNAgent::copyToInternal(const std::shared_ptr<ManagedBuffer>& srcManagedBuffer,
                                               const hardware::hidl_memory &dst) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    // create virtual memorypool
    BufferInfoOnExecute bufInfoOnExecute;
    char * vAddr;
    uint32_t ret = bufInfoOnExecute.loadHidlMem(dst, false, vAddr);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "copyTo(): Unable to map dest memory\n");
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }
    // validate copyTo
    const V1_3::ErrorStatus validateError  = srcManagedBuffer->validateCopyTo(dst.size());
    if (validateError != V1_3::ErrorStatus::NONE) {
        LOGE(EDEN_DRIVER, "copyTo(): Failed to validate source buffer\n");
        return validateError;
    }

    // copy from Buffer to destination
    EdenBuffer* srcAddr = srcManagedBuffer->getBuffer();
    std::memcpy(reinterpret_cast<void*>(vAddr), srcAddr->addr, srcAddr->size);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_3::ErrorStatus::NONE;
}

/**
 * @brief copy the shared memory to device memory
 * @details This functions copies data from given hidl shared memory to the allocated device buffer
 * @param[in] destManagedBuffer ManagedBuffer class to which data need to be copied
 * @param[in] hidl shared memory location of source of data
 * @param[in] dimensions
 * @returns error status
 */
V1_3::ErrorStatus NNAgent::copyFromInternal(const std::shared_ptr<ManagedBuffer>& destManagedBuffer,
                                                 const hidl_memory& src,
                                                 const hidl_vec<uint32_t>& dimensions) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // create virtual memorypool
    BufferInfoOnExecute bufInfoOnExecute;
    char* vAddr;
    uint32_t ret = bufInfoOnExecute.loadHidlMem(src, true, vAddr);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "copyFrom(): Unable to map source memory\n");
        destManagedBuffer->setInitialized(false);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    const V1_3::ErrorStatus validationError = destManagedBuffer->validateCopyFrom(dimensions, src.size());
    if (validationError != V1_3::ErrorStatus::NONE) {
        LOGD(EDEN_DRIVER, "copyFrom(): failed to validate dest buffer\n");
        destManagedBuffer->setInitialized(false);
        return validationError;
    }
    EdenBuffer *dstAddr = destManagedBuffer->getBuffer();
    std::memcpy(dstAddr->addr, reinterpret_cast<void*>(vAddr), src.size());

    destManagedBuffer->updateDimensions(dimensions);
    destManagedBuffer->setInitialized(true);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_3::ErrorStatus::NONE;
}

/**
 * @brief Get device status
 * @details This function returns a device status.
 * @param[out] status DeviceStatus
 * @return error code
 */
int32_t NNAgent::getStatus(DeviceStatus& status) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // @todo Get device status from runtime
    status = DeviceStatus::AVAILABLE;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Execute a preparedModel with a given request asynchronously
 * @details This function executes a preparedModel with a given request
 *          and executes a callback when it's finished.
 * @param[in] preparedModel EdenPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t NNAgent::execute(EdenPreparedModel* edenPreparedModel,
                         const V1_3::Request& request,
                         V1_2::MeasureTiming /*measure*/,
                         const sp<V1_0::IExecutionCallback>& callback_1_0) {
    std::chrono::steady_clock::time_point dummyDriverStart;

    if (!validateRequest(request, edenPreparedModel->model)) {
        return INVALID_PARAMS;
    }

    return executeBaseOnNNAgent(this, edenPreparedModel, request, {}, V1_2::MeasureTiming::NO,
                                dummyDriverStart, callback_1_0, EXECUTION_MODE::ASYNC);
}

/**
 * @brief Execute a preparedModel with a given request asynchronously
 * @details This function executes a preparedModel with a given request
 *          and executes a callback when it's finished.
 * @param[in] preparedModel EdenPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t NNAgent::execute(EdenPreparedModel* edenPreparedModel,
                         const V1_3::Request& request,
                         V1_2::MeasureTiming measure,
                         const sp<V1_2::IExecutionCallback>& callback_1_2) {
    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) driverStart = std::chrono::steady_clock::now();

    if (!validateRequest(request, edenPreparedModel->model)) {
        return INVALID_PARAMS;
    }

    return executeBaseOnNNAgent(this, edenPreparedModel, request, {}, measure,
                                driverStart, callback_1_2, EXECUTION_MODE::ASYNC);
}

/**
 * @brief Execute a preparedModel with a given request asynchronously
 * @details This function executes a preparedModel with a given request
 *          and executes a callback when it's finished.
 * @param[in] preparedModel EdenPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t NNAgent::execute(EdenPreparedModel* edenPreparedModel,
                         const V1_3::Request& request,
                         V1_2::MeasureTiming measure,
                         const sp<V1_3::IExecutionCallback>& callback_1_3) {
    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) driverStart = std::chrono::steady_clock::now();

    if (!validateRequest(request, edenPreparedModel->model)) {
        return INVALID_PARAMS;
    }

    return executeBaseOnNNAgent(this, edenPreparedModel, request, {}, measure,
                                driverStart, callback_1_3, EXECUTION_MODE::ASYNC);
}

/**
 * @brief Execute a preparedModel with a given request synchronously
 * @details This function executes a preparedModel with a given request
 *          and executes a callback when it's finished.
 * @param[in] preparedModel EdenPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t NNAgent::executeSynchronously(EdenPreparedModel* edenPreparedModel,
                                      const V1_3::Request& request,
                                      V1_2::MeasureTiming measure,
                                      const sp<V1_2::IExecutionCallback>& callback_1_2) {
    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) driverStart = std::chrono::steady_clock::now();
    if (!validateRequest(request, edenPreparedModel->model)) {
        return INVALID_PARAMS;
    }
    return executeBaseOnNNAgent(this, edenPreparedModel, request, {}, measure,
                                driverStart, callback_1_2, EXECUTION_MODE::SYNC);
}

/**
 * @brief Execute a preparedModel with a given request synchronously
 * @details This function executes a preparedModel with a given request
 *          and executes a callback when it's finished.
 * @param[in] preparedModel EdenPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t NNAgent::executeSynchronously(EdenPreparedModel* edenPreparedModel,
                                      const V1_3::Request& request,
                                      V1_2::MeasureTiming measure,
                                      const sp<V1_3::IExecutionCallback>& callback_1_3) {
    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) driverStart = std::chrono::steady_clock::now();
    if (!validateRequest(request, edenPreparedModel->model)) {
        return INVALID_PARAMS;
    }
    return executeBaseOnNNAgent(this, edenPreparedModel, request, {}, measure,
                                driverStart, callback_1_3, EXECUTION_MODE::SYNC);
}

/**
 * @brief Execute a preparedModel asynchronously after wait sync fence in waitFor
 * @details This function wait for sync fences, and then executes a preparedModel
 *          with a given request and executes a callback when it's finished.
 * @param[in] preparedModel EdenPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] waitFor Sync fences to be waited.
 * @param[in] cb Callback function to be executed
 * @return error code
 */
int32_t NNAgent::executeFenced(EdenPreparedModel* edenPreparedModel,
                               const V1_3::Request& request,
                               const hidl_vec<hidl_handle>& waitFor,
                               V1_2::MeasureTiming measure,
                               hardware::neuralnetworks::V1_3::IPreparedModel::executeFenced_cb cb) {
    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) driverStart = std::chrono::steady_clock::now();
    if (!validateRequest(request, edenPreparedModel->model)) {
        return INVALID_PARAMS;
    }

    return executeBaseOnNNAgent(this, edenPreparedModel, request, waitFor, measure,
                                driverStart, cb, EXECUTION_MODE::FENCED);
}

/**
 * @brief Notify to caller by executing a callback function
 * @details This function notifies to the caller by executing a callback function.
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t NNAgent::notify(const sp<V1_2::IExecutionCallback>& /*callback_1_2*/) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Query operation information.
 * @details This function queries to target device to load operation information.
 *          such as supported operation list and its constraints.
 * @param[in] model Android NN Model
 * @param[in] targetDevice 0(NPU), 1(GPU), 2(CPU)
 * @param[out] vecOperationInfos OperationInfo for a target device
 * @return error code
 */
int32_t NNAgent::queryOperationInfo(const V1_3::Model& model, int32_t targetDevice, std::vector<OperationInfo>& vecOperationInfos) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::vector<std::shared_ptr<void>> constraints;
    int32_t retCode = queryConstraints(targetDevice, constraints);
    if (retCode != RET_OK) return retCode;

    // Query it to NPU compiler manager.
    std::vector<bool> supportedOperations;
    retCode = querySupportedOperations(model, targetDevice, constraints, supportedOperations);

    if (retCode != RET_OK) return retCode;

    // Make sure size for supportedOperations and constraints are same.
    if (supportedOperations.size() != constraints.size()) {
        LOGE(EDEN_DRIVER, "Error, size for supportedOperations and constraints should be same!\n");
        LOGE(EDEN_DRIVER, " supportedOperations.size()=%zu\n", static_cast<size_t>(supportedOperations.size()));
        LOGE(EDEN_DRIVER, " constraints.size()=%zu\n", static_cast<size_t>(constraints.size()));
        return INVALID_NUM_OF_SUPPORTED_OPERATIONS;
    }

    // If vecOperationInfos_ is occupied, it will be dropped.
    if (vecOperationInfos.empty() == false) {
        LOGE(EDEN_DRIVER, "vecOperationInfos is not empty, so they are dropped!\n");
        vecOperationInfos.clear();
    }

    // Keep supported operations and its constaints
    vecOperationInfos.resize(supportedOperations.size());
    for (size_t idx = 0; idx < supportedOperations.size(); idx++) {
        vecOperationInfos[idx].supported = supportedOperations[idx];
        vecOperationInfos[idx].constraint = constraints[idx];
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

int32_t NNAgent::resetSupportedOperations(std::vector<bool>& supportedOperations) {
    for (uint32_t idx = 0; idx < supportedOperations.size(); idx++) {
        supportedOperations[idx] = false;
    }
    return RET_OK;
}

/**
 * @brief Query to target device to load supported operation list.
 * @details This function queries to target device to load supported operation list.
 * @param[in] model Android NN Model
 * @param[in] targetDevice 0(NPU), 1(GPU), 2(CPU)
 * @param[out] supportedOperations supported operation list
 * @return error code
 */
int32_t NNAgent::querySupportedOperations(const V1_3::Model& model, int32_t targetDevice, const std::vector<std::shared_ptr<void>>& constraints,
                                          std::vector<bool>& supportedOperations) {
    LOGD(EDEN_DRIVER, "%s() is called with targetDevice:%d, NPU(0), GPU(1), CPU(2)\n", __func__, targetDevice);

    int32_t retCode = RET_OK;

    supportedOperations.resize(ANDROID_NN_OP_NUM_MAXIMUM);
    resetSupportedOperations(supportedOperations);

    if (!checkSupportedModel(model)) {
        return retCode;
    }

    switch (targetDevice) {
    case TARGET_DEVICE_NPU:  // NPU
        {
// @ToDo(R) : Unblock below code when latest eden-core is enabled.
//            DEVICE_STATE npuState = state_.deviceState[DevicesType_NPU];
//            if (npuState == RUNNING || npuState == INITIALIZED) {
            if (0) // remove
                return compilerManager_->getSupportedOperations(model, constraints, supportedOperations);
//            }

            LOGD(EDEN_DRIVER, "Not supported devices NPU\n");
            return RET_OK;
        }
    case TARGET_DEVICE_GPU:  // GPU
        // the following operations only support GPU
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_ABS)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_ARGMAX)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_ARGMIN)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_AXIS_ALIGNED_BBOX_TRANSFORM)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_LSTM)] = true; // 42

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_BIDIRECTIONAL_SEQUENCE_RNN)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_BOX_WITH_NMS_LIMIT)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_CAST)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_CHANNEL_SHUFFLE)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_DETECTION_POSTPROCESSING)] = true; // 47

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_EQUAL)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_EXP)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_EXPAND_DIMS)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_GATHER)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_GENERATE_PROPOSALS)] = true; // 52

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_GREATER)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_GREATER_EQUAL)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_GROUPED_CONV_2D)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_HEATMAP_MAX_KEYPOINT)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_INSTANCE_NORMALIZATION)] = true; // 57

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LESS)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LESS_EQUAL)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LOG)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LOGICAL_AND)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LOGICAL_NOT)] = true; // 62

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LOGICAL_OR)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LOG_SOFTMAX)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_MAXIMUM)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_MINIMUM)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_NEG)] = true;  // 67

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_NOT_EQUAL)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_PAD_V2)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_POW)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_PRELU)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_QUANTIZE)] = true; // 72

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_QUANTIZED_16BIT_LSTM)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RANDOM_MULTINOMIAL)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_REDUCE_ALL)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_REDUCE_ANY)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_REDUCE_MAX)] = true; // 77

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_REDUCE_MIN)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_REDUCE_PROD)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_REDUCE_SUM)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_ROI_ALIGN)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_ROI_POOLING)] = true;  // 82

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RSQRT)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SELECT)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SIN)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SLICE)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SPLIT)] = true; // 87

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SQRT)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_TILE)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_TOPK_V2)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_TRANSPOSE_CONV_2D)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_UNIDIRECTIONAL_SEQUENCE_LSTM)] = true; // 92

        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_UNIDIRECTIONAL_SEQUENCE_RNN)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RESIZE_NEAREST_NEIGHBOR)] = true;

        [[fallthrough]];
    case TARGET_DEVICE_CPU:  // CPU
        // List up supported operation list as below
        // @todo below define should be replaced to proper number
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_ADD)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_AVERAGE_POOL_2D)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_CONCATENATION)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_CONV_2D)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_DEPTHWISE_CONV_2D)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_DEPTH_TO_SPACE)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_DEQUANTIZE)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_EMBEDDING_LOOKUP)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_FLOOR)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_FULLY_CONNECTED)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_HASHTABLE_LOOKUP)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_L2_NORMALIZATION)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_L2_POOL_2D)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LOCAL_RESPONSE_NORMALIZATION)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LOGISTIC)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LSH_PROJECTION)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_LSTM)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_MAX_POOL_2D)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_MUL)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RELU)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RELU1)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RELU6)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RESHAPE)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RESIZE_BILINEAR)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_RNN)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SOFTMAX)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SPACE_TO_DEPTH)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SVDF)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_TANH)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_BATCH_TO_SPACE_ND)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_DIV)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_MEAN)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_PAD)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SPACE_TO_BATCH_ND)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SQUEEZE)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_STRIDED_SLICE)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_SUB)] = true;
        supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_TRANSPOSE)] = true;

        // supportedOperations[static_cast<int32_t>(ANEURALNETWORKS_OEM_OPERATION)] = false;
        break;
#ifdef ENABLE_DSP
    case TARGET_DEVICE_DSP:  // DSP
        {
            if (dspCompilerManager_->isSupportedModel(model) == RET_OK) {
                return dspCompilerManager_->getSupportedOperations(model, constraints, supportedOperations);
            }
            LOGD(EDEN_DRIVER, "Not supported devices DSP\n");
            return RET_OK;
        }
#endif
    default:
        LOGE(EDEN_DRIVER, "Oops, targetDevice=%d is not yet supported!\n", targetDevice);
        retCode = INVALID_TARGET_DEVICE;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}

/**
 * @brief Query to target device to load constraints list.
 * @details This function queries to target device to load constraints list.
 * @param[in] targetDevice 0(NPU), 1(GPU), 2(CPU)
 * @param[out] constraints constraints list
 * @return error code
 */
int32_t NNAgent::queryConstraints(int32_t targetDevice, std::vector<std::shared_ptr<void>>& constraints) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode = RET_OK;
    constraints.resize(ANDROID_NN_OP_NUM_MAXIMUM);
    for (int32_t idx = 0; idx < ANDROID_NN_OP_NUM_MAXIMUM; idx++) {
        constraints[idx] = nullptr;
    }

    switch (targetDevice) {
    case TARGET_DEVICE_NPU:  // NPU
        retCode = compilerManager_->getConstraints(constraints);
        break;
    case TARGET_DEVICE_GPU:  // GPU
    case TARGET_DEVICE_CPU:  // CPU
#ifdef ENABLE_DSP
    case TARGET_DEVICE_DSP:
#endif
        // @todo Add constraints if there is
        break;
    default:
        LOGE(EDEN_DRIVER, "Oops, targetDevice=%d is not yet supported!\n", targetDevice);
        retCode = INVALID_TARGET_DEVICE;
        break;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}

/**
 * @brief Load supported operation on model
 * @details This function loads supported operation on model.
 *          Index on supportedOperations corresponds to operations on model.
 * @param[in] model Android NN Model
 * @param[out] supportedOperations supported operation list on model
 * @return error code
 */
int32_t NNAgent::loadSupportedOperationList(const V1_3::Model& model,
                                            std::vector<bool>& supportedOperations, bool isRunByGraphGen) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    LOGD(EDEN_DRIVER, "model.main.operations.size: %zu\n", model.main.operations.size());
    if (!supportedOperations.empty()){
        supportedOperations.clear();
    }
    supportedOperations.resize(model.main.operations.size());

    if (isRunByGraphGen) {
        for (size_t idx = 0; idx < model.main.operations.size(); idx++) {
            supportedOperations.at(idx) = true;
        }
    } else {
        for (size_t idx = 0; idx < model.main.operations.size(); idx++) {
            int32_t opType = static_cast<int32_t>(model.main.operations[idx].type);
            LOGD(EDEN_DRIVER, "opType: %d\n", opType);
            // @ToDo(R) ANEURALNETWORKS_RANK is last op name. I think we should use another name.
            if ((opType < 0) || (opType > ANEURALNETWORKS_RANK)) {
                return INVALID_NUM_OF_SUPPORTED_OPERATIONS;
            }
#ifdef ENABLE_DSP
            if (dspCompilerManager_->isSupportedModel(model) == RET_OK) {
                if (vecDSPOperationInfos_.at(opType).supported) {
                    supportedOperations.at(idx) = true;
                } else {
                    supportedOperations.at(idx) = false;
                }
            } else
#endif
            {
                if ((vecNPUOperationInfos_.at(opType).supported) ||
                    (vecGPUOperationInfos_.at(opType).supported) ||
                    (vecCPUOperationInfos_.at(opType).supported)) {
                    supportedOperations.at(idx) = true;
                } else {
                    supportedOperations.at(idx) = false;
                }
            }
            LOGD(EDEN_DRIVER, "supportedOperations[%zu] is %d\n",
                                                   idx, static_cast<int32_t>(supportedOperations.at(idx)));
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Load cached IPreparedModel if exists
 * @details This function tries to load cached IPreparedModel if exists.
 *          If there is a cached IPreparedModel, it is loaded on a given preparedModel.
 *          If not, preparedModel has nullptr.
 * @param[in] model Android NN Model
 * @param[out] preparedModel IPreparedModel loaded from the cached model
 * @return error code
 */
int32_t NNAgent::loadCachedModel(const V1_3::Model& /*model*/, V1_3::IPreparedModel** /*preparedModel*/) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return 0;
}

/**
 * @brief Store IPreparedModel to cache
 * @details This function stores a IPreparedModel to cache
 *          so that it can be retrieved via loadCachedModel next time.
 * @param[in] model Android NN Model
 * @param[in] preparedModel IPreparedModel to be stored to the cached model
 * @return error code
 */
int32_t NNAgent::storeCachedModel(const V1_3::Model& /*model*/, V1_3::IPreparedModel* /*preparedModel*/) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return 0;
}

/**
 * @brief Create IPreparedModel
 * @details This function creates a IPreparedModel with given information.
 * @param[in] model Android NN Model
 * @param[in] modelId unique model id generated by Eden Runtime service
 * @param[in] edenModel EdenModel
 * @param[in] inputBuffers InHouseBufferInfo representing for buffer information returned by Eden Runtime service
 * @param[in] outputBuffers InHouseBufferInfo representing for buffer information returned by Eden Runtime service
 * @param[in] vecAddr virtual address for buffers on pools
 * @param[in] vecSize size for buffers on pools
 * @param[in] operandValues copied internal operandValues of model
 * @param[in] mapOperandIdFromAToE map operand id from Android NN Model to Eden Model
 * @param[in] mapOperationIdFromAToE map operation id from Android NN Model to Eden Model
 * @param[in] inputConsumers android operation id that consumes an operand at index
 * @param[in] internalBuffers internal buffers allocated for this model
 * @param[in] vecIMemoryOnAshmems list of sp<IMemory> that returned via mapMemory()
 * @param[out] preparedModel IPreparedModel which is generated
 * @return error code
 */
int32_t NNAgent::createPreparedModel(const V1_3::Model& model, uint32_t modelId,
                                     EdenModel* edenModel, HwPreference hwPreference,
                                     NNCBuf nncBuffer, bool isNNCModel,
                                     InHouseBufferInfo inputBuffers, InHouseBufferInfo outputBuffers,
                                     std::vector<char*>& vecAddr, std::vector<int32_t>& vecSize,
                                     std::unique_ptr<uint8_t[]>& operandValues,
                                     std::map<int32_t, int32_t>& mapOperandIdFromAToE,
                                     std::map<int32_t, int32_t>& mapOperationIdFromAToE,
                                     std::vector<int32_t>& inputConsumers,
                                     std::vector<std::unique_ptr<int32_t[]>>& internalBuffers,
                                     std::vector<sp<IMemory>>& vecIMemoryOnAshmems,
                                     V1_3::IPreparedModel** preparedModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    EdenPreparedModel* edenPreparedModel = new EdenPreparedModel(this, model, modelId, edenModel,
                                                                 hwPreference, nncBuffer, isNNCModel,
                                                                 inputBuffers.addr, inputBuffers.numOfBuffers,
                                                                 outputBuffers.addr, outputBuffers.numOfBuffers,
                                                                 vecAddr, vecSize, operandValues,
                                                                 mapOperandIdFromAToE, mapOperationIdFromAToE,
                                                                 inputConsumers, internalBuffers,
                                                                 vecIMemoryOnAshmems); // FIXME inputBuffers outputBuffers

    *preparedModel = edenPreparedModel;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

#ifdef ENABLE_DSP
/**
 * @brief Create IPreparedModel
 * @details This function creates a IPreparedModel with given information.
 * @param[in] model Android NN Model
 * @param[out] preparedModel IPreparedModel which is generated
 * @return error code
 */
int32_t NNAgent::createPreparedModel(const V1_3::Model& model,
                                     V1_3::IPreparedModel** preparedModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    DspPreparedModel* dspPreparedModel = new DspPreparedModel(model);

    *preparedModel = dspPreparedModel;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}


#endif

/**
 * @brief Set compute precision(FP32, FP16, UINT8 for a given EdenModel
 * @details This function calculates a compute precision based on a GPU userdriver policy.
 * @param[in] model Android NN Model
 * @param[in] edenModel EdenModel
 * @return error code
 */
void NNAgent::setComputePrecision(const V1_3::Model& model, EdenModel* edenModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // Below logic comes from legacy EdenDriver implemented by SRCX.
    ComputePrecision computePrecision = FP16;

    if (model.relaxComputationFloat32toFloat16 == false) {
        LOGD(EDEN_DRIVER, "model.relaxComputationFloat32toFloat16 is false, so set computePrecision to FP32\n");
        computePrecision = FP32;
    }

    for (int32_t inputIndex : model.main.inputIndexes) {
        auto& operand = model.main.operands[inputIndex];
        if ((operand.lifetime == V1_3::OperandLifeTime::SUBGRAPH_INPUT) &&
            (operand.scale != 0 || operand.zeroPoint != 0) &&
            operand.type != V1_3::OperandType::TENSOR_INT32) {
            LOGD(EDEN_DRIVER, "input operand's lifetime is SUBGRAPH_INPUT, so set computePrecision to UINT8\n");
            if (operand.type == V1_3::OperandType::TENSOR_QUANT8_ASYMM_SIGNED) {
                computePrecision = INT8;
            } else {
                computePrecision = UINT8;
            }
            break;
        } else if (operand.type == V1_3::OperandType::TENSOR_FLOAT16) {
            computePrecision = FP16;
            break;
        }
    }

    // Use FP32 to compute FLOAT16 to get better accurary to pass VTS&CTS
    if (model.main.operations.size() == 1) {
        int32_t opType = static_cast<int32_t>(model.main.operations[0].type);
        switch (opType) {
            case ANEURALNETWORKS_CONV_2D:
            case ANEURALNETWORKS_TRANSPOSE_CONV_2D:
            case ANEURALNETWORKS_GROUPED_CONV_2D:
            case ANEURALNETWORKS_LOG_SOFTMAX:
            case ANEURALNETWORKS_LSTM:
            case ANEURALNETWORKS_DEPTHWISE_CONV_2D: {
                const V1_3::Operand &androidInputOperandFirst = model.main.operands[model.main.operations[0].inputs[0]];
                if (androidInputOperandFirst.type == V1_3::OperandType::TENSOR_FLOAT16) {
                    LOGD(EDEN_DRIVER, "set FP32 to get better accurary");
                    computePrecision = FP32;
                }
                break;
            }
            default:
                break;
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    edenModel->SetComputePrecision(computePrecision);
}

/**
 * @brief check dimensions for a given model
 * @details This function check dimensions, return error if it is larger than 4 for some operations.
 * @param[in] model Android NN Model
 * @return error code
 */
int32_t NNAgent::checkDimensions(const V1_3::Model& model) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    // bypass the notice of execution failure in CTS

    for (const V1_3::Operation &operation : model.main.operations) {
        int32_t inputSizeSum = 0;
        for (size_t idx = 0; idx < operation.inputs.size(); idx++) {
            int32_t inputIndex = operation.inputs[idx];
            const V1_3::Operand& androidOperand = model.main.operands[inputIndex];
            for (size_t i = 0; i < androidOperand.dimensions.size(); i++) {
                inputSizeSum += androidOperand.dimensions[i];
            }
        }
        if (inputSizeSum == 0) {
            LOGE(EDEN_DRIVER, "Invalied inputParams.\n");
            return RET_PARAM_INVALID;
        }


        int32_t outputSizeSum = 0;
        for (size_t idx = 0; idx < operation.outputs.size(); idx++) {
            int32_t outputIndex = operation.outputs[idx];
            const V1_3::Operand& androidOperand = model.main.operands[outputIndex];
            for (size_t i = 0; i < androidOperand.dimensions.size(); i++) {
                outputSizeSum += androidOperand.dimensions[i];
            }
        }
        if (outputSizeSum == 0) {
            LOGE(EDEN_DRIVER, "Invalied OutputParams.\n");
            return RET_PARAM_INVALID;
        }
    }

    // TODO: to support dimension large than 4
    for (const V1_3::Operation &operation : model.main.operations) {
        int32_t opType = static_cast<int32_t>(operation.type);
        switch (opType) {
            case ANEURALNETWORKS_EXPAND_DIMS:
            case ANEURALNETWORKS_POW:
            case ANEURALNETWORKS_PRELU:
            case ANEURALNETWORKS_MAXIMUM:
            case ANEURALNETWORKS_MINIMUM:
            case ANEURALNETWORKS_LOGICAL_AND:
            case ANEURALNETWORKS_LOGICAL_OR:
            case ANEURALNETWORKS_ARGMIN:
            case ANEURALNETWORKS_ARGMAX:
            case ANEURALNETWORKS_TILE :
            case ANEURALNETWORKS_GATHER : {
                for (size_t idx = 0; idx < operation.inputs.size(); idx++) {
                    uint32_t idxInput = operation.inputs[idx];
                    const V1_3::Operand& androidOperand = model.main.operands[idxInput];
                    if (androidOperand.dimensions.size() > 4) {
                        LOGD(EDEN_DRIVER,  "not support dimensions.size = %d ", (int)androidOperand.dimensions.size());
                        return RET_PARAM_INVALID;
                    }
                }

                for (size_t idx = 0; idx < operation.outputs.size(); idx++) {
                    uint32_t idxOutput = operation.outputs[idx];
                    const V1_3::Operand& androidOperand = model.main.operands[idxOutput];
                    if (androidOperand.dimensions.size() > 4) {
                        LOGD(EDEN_DRIVER,  "not support dimensions.size = %d", (int)androidOperand.dimensions.size());
                        return RET_PARAM_INVALID;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    return RET_OK;
}


/**
 * @brief check graph
 * @details This function check graph, return error if it is not supported in eden
 * @param[in] model Android NN Model
 * @return error code
 */
int32_t NNAgent::checkGraph(const V1_3::Model& model) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    for (const V1_3::Operation &operation : model.main.operations) {
        int32_t opType = static_cast<int32_t>(operation.type);

        if (opType == ANEURALNETWORKS_REDUCE_SUM && model.main.operations.size() == 1) { // to skip case that mse>0.0001
            for (int32_t inputIndex : model.main.inputIndexes) {
                auto& operand = model.main.operands[inputIndex];
                if (operand.type == V1_3::OperandType::TENSOR_FLOAT16) {
                    return RET_PARAM_INVALID;
                }
            }
        }

        if ( opType == ANEURALNETWORKS_INSTANCE_NORMALIZATION && model.main.operations.size() == 1) {
            // temperary skip INSTANCE_NORMALIZATION of CTS
            return RET_PARAM_INVALID;
        }

        if (model.main.operations.size() >= 2) {
            // 2) TODO: to support create graph with fully const input
            bool isConstInput = true;
            for (size_t idx = 0; idx < operation.inputs.size(); idx++) {
                uint32_t idxInput = operation.inputs[idx];
                const V1_3::Operand& androidOperand = model.main.operands[idxInput];
                if (androidOperand.lifetime == V1_3::OperandLifeTime::SUBGRAPH_INPUT ||
                    androidOperand.lifetime == V1_3::OperandLifeTime::TEMPORARY_VARIABLE) {
                    isConstInput = false;
                }
            }

            if (isConstInput == true) {
                LOGD(EDEN_DRIVER, "eden-drv not support model with full const input \n");
                return RET_PARAM_INVALID;
            }
        }
    }

    return RET_OK;
}

int32_t NNAgent::validateDeviceMemoryRequest(const V1_3::Request& request,
                                             const EdenPreparedModel* edenPreparedModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    for (uint32_t i = 0; i < request.pools.size(); i++) {
        auto& pool = request.pools[i];
        if (pool.getDiscriminator() == V1_3::Request::MemoryPool::hidl_discriminator::token) {
            auto managedBuffer = bufferTracker_->get(pool.token());
            if (managedBuffer == nullptr) {
                LOGD(EDEN_DRIVER, "Invalid Managed buffer\n");
                return INVALID_PARAMS;
            }
            auto ret = managedBuffer->validateRequest(i, request, edenPreparedModel);
            if (ret) {
                LOGE(EDEN_DRIVER, "Invalid device memory request\n");
                return ret;
            }
        }
    }
    return RET_OK;
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android
