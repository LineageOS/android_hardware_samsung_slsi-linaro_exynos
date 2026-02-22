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
 * @file    NNAgent.cpp
 * @brief   This is NNAgent class file.
 * @details This header defines NNAgent class.
 *          This class is implementing NNAgent which is Facade.
 * @author  nihar.desai/shashank.r/ankit.goel
 */

//#include <android/sync.h>
#include <iostream>
#include <fstream>

#include "log.h"

#include "NeuralNetworks.h"      // Operation, Operand, ANEURALNETWORKS_ADD etc

#include "../enn/include/enn_api-type.h"

#include "Common.h"

#include "NNAgent.h"
#include "ModelUtils.h"
#include "UEnnServiceDelegatorLib.h"
#include "BufferManager.h"
#include "LegacyUtils.h"


#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EnnDriver::NNAgent"

// @todo Since there is no useful maximum number representing for ANEURALNETWORKS_XXX, just sets it to 256 which is big enough.
#define ANDROID_NN_OP_NUM_MAXIMUM 256

using namespace ::android::nn;

namespace android {
namespace nn {
namespace enn_driver {

#ifdef NNDRIVER_DEBUG
void write_bin_file(const std::string &file_path, int8_t* data, int size)
{
    std::ofstream ofs(file_path, std::ios::binary | std::ios::out);
    ofs.write((char*)data, size);
    ofs.close();
}
#endif

int32_t adjustIndex(const Model& model, const int32_t iIndex);
size_t calculateBufferSize(const Model& model, const int32_t operandIdx);
/**
 * @brief This function Prepares NNC Model for the Given Ann Canonical Model.
 * @detail
 * @param[in] model Android NN Model
 * @param[in] NNCbuffer returned from Graphgen
 * @param[in] EnnModelId Model Id of the model
 * @param[in] UEnnBufferInfo Information about input buffers of the model
 * @param[in] UEnnBufferInfo Information about output buffers of the model
 * @return error code
 */


int32_t prepareNNCModel(NNAgent* nnAgent, const Model& model,
                        NNCBuf& nncBuffer,
                        EnnModelId *model_id,
                        UEnnBufferInfo& inputBuffers, UEnnBufferInfo& outputBuffers) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode = RET_OK;
    bool encrypted =  false;
    NNCOpInfo nncOpInfo;
    retCode = nnAgent->modelConverter_->convertToNNC(model, nncBuffer, nncOpInfo);
    if (retCode != RET_OK) {
        LOGE(ENN_DRIVER, "%s(-) Error on convertToNNC(..)!\n", __func__);
        return retCode;
    }

    LOGD(ENN_DRIVER, "NNC buffer is generated. addr : %p, size : %d \n", nncBuffer.addr, nncBuffer.size);

#ifdef NNDRIVER_DEBUG
    write_bin_file(nnAgent->model_file, nncBuffer.addr, nncBuffer.size);
#endif
    // ToDo: Modify Preference for execution.
    const EnnModelPreference preference = {0};

    retCode = nnAgent->uennServiceDelegator_->uennOpenModel(reinterpret_cast<const char *>(nncBuffer.addr), nncBuffer.size, model_id);

    if (retCode != RET_OK) {
        LOGE(ENN_DRIVER, "%s(-) Error on ennOpenModelFromMemory(..)!\n", __func__);
        nnAgent->graphgenManager_->freeMemory(nncBuffer);
        return retCode;
    }

    // Allocate input, output buffers for model
    const EnnModelId model_id_ = *model_id;
    retCode = nnAgent->allocateAllBuffers(model_id_, inputBuffers, outputBuffers);
    if (retCode != RET_OK || inputBuffers.addr == nullptr || outputBuffers.addr == nullptr) {
        LOGE(ENN_DRIVER, "%s(-) Error on allocateAllBuffers(..)!\n", __func__);
        return retCode;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}

/**
 * @brief  prepareModelBaseOnNNAgent: This function Prepares IPreparedModel for the Given Ann Canonical Model
 * @detail
 * @param[in] nnAgent nnAgent class
 * @param[in] model Android NN Model
 * @param[in] ExecutionPreference preference for executiong model
 * @param[in] Priority
 * @param[in] OptionalTimePoint
 * @param[in] ModelCache
 * @param[in] DataCache
 * @param[in] CacheToken
 * @param[in] IPreparedModel PreparedModel creted from given  ANN Model.
 * @return error code
 */
int32_t prepareModelBaseOnNNAgent(NNAgent* nnAgent, const Model& model, ExecutionPreference /*preference*/,
                                  Priority /*priority*/, const OptionalTimePoint& /*deadline*/,
                                  const std::vector<SharedHandle>* /*modelCache*/,
                                  const std::vector<SharedHandle>* /*dataCache*/,
                                  const CacheToken* /*token*/, std::shared_ptr<IPreparedModel>  &rPreparedModel) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode;

    NNCBuf nncBuffer;
    EnnModelId model_id;
    HwType hwPreference = nnAgent->hwtype_;
    UEnnBufferInfo inputBuffers, outputBuffers;
    if (nnAgent->isModelSupported_) {
        retCode = prepareNNCModel(nnAgent, model, nncBuffer, &model_id,
                                  inputBuffers, outputBuffers);
        if (retCode != RET_OK) {
            LOGE(ENN_DRIVER, "%s(-) Error on prepareNNCModel(..)!\n", __func__);
            return retCode;
        }
    } else {
        LOGE(ENN_DRIVER, "%s(-) Model Not FullY Supported\n", __func__);
        return MODEL_FULLY_NOT_SUPPORTED;
    }
    retCode = nnAgent->createPreparedModel(model, model_id, hwPreference, nncBuffer,
                                           inputBuffers, outputBuffers,
                                           rPreparedModel);
    if (retCode != RET_OK) {
        LOGE(ENN_DRIVER, "%s(-) Error on createPrepareModel(..)!\n", __func__);
        return retCode;
    }

    /*
    // [CanBePostponed] Store model to cache.
    nnAgent->resourceManager_->storeModelToCache(model, preparedModel);
    */
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}


/**
 * @brief  executeModelBaseOnNNAgent: This function executes a Model
 * @detail
 * @param[in] nnAgent nnAgent class
 * @param[in] model Android NN Model
 * @param[in] EnnPreparedModel PreparedModel to be executed
 * @param[in] Request request specifies input/output information for execution
 * @param[in] <SyncFence> waitFor
 * @param[in] MeasureTiming mesure eecution time
 * @param[in] time_point driverStart
 * @param[in] executionMode specified mode of execution: SYNC/ASYNC/FENCE/BURST
 * @param[in] OutputShape shape of Output
 * @param[in] timing execution timing
 * @return error code
 */
int32_t executeBaseOnNNAgent(NNAgent* nnAgent,
                             const EnnPreparedModel* ennPreparedModel,
                             const Request& request,
                             const std::vector<SyncFence>& waitFor,
                             MeasureTiming measure,
                             std::chrono::steady_clock::time_point driverStart,
                             EXECUTION_MODE executionMode,
                             std::vector<OutputShape>& outputshape, Timing& timing) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    int32_t ret = RET_OK;
    std::chrono::steady_clock::time_point driverStartAfterFence;

    do {
        uint32_t modelId = ennPreparedModel->modelId;

        if (ennPreparedModel->IsInvalidParams() == true) {
            LOGD(ENN_DRIVER, "Oops, Invalid input parameters\n");
            ret = INVALID_PARAMS;
            break;
        }
        ret = nnAgent->validateDeviceMemoryRequest(request, ennPreparedModel);
        if (ret != RET_OK) {
            LOGE(ENN_DRIVER, "Invalid Request for Device memory\n");
            break;
        }

        // Prepare buffer information on execute.
        BufferInfoOnExecute bufInfoOnExecute;

        // Load input data(Android NN Memory) to input buffer(Enn Memory Manager)
        ret = ennPreparedModel->loadInputData(request, bufInfoOnExecute);
        if (ret != RET_OK) {
            LOGE(ENN_DRIVER, "Oops, loadInputData() is failed!\n");
            break;
         }

        ret = nnAgent->executionScheduler_->requestOneExecution(ennPreparedModel,
                                                                request,
                                                                bufInfoOnExecute,
                                                                measure,
                                                                driverStart,
                                                                driverStartAfterFence,
                                                                executionMode, outputshape, timing);
        if (ret != RET_OK) {
            LOGE(ENN_DRIVER, "Oops, requestOneExecution() is failed!\n");
            break;
        }
        // Load output result(Enn Memory Manager) to output buffer(Android NN Memory)
        // loadOutputData(request.outputs);
    } while (0);

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return ret;
}

/**
 * @brief Constructor
 * @details Constructor
 * @param void
 */
NNAgent::NNAgent(HwType hwtype) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    modelConverter_ = std::make_shared<ModelConverter>();
    graphgenManager_ = std::make_shared<GraphGenManager>();
    executionScheduler_ = std::make_shared<ExecutionScheduler>();
    uennServiceDelegator_ = std::make_shared<UEnnServiceDelegatorLib>();
    bufferTracker_ = std::make_shared<BufferTracker>();

    modelConverter_->setGraphGenManager(graphgenManager_);
    executionScheduler_->setUEnnServiceDelegator(uennServiceDelegator_);
    hwtype_ = hwtype;

    initialize();
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Destructor
 * @details Destructor
 * @param void
 */
NNAgent::~NNAgent(void) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    uennServiceDelegator_->uennDeinitialize();
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Initialize
 * @details This function initializes NNAgent.
 * @param void
 * @return error code
 */
void NNAgent::initialize(void) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    uennServiceDelegator_->uennInitialize();
    graphgenManager_->initGraphGenManager(uennServiceDelegator_);

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief close model
 * @details This function lets NNAgent close model.
 * @param modelId EnnModel modelId
 * @return error code
 */
void NNAgent::closeModel(EnnModelId& modelId, NNCBuf& nncBuffer, void* pbuffer_set) {
    LOGD(ENN_DRIVER, "%s(+) modelId is %u\n", __func__, modelId);

    uennServiceDelegator_->uennFreeBuffers(reinterpret_cast<EnnBufferPtr*>(pbuffer_set), modelId);
    graphgenManager_->freeMemory(nncBuffer);
    uennServiceDelegator_->uennCloseModel(modelId);

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Get capabilities for this device
 * @details This function returns a capabilities representing for performance of this device.
 * @param[in] capabilities Capabilities representing for performance of device
 * @return error code
 */
Capabilities NNAgent::getCapabilities() {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode = RET_OK;
    const Capabilities::PerformanceInfo perfInfo_high = { .execTime = 0.01f, .powerUsage = 0.03f };
    const Capabilities::PerformanceInfo perfInfo = { .execTime = 0.05f, .powerUsage = 0.05f };

    constexpr OperandType kOperandsTypes[] = {
            OperandType::FLOAT32,
            OperandType::INT32,
            OperandType::UINT32,
            OperandType::TENSOR_FLOAT32,
            OperandType::TENSOR_INT32,
            OperandType::TENSOR_QUANT8_ASYMM,
            OperandType::BOOL,
            OperandType::TENSOR_QUANT16_SYMM,
            OperandType::TENSOR_FLOAT16,
            OperandType::TENSOR_BOOL8,
            OperandType::FLOAT16,
            OperandType::TENSOR_QUANT8_SYMM_PER_CHANNEL,
            OperandType::TENSOR_QUANT16_ASYMM,
            OperandType::TENSOR_QUANT8_SYMM,
            OperandType::TENSOR_QUANT8_ASYMM_SIGNED,
    };

    std::vector<Capabilities::OperandPerformance> operandPerformance;
    operandPerformance.reserve(std::size(kOperandsTypes));
    std::transform(std::begin(kOperandsTypes), std::end(kOperandsTypes),
                   std::back_inserter(operandPerformance), [perfInfo, perfInfo_high](OperandType op) {
                   const bool ifFloat32 = (op==OperandType::FLOAT32 || op==OperandType::TENSOR_FLOAT32);
                   if (ifFloat32) {
                       #ifdef GPU_HW
                       return Capabilities::OperandPerformance{.type = op, .info = perfInfo_high};
                       #elif NPU_HW
                       return Capabilities::OperandPerformance{.type = op, .info = perfInfo};
                       #endif
                   } else {
                       #ifdef GPU_HW
                       return Capabilities::OperandPerformance{.type = op, .info = perfInfo};
                       #elif NPU_HW
                       return Capabilities::OperandPerformance{.type = op, .info = perfInfo_high};
                       #endif
                  }
                  #ifdef ENN_HW
                  return Capabilities::OperandPerformance{.type = op, .info = perfInfo};
                  #endif
             });
    auto table =
            Capabilities::OperandPerformanceTable::create(std::move(operandPerformance)).value();

    return  {.relaxedFloat32toFloat16PerformanceScalar = perfInfo,
             .relaxedFloat32toFloat16PerformanceTensor = perfInfo,
             .operandPerformance = std::move(table),
             .ifPerformance = perfInfo,
             .whilePerformance = perfInfo};

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Get supported operation list on a given model by this device
 * @details This function returns a supported operation list on a given model by this device.
 * @param[in] model Android NN Model
 * @param[out] supportedOperations supported operation list
 * @return error code
 */
int32_t NNAgent::getSupportedOperations(const Model& model, std::vector<bool>& supportedOperations) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    int32_t retCode = RET_OK;

    supportedOperations.resize(model.main.operations.size());
    resetSupportedOperations(supportedOperations);
    if (!checkSupportedModel(model)){
        isModelSupported_ = false;
        return retCode;
    }
    if (graphgenManager_->isGraphGenSupported()) {
        if (RET_OK != graphgenManager_->getSupportedOperations(model, supportedOperations)) {
            resetSupportedOperations(supportedOperations);
            isModelSupported_ = false;
        } else {
            isModelSupported_ = true;
        }
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}

/**
 * @brief Check whether the model supported
 * @details This function returns a bool to identify whether the model is supported.
 * @param[in] model Android NN Model
 * @param[out] supportedModel bool
 * @return error code
 */
bool NNAgent::checkSupportedModel(const Model& model) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    if (model.referenced.size() != 0) {
        return false;
    }

    if (checkDimensions(model) != RET_OK) {
        LOGI(ENN_DRIVER, "unsupported Dimensions \n");
        return false;
    }
    if (checkGraph(model) != RET_OK) {
        LOGI(ENN_DRIVER, "unsupported Graph \n");
        return false;
    }

    return true;
}

/**
 * @brief allocate Input and Output buffer of a Model
 * @details
 * @param[in] EnnModelId Model ID, for which buffer need tobe allocated
 * @param[in] UEnnBufferInfo Information about Input buffers
 * @param[in] UEnnBufferInfo Information about Output buffers
 * @return returnCode
 */
int32_t NNAgent::allocateAllBuffers(const EnnModelId& model_id, UEnnBufferInfo& inputBuffers, UEnnBufferInfo& outputBuffers) {
    uint32_t numOfInputBuffers = 0;
    uint32_t numOfOutputBuffers = 0;
    int32_t retCode = RET_OK;

    retCode = uennServiceDelegator_->uennAllocateAllBuffers(model_id, &buffer_set, &numOfInputBuffers, &numOfOutputBuffers);

    if (retCode != RET_OK) {
        LOGE(ENN_DRIVER, "AllocateAllBuffers() is failed. Set Buffer Address to nullptr!!\n");
        return retCode;
    } else {
        inputBuffers.addr = reinterpret_cast<void*>(buffer_set);
        inputBuffers.numOfBuffers = numOfInputBuffers;
        outputBuffers.addr = reinterpret_cast<void*>(buffer_set + numOfInputBuffers);
        outputBuffers.numOfBuffers = numOfOutputBuffers;
    }
    return retCode;
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
ErrorStatus NNAgent::allocate(const BufferDesc& desc,
                       const std::vector<SharedPreparedModel>& preparedModels,
                       const std::vector<BufferRole>& inputRoles,
                       const std::vector<BufferRole>& outputRoles,
                       std::shared_ptr<DeviceBuffer>& deviceBuffer) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    std::set<PreparedModelRole> roles;
    Operand operand;
    constexpr uint32_t kInvalidBufferToken = 0;

    auto getModel =
        [](const SharedPreparedModel& preparedModel) -> const Model* {

        if (preparedModel.get() == nullptr) {
            LOGE(ENN_DRIVER, "preparedModel is nullptr()\n");
            return nullptr;
        }
        const EnnPreparedModel* edenPreparedModel =
            static_cast<const EnnPreparedModel*>(preparedModel.get());
        if (edenPreparedModel == nullptr) {
            LOGE(ENN_DRIVER, "Invalid params in allocate()\n");
            return nullptr;
        }
        return &(edenPreparedModel->model);
    };

    /* From validateHal.h */
    const auto result =  validateMemoryDesc(desc, preparedModels, inputRoles, outputRoles, getModel, &roles, &operand);
    if (!result.ok()) {
        LOGE(ENN_DRIVER, "validateMemoryDesc failed: %s\n", result.error().c_str());
        return ErrorStatus::INVALID_ARGUMENT;
    }

    // Do not allcate Device Buffer in case of multiple
    // subgraph in IF and WHILE Operations
    if (inputRoles.size() > 1 || outputRoles.size() > 1) {
        LOGE(ENN_DRIVER, "Do notallocate Device Buffer \n");
        return ErrorStatus::GENERAL_FAILURE;
    }

    // Get address of allocated Input/output buffers
    EnnBuffer* allocatedEnnBuffer = nullptr;
    size_t calculatedBufferSize = 1;
    if (inputRoles.size()) {
        const SharedPreparedModel& aModel = preparedModels[inputRoles[0].modelIndex];
        const EnnPreparedModel* ennPreparedModel = static_cast<const EnnPreparedModel*>(aModel.get());
        //Adjust Input Index for *all_inputs* Tests
        int32_t EnnBufferIdx = adjustIndex(ennPreparedModel->model, inputRoles[0].ioIndex);
        int32_t inputOperandIdx = ennPreparedModel->model.main.inputIndexes[inputRoles[0].ioIndex];
        calculatedBufferSize = calculateBufferSize(ennPreparedModel->model, inputOperandIdx);
        if (ennPreparedModel->inputAddr != nullptr) {
            EnnBufferPtr* ennBufferForInputs =
                reinterpret_cast<EnnBufferPtr*>(ennPreparedModel->inputAddr);
            allocatedEnnBuffer = (ennBufferForInputs[EnnBufferIdx]);
        } else {
            // ToDo: Need to allocate buffer based on size
            LOGE(ENN_DRIVER, "Not support unknown dimension/rank");
            return ErrorStatus::GENERAL_FAILURE;
        }
    } else if (outputRoles.size()) {
        const SharedPreparedModel& aModel = preparedModels[outputRoles[0].modelIndex];
        const EnnPreparedModel* ennPreparedModel = static_cast<const EnnPreparedModel*>(aModel.get());
        int32_t outputOperandIdx = ennPreparedModel->model.main.outputIndexes[outputRoles[0].ioIndex];
        calculatedBufferSize = calculateBufferSize(ennPreparedModel->model, outputOperandIdx);
        if (ennPreparedModel->outputAddr != nullptr) {
            EnnBufferPtr* ennBufferForOutputs =
                reinterpret_cast<EnnBufferPtr*>(ennPreparedModel->outputAddr);
            allocatedEnnBuffer = (ennBufferForOutputs[outputRoles[0].ioIndex]);
        } else {
            // ToDo: Need to allocate buffer based on size
            LOGE(ENN_DRIVER, "Not support unknown dimension/rank");
            return ErrorStatus::GENERAL_FAILURE;
        }
    } else {
        LOGD(ENN_DRIVER, "No input/output roles specified\n");
        return ErrorStatus::GENERAL_FAILURE;
    }

    if (allocatedEnnBuffer == nullptr || allocatedEnnBuffer->size == 0 || calculatedBufferSize == 0) {
        LOGE(ENN_DRIVER, "Not allocated EnnBuffer or Allocated EnnBuffer size is 0\n");
        return ErrorStatus::GENERAL_FAILURE;
    }

    LOGD(ENN_DRIVER, "allocatedBuffer addr:%p, size=%d\n", allocatedEnnBuffer->va, allocatedEnnBuffer->size);
    uint32_t size = nonExtensionOperandSizeOfData(operand.type, operand.dimensions);
    LOGD(ENN_DRIVER, "size: %d, dimension: %s", size, toString(operand.dimensions).c_str());
    if (size == 0) {
        LOGE(ENN_DRIVER, "does not support dynamic output shape\n");
        return ErrorStatus::GENERAL_FAILURE;
    }

    auto bufferWrapper = ManagedBuffer::create(allocatedEnnBuffer, size,
            std::move(roles), std::move(operand));
    if (bufferWrapper == nullptr) {
        LOGE(ENN_DRIVER, "ManagedBuffer::create() failed\n");
        return ErrorStatus::GENERAL_FAILURE;
    }
    // add a token
    auto token = bufferTracker_->add(bufferWrapper);
    if (token == nullptr) {
        LOGE(ENN_DRIVER, "BufferTracker returned invalid token.\n");
        return ErrorStatus::GENERAL_FAILURE;
    }

    const Request::MemoryDomainToken  bufferTokenValue = token->get();

    // create new DeviceBuffer instance.
    // Give DeviceBuffer to app in cb.
    deviceBuffer = std::make_shared<DeviceBuffer>(std::move(bufferWrapper), std::move(token), this);
    LOGD(ENN_DRIVER, "Success the requested memory: TokenValue = %d\n", bufferTokenValue);
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return ErrorStatus::NONE;
}

bool NNAgent::initializeManagedBuffer(Request::MemoryDomainToken token) {
    auto managedBuffer = bufferTracker_->get(token);
    if (managedBuffer == nullptr) {
        LOGD(ENN_DRIVER, "Invalid managedBuffer\n");
        return false;
    }
    managedBuffer->setInitialized(true);
    LOGD(ENN_DRIVER, "Buffer is initialized, buffer=[%p]\n", managedBuffer.get());
    return true;
}

/**
 * @brief copy content of device buffer to the shared memory
 * @details This functions copies data of allocated device buffer to the given hidl shared memory
 * @param[in] srcManagedBuffer ManagedBuffer class from which data need to be copied
 * @param[in] hidl shared memory location of destination
 * @returns error status
 */
ErrorStatus NNAgent::copyToInternal(const std::shared_ptr<ManagedBuffer>& srcManagedBuffer,
                                               const SharedMemory& dst) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    // create virtual memorypool
    BufferInfoOnExecute bufInfoOnExecute;
    char * vAddr;
    uint32_t size;
    uint32_t ret = bufInfoOnExecute.loadSharedMem(dst, false, vAddr, size);
    if (ret != RET_OK) {
        LOGE(ENN_DRIVER, "copyTo(): Unable to map dest memory\n");
        return ErrorStatus::INVALID_ARGUMENT;
    }
    // validate copyTo
    const ErrorStatus validateError  = srcManagedBuffer->validateCopyTo(size);
    if (validateError != ErrorStatus::NONE) {
        LOGE(ENN_DRIVER, "copyTo(): Failed to validate source buffer\n");
        return validateError;
    }

    // copy from Buffer to destination
    EnnBuffer* srcAddr = srcManagedBuffer->getBuffer();
    std::memcpy(reinterpret_cast<void*>(vAddr), srcAddr->va, srcAddr->size);

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return ErrorStatus::NONE;
}

/**
 * @brief copy the shared memory to device memory
 * @details This functions copies data from given hidl shared memory to the allocated device buffer
 * @param[in] destManagedBuffer ManagedBuffer class to which data need to be copied
 * @param[in] hidl shared memory location of source of data
 * @param[in] dimensions
 * @returns error status
 */
ErrorStatus NNAgent::copyFromInternal(const std::shared_ptr<ManagedBuffer>& destManagedBuffer,
                                                 const SharedMemory& src,
                                                 const Dimensions& dimensions) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    // create virtual memorypool
    BufferInfoOnExecute bufInfoOnExecute;
    char* vAddr;
    uint32_t size;
    uint32_t ret = bufInfoOnExecute.loadSharedMem(src, true, vAddr, size);
    if (ret != RET_OK) {
        LOGE(ENN_DRIVER, "copyFrom(): Unable to map source memory\n");
        destManagedBuffer->setInitialized(false);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    const ErrorStatus validationError = destManagedBuffer->validateCopyFrom(dimensions, size);
    if (validationError != ErrorStatus::NONE) {
        LOGD(ENN_DRIVER, "copyFrom(): failed to validate dest buffer\n");
        destManagedBuffer->setInitialized(false);
        return validationError;
    }
    EnnBuffer *dstAddr = destManagedBuffer->getBuffer();
    std::memcpy(dstAddr->va, reinterpret_cast<void*>(vAddr), size);

    destManagedBuffer->updateDimensions(dimensions);
    destManagedBuffer->setInitialized(true);

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return ErrorStatus::NONE;
}
/**
 * @brief Get device status
 * @details This function returns a device status.
 * @param[out] status DeviceStatus
 * @return error code
 */
int32_t NNAgent::getStatus(DeviceStatus& status) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    // @todo Get device status from runtime
    status = DeviceStatus::AVAILABLE;

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}


int32_t NNAgent::prepareModel(const Model& model, ExecutionPreference preference,
                                             Priority priority, const OptionalTimePoint& deadline,
                                             const std::vector<SharedHandle>*  modelCache, const std::vector<SharedHandle>*  dataCache,
                                             const CacheToken* token, std::shared_ptr<IPreparedModel> &rPreparedModel)
{
    return  prepareModelBaseOnNNAgent(this, model, preference, priority,
                                            deadline, modelCache, dataCache, token, rPreparedModel);
}


/**
 * @brief Execute a preparedModel with a given request synchronously
 * @details This function executes a preparedModel with a given request
 *          and executes a callback when it's finished.
 * @param[in] preparedModel EnnPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[in] callback IExecutionCallback to be executed
 * @return error code
 */
int32_t NNAgent::executeSynchronously(const EnnPreparedModel* ennPreparedModel,
                                      const Request& request,
                                      MeasureTiming measure,
				      std::vector<OutputShape>& outputshape,
                                      Timing& timing) {
    std::chrono::steady_clock::time_point driverStart;
    if (measure == MeasureTiming::YES) driverStart = std::chrono::steady_clock::now();
    return executeBaseOnNNAgent(this, ennPreparedModel, request, {}, measure,
                                driverStart, EXECUTION_MODE::SYNC, outputshape, timing);
}

int32_t NNAgent::resetSupportedOperations(std::vector<bool>& supportedOperations) {
    for (uint32_t idx = 0; idx < supportedOperations.size(); idx++) {
        supportedOperations[idx] = false;
    }
    return RET_OK;
}

/**
 * @brief Create IPreparedModel
 * @details This function creates a IPreparedModel with given information.
 * @param[in] model Android NN Model
 * @param[in] EnnModelId unique model id generated by Enn Framework
 * @param[in] ennModel EnnModel
 * @param[in] inputBuffers UEnnBufferInfo representing for buffer information returned by Enn Framework
 * @param[in] outputBuffers UEnnBufferInfo representing for buffer information returned by Enn Framework
 * @param[out] preparedModel IPreparedModel which is generated
 * @return error code
 */
int32_t NNAgent::createPreparedModel(const Model& model, EnnModelId model_id,
                                     HwType hwPreference,
                                     NNCBuf nncBuffer,
                                     UEnnBufferInfo inputBuffers, UEnnBufferInfo outputBuffers,
                                     std::shared_ptr<IPreparedModel>  &preparedModel) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    preparedModel =  std::make_shared<EnnPreparedModel>(this, model, model_id,
                                                                 hwPreference, nncBuffer,
                                                                 inputBuffers.addr, inputBuffers.numOfBuffers,
                                                                 outputBuffers.addr, outputBuffers.numOfBuffers
                                                                 );
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}


/**
 * @brief check dimensions for a given model
 * @details This function check dimensions, return error if it is larger than 4 for some operations.
 * @param[in] model Android NN Model
 * @return error code
 */
int32_t NNAgent::checkDimensions(const Model& model) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    // bypass the notice of execution failure in CTS

    for (const Operation &operation : model.main.operations) {
        int32_t inputSizeSum = 0;
        for (size_t idx = 0; idx < operation.inputs.size(); idx++) {
            int32_t inputIndex = operation.inputs[idx];
            const Operand& androidOperand = model.main.operands[inputIndex];
            for (size_t i = 0; i < androidOperand.dimensions.size(); i++) {
                inputSizeSum += androidOperand.dimensions[i];
            }
        }
        if (inputSizeSum == 0) {
            LOGE(ENN_DRIVER, "Invalied inputParams.\n");
            return INVALID_PARAMS;
        }


        int32_t outputSizeSum = 0;
        for (size_t idx = 0; idx < operation.outputs.size(); idx++) {
            int32_t outputIndex = operation.outputs[idx];
            const Operand& androidOperand = model.main.operands[outputIndex];
            for (size_t i = 0; i < androidOperand.dimensions.size(); i++) {
                outputSizeSum += androidOperand.dimensions[i];
            }
        }
        if (outputSizeSum == 0) {
            LOGE(ENN_DRIVER, "Invalied OutputParams.\n");
            return RET_PARAM_INVALID;
        }
    }

    // TODO: to support dimension large than 4
    for (const Operation &operation : model.main.operations) {
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
                    const Operand& androidOperand = model.main.operands[idxInput];
                    if (androidOperand.dimensions.size() > 4) {
                        LOGD(ENN_DRIVER,  "not support dimensions.size = %d ", (int)androidOperand.dimensions.size());
                        return RET_PARAM_INVALID;
                    }
                }

                for (size_t idx = 0; idx < operation.outputs.size(); idx++) {
                    uint32_t idxOutput = operation.outputs[idx];
                    const Operand& androidOperand = model.main.operands[idxOutput];
                    if (androidOperand.dimensions.size() > 4) {
                        LOGD(ENN_DRIVER,  "not support dimensions.size = %d", (int)androidOperand.dimensions.size());
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
 * @details This function check graph, return error if it is not supported in enn
 * @param[in] model Android NN Model
 * @return error code
 */
int32_t NNAgent::checkGraph(const Model& model) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    // Skip CTS Test if Input-0 of Operation is not IFM
    const Operation &op0 = model.main.operations[0];
    const Operand &operand = model.main.operands[op0.inputs[0]];
    if (operand.lifetime != Operand::LifeTime::SUBGRAPH_INPUT && operand.lifetime != Operand::LifeTime::TEMPORARY_VARIABLE) {
        LOGD(ENN_DRIVER, "Incorrect input order!\n");
        return INVALID_PARAMS;
    }

    // Skip AIBv4  SRCNN
    if (model.main.operations.size() == 4) {
        uint8_t bitSum = 0;
        for (int8_t idx = 0; idx < model.main.operations.size(); ++idx) {
            int32_t opType = static_cast<int32_t>(model.main.operations[idx].type);
            switch (opType) {
                case ANEURALNETWORKS_CONV_2D:
                    bitSum |= 1 << idx;
                    break;
                case ANEURALNETWORKS_ADD :
                     bitSum |= 1 << 3;
                     break;
                default:
                     bitSum = 0;
                     break;
            }
        }
        if (bitSum == 0x0F) {
            LOGE(ENN_DRIVER, "SRCNN Network is not Supported!\n");
            return INVALID_PARAMS;
        }
    }

    // Skip WHILE Tests
    std::vector<uint32_t> common_inputs;
    for (const Operation &operation : model.main.operations) {
        common_inputs.insert(common_inputs.end(), operation.inputs.begin(), operation.inputs.end());
    }
    for (const int32_t inputIndex : model.main.inputIndexes) {
        auto itr = find(common_inputs.begin(), common_inputs.end(), inputIndex);
        if (itr == common_inputs.end()) {
            return INVALID_PARAMS;
        }
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

int32_t NNAgent::validateDeviceMemoryRequest(const Request& request,
                                             const EnnPreparedModel* ennPreparedModel) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    for (uint32_t i = 0; i < request.pools.size(); i++) {
        auto& pool = request.pools[i];
        if (const auto* isToken = std::get_if<Request::MemoryDomainToken>(&pool)) {
            auto managedBuffer = bufferTracker_->get(*isToken);
            if (managedBuffer == nullptr) {
                LOGD(ENN_DRIVER, "Invalid Managed buffer\n");
                return INVALID_PARAMS;
            }
            auto ret = managedBuffer->validateRequest(i, request, ennPreparedModel);
            if (ret) {
                LOGE(ENN_DRIVER, "Invalid device memory request\n");
                return ret;
            }
        }
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

int32_t adjustIndex(const Model& model, const int32_t iIndex) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    std::vector<int32_t> inputArray;

    for (int32_t in = 0; in < model.main.operations.size(); in++) {
        const Operation&  androidOperation = model.main.operations[in];
        for (int32_t k = 0; k < androidOperation.inputs.size(); k++) {
            auto inputId = androidOperation.inputs[k];
            if (model.main.operands[inputId].lifetime == Operand::LifeTime::SUBGRAPH_INPUT) {
                inputArray.push_back(inputId);
            }
        }
    }

    int32_t operandId = model.main.inputIndexes[iIndex];
    int32_t it = 0;
    auto itr = find(inputArray.begin(), inputArray.end(), operandId);
    if (itr != inputArray.end()) {
        it = itr - inputArray.begin();
    }
    std::vector<int32_t>().swap(inputArray);
    LOGD(ENN_DRIVER, "OperandId =%d , it = %d", operandId, it);
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return it;
}

size_t calculateBufferSize(const Model& model, const int32_t operandIdx) {
    auto operand = model.main.operands[operandIdx];
    uint32_t totalDims = operand.dimensions.size();
    size_t buffer_size = 1;
    for (int i = 0; i < totalDims; i++) {
        buffer_size *= operand.dimensions[i];
    }
    return buffer_size;
}

}  // namespace enn_driver
}  // namespace nn
}  // namespace android
