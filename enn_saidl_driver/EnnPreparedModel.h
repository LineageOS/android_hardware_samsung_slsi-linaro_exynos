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
 * @file    EnnPreparedModel.h
 * @brief   This is EnnPreparedModel class file.
 * @details This header implements IPreparedModel interface.
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#ifndef ENNDRIVER_ENNPREPAREDMODEL_H_
#define ENNDRIVER_ENNPREPAREDMODEL_H_

#include <map>
#include <vector>
#include <cstdint>  // int32_t

#include <nnapi/IPreparedModel.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include "../enn/include/enn_api-type.h"
#include "BufferInfoOnExecute.h"
#include "GraphGenManager.h"

using namespace android::nn;

namespace android {
namespace nn {
namespace enn_driver {


class NNAgent;

class EnnPreparedModel final : public IPreparedModel,
                                public std::enable_shared_from_this<EnnPreparedModel> {
 public:
    explicit EnnPreparedModel(NNAgent* nnAgent,
                               const Model& model,  EnnModelId modelId,
                               HwType hwPreference, NNCBuf nncBuffer,
                               void* inputAddr, int32_t inputNumOfBuffers,
                               void* outputAddr, int32_t outputNumOfBuffers
                               );
    ~EnnPreparedModel() override;

    ExecutionResult<std::pair<std::vector<OutputShape>, Timing>>  execute(const Request& request,
                               MeasureTiming measure,
                               const OptionalTimePoint& deadline,
	                       const OptionalDuration& loopTimeoutDuration,
                               const std::vector<nn::TokenValuePair>& hints,
                               const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const override;


    GeneralResult<std::pair<SyncFence, ExecuteFencedInfoCallback>> executeFenced(
            const Request& request, const std::vector<SyncFence>& waitFor, MeasureTiming measure,
            const OptionalTimePoint& deadline, const OptionalDuration& loopTimeoutDuration,
            const OptionalDuration& timeoutDurationAfterFence,
            const std::vector<nn::TokenValuePair>& hints,
            const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const override;

    GeneralResult<nn::SharedExecution> createReusableExecution(
            const Request& request, MeasureTiming measure,
            const OptionalDuration& loopTimeoutDuration,
            const std::vector<nn::TokenValuePair>& hints,
            const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const override;

    GeneralResult<SharedBurst> configureExecutionBurst() const override;

    std::any getUnderlyingResource() const override;

    // Internal public functions
    int32_t loadInputData(const Request& request, BufferInfoOnExecute& bufInfoOnExecute) const ;

    // Setter
    void updateInputBuffers(void* addr, int32_t numOfBuffers);
    void updateOutputBuffers(void* addr, int32_t numOfBuffers);
    EnnPreparedModel *getEnnPreparedModel() { return this;}
/* ToDo: nihar.desai: Check below code is needed or not
    // Getter
    bool readyToAllocateInputBuffers(const Request* request);
    bool readyToAllocateOutputBuffers(constRequest* request);
*/
    bool needToAllocateInputBuffers() const { return needInputBuffers_; }
    bool needToAllocateOutputBuffers() const { return needOutputBuffers_; }

    bool initializeManagedBuffer(Request::MemoryDomainToken token) const;
    bool IsInvalidParams() const ;

    Model model;

    EnnModelId modelId;
    HwType hwPreference;
    void* inputAddr;  // EnnBufferPtr*
    int32_t inputNumOfBuffers;
    void* outputAddr;  // EnnBufferPtr*
    int32_t outputNumOfBuffers;

    std::vector<int32_t> updatedOperations;

 private:
    int32_t loadDefaultDataOnTargetBuffer(int32_t inputIdx) const ;
    int32_t loadDataOnTargetBuffer(char* srcAddr, int32_t length, int32_t inputIdx) const;
    void show() const;

    NNAgent* nnAgent_;
    NNCBuf nncBuffer_;

    bool needInputBuffers_;
    bool needOutputBuffers_;
};


}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // ENNDRIVER_ENNPREPAREDMODEL_H_
