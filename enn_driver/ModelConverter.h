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
 * @file    ModelConverter.h
 * @brief   This is ModelConverter class file.
 * @details This header defines ModelConverter class.
 *          This class is implementing model converting from Android NN Model to EdenModel.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_MODELCONVERTER_H_
#define DRIVER_MODELCONVERTER_H_

#include <vector>
#include <set>
#include <cstdint>  // int32_t

#include "HalInterfaces.h"        // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
#include "EdenModelConvertLib.h"  // ModelInfo etc
#include "GraphGenManager.h"

#include "../include/eden_model.h"

using namespace eden::nn;

namespace android {
namespace nn {
namespace eden_driver {

class CompilerManager;
class GraphGenManager;

enum {
    TARGET_DEVICE_NPU = 0,
    TARGET_DEVICE_GPU = 1,
    TARGET_DEVICE_CPU = 2,
#ifdef ENABLE_DSP
    TARGET_DEVICE_DSP = 3,
#endif
};

class ModelConverter {
 public:
    void setCompilerManager(std::shared_ptr<CompilerManager> compilerManager);
    void setGraphGenManager(std::shared_ptr<GraphGenManager> graphgenManager);
    void setNPUInfo(std::vector<OperationInfo>& vecNPUOperationInfos);
    void setConstantCopy(std::shared_ptr<int8_t>& constantCopyAddr);
    int32_t convertToNNC(const V1_3::Model& model, NNCBuf& nncBuffer, NNCOpInfo& nncOpInfo, ModelInfo& modelInfo);
    int32_t convert(const V1_3::Model& model, EdenModel*& edenModel, ModelInfo& modelInfo);

 private:
    void convertOperandToNCHW(ModelInfo& modelInfo, const V1_3::Operand& operand);
    int32_t convertToNCHW(const V1_3::Model& model, ModelInfo& modelInfo, const std::vector<int32_t>& operationList);

    void getOperandsExceptNPU(ModelInfo& modelInfo, std::set<int32_t>& allOperandsExceptNPU);
    void getOperationsExceptNPU(ModelInfo& modelInfo, std::vector<int32_t>& allOperationsExceptNPU);
    int32_t getNumOfOperationForNPU(ModelInfo& modelInfo);
    void getInputOperandIndexes(NNSubOperationList& subOperationList, std::vector<int32_t>& inputIndexes);
    void getOutputOperandIndexes(NNSubOperationList& subOperationList, std::vector<int32_t>& outputIndexes);

    int32_t divideOperationList(const V1_3::Model& model, const ModelInfo* modelInfo, std::vector<NNSubOperationList>& vecOpGroup);
    int32_t buildOpGroup(const V1_3::Model& model, int32_t offset, std::vector<bool>& supportedByNPU, std::vector<NNSubOperationList>& vecOpGroup);
    bool supportedByNPU(const V1_3::Model& model, const ModelInfo* modelInfo, const V1_3::Operation& androidOperation);

    int32_t createEdenCustomOpForNCP(const V1_3::Model& model, ModelInfo& modelInfo, NNSubOperationList& subOperationList, EdenOperation* edenOperation,
                                     EdenModel* edenModel);
    int32_t createEdenCustomOpForNormalization(const std::vector<int32_t>& vecMeans, const std::vector<int32_t>& vecScales, EdenOperand*& edenOperand);
    int32_t createEdenCustomOpForQuantization(const std::vector<int32_t>& vecFracLens, EdenOperand*& edenOperand);
    int32_t createEdenCustomOpForDequantization(const std::vector<int32_t>& vecFracLens, EdenOperand*& edenOperand);

    int32_t clearMaps();

    std::shared_ptr<int8_t> userConstantCopyAddr_;

    std::vector<OperationInfo> vecNPUOperationInfos_;

    std::shared_ptr<CompilerManager> compilerManager_;
    std::shared_ptr<GraphGenManager> graphgenManager_;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_MODELCONVERTER_H_

