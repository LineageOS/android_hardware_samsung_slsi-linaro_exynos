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
 * @file    NNAgent.h
 * @brief   This is NNAgent class file.
 * @details This header defines NNAgent class.
 *          This class is implementing NNAgent which is Facade.
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#ifndef ENNDRIVER_NNAGENT_H_
#define ENNDRIVER_NNAGENT_H_

#include <iostream>
#include <vector>
#include <map>
#include <cstdint>  // int32_t
#include <memory>   // shared_ptr

#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include "EnnPreparedModel.h"
#include "GraphGenManager.h"
#include "ExecutionScheduler.h"
#include "Common.h"  // OperationInfo
#include "EnnDevice.h"

class GraphGenManager;
class ExecutionScheduler;
class UEnnServiceDelegator;
class ManagedBuffer;
class BufferTracker;

using namespace ::android::nn;

namespace android {
namespace nn {
namespace enn_driver {


typedef struct __UEnnBufferInfo {
    void* addr;
    int32_t numOfBuffers;
} UEnnBufferInfo;

class NNAgent {
 public:
    explicit NNAgent(HwType hwtype);
    ~NNAgent();

    void initialize();
    void closeModel(EnnModelId& modelId, NNCBuf& nncBuffer, void* pbuffer_set);

    Capabilities getCapabilities();
    int32_t getSupportedOperations(const Model& model, std::vector<bool>& supportedOperations);
    bool checkSupportedModel(const Model& model);

    int32_t prepareModel(const Model& model, ExecutionPreference preference,
                                             Priority priority, const OptionalTimePoint& deadline,
                                             const std::vector<SharedHandle>*  modelCache, const std::vector<SharedHandle>*  dataCache,
                                             const CacheToken* token,
                                             std::shared_ptr<IPreparedModel> &rPreparedModel);

    ErrorStatus allocate(const BufferDesc& desc,
                  const std::vector<SharedPreparedModel>& preparedModels,
                  const std::vector<BufferRole>& inputRoles,
                  const std::vector<BufferRole>& outputRoles,
		  std::shared_ptr<DeviceBuffer>& deviceBuffer);

    bool initializeManagedBuffer(Request::MemoryDomainToken token);
    ErrorStatus copyToInternal(const std::shared_ptr<ManagedBuffer>& srcManagedBuffer,
                                          const SharedMemory& dst);
    ErrorStatus copyFromInternal(const std::shared_ptr<ManagedBuffer> &destManagedBuffer,
                                            const SharedMemory& src,
                                            const Dimensions& dimensions);

    int32_t getStatus(DeviceStatus& status);

    int32_t executeSynchronously(const EnnPreparedModel* ennPreparedModel,
                                 const Request& request,
                                 MeasureTiming measure,
                                 std::vector<OutputShape>& outputshape,
                                 Timing& timing);
#if 0
    int32_t executeFenced(EnnPreparedModel* ennPreparedModel,
                          const Request& request,
                          const hidl_vec<hidl_handle>& waitFor,
                          V1_2::MeasureTiming measure,
                          hardware::neuralnetworks::IPreparedModel::executeFenced_cb cb);
#endif

  //private:
    int32_t resetSupportedOperations(std::vector<bool>& supportedOperations);
    int32_t querySupportedOperations(const Model& model, int32_t targetDevice,
                                     const std::vector<std::shared_ptr<void>>& constraints,
                                     std::vector<bool>& supportedOperations);

    int32_t allocateAllBuffers(const EnnModelId& modelId, UEnnBufferInfo& inputBuffers, UEnnBufferInfo& outputBuffers);
    int32_t createPreparedModel(const Model& model, EnnModelId modelId,
                                HwType hwPreference,
                                NNCBuf nncBuffer,
                                UEnnBufferInfo inputBuffers, UEnnBufferInfo outputBuffers,
                                std::shared_ptr<IPreparedModel> &preparedModel);

    int32_t checkDimensions(const Model& model);
    int32_t checkGraph(const Model& model);
    int32_t validateDeviceMemoryRequest(const Request& request,
                                        const EnnPreparedModel* ennPreparedModel);
    std::shared_ptr<ModelConverter> modelConverter_;
    std::shared_ptr<GraphGenManager> graphgenManager_;
    std::shared_ptr<ExecutionScheduler> executionScheduler_;
    std::shared_ptr<UEnnServiceDelegator> uennServiceDelegator_;
    std::shared_ptr<BufferTracker> bufferTracker_;
    HwType hwtype_;
    bool isModelSupported_ = false;
    EnnBufferPtr *buffer_set = nullptr;
    const std::string model_file = "/data/vendor/enn/ann_.nnc";

    friend int32_t prepareNNCModel(NNAgent* nnAgent, const Model& model,
                                   NNCBuf& nncBuffer,
                                   EnnModelId *modelId, HwType& hwPreference,
                                   UEnnBufferInfo& inputBuffers, UEnnBufferInfo& outputBuffers);


    friend int32_t prepareModelBaseOnNNAgent(NNAgent* nnAgent, const Model& model, ExecutionPreference preference,
                                             Priority priority, const OptionalTimePoint& deadline,
                                             const std::vector<SharedHandle>*  modelCache, const std::vector<SharedHandle>*  dataCache,
                                             const CacheToken* token,
                                             std::shared_ptr<SharedPreparedModel> &rPreparedModel);

    friend int32_t executeBaseOnNNAgent(NNAgent* nnAgent,
                                        const EnnPreparedModel* ennPreparedModel,
                                        const Request& request,
                                        const std::vector<SyncFence>& waitFor,
                                        MeasureTiming measure,
                                        std::chrono::steady_clock::time_point driverStart,
                                        EXECUTION_MODE executionMode);
};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // ENNDRIVER_NNAGENT_H_
