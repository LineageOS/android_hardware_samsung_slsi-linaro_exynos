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
 * @file    CompilerManager.h
 * @brief   This is CompilerManager class file.
 * @details This header defines CompilerManager class.
 *          This class is implementing handshaking with compiler component.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_COMPILERMANAGER_H_
#define DRIVER_COMPILERMANAGER_H_

#include <vector>
#include <cstdint>  // int32_t
#include <memory>   // shared_ptr

#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
#include "Common.h"         // DATA_TYPE
#include "EdenModelConvertLib.h"
#include "./npuc/include/npuc/capability/NPUCtype.h"

#include "./npuc/include/npuc/NPUcompiler.h"

namespace NPUC {
class NPUCompiler;
}

namespace android {
namespace nn {
namespace eden_driver {

class NpucIrConverter;

class CompilerManager {
 public:
    CompilerManager();
    int32_t getPerformanceInfo(DATA_TYPE type, V1_0::PerformanceInfo& perfInfo);
    int32_t getSupportedOperations(const V1_3::Model& model, const std::vector<std::shared_ptr<void>>& constraints, std::vector<bool>& supportedOperations);
    int32_t getConstraints(std::vector<std::shared_ptr<void>>& constraints);
    int32_t compile(const V1_3::Model& model, ModelInfo& modelInfo, const std::vector<int32_t>& operationList, void** buffer, int32_t* bufferSize);
    int32_t getInputOffset();
    int32_t getOutputOffset();
    void getNpucSOCType(NPUC::SOCType& npuSocType);
    void initNPUCCompiler();
    bool isSupportedModelByNPUC(const V1_3::Model& model);
    void clearNcpBuffer(void* ncpBuffer);
    std::shared_ptr<NPUC::NPUCompiler> getNpuCompiler() { return npuCompiler_; }

 private:
    bool verifyConstraints(const V1_3::Model& model, const V1_3::Operation& operation, int32_t opType, const std::vector<std::shared_ptr<void>>& constraints);

    std::shared_ptr<NpucIrConverter> npucIrConverter_;
    std::shared_ptr<NPUC::NPUCompiler> npuCompiler_;
    int32_t inputOffset_;
    int32_t outputOffset_;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_COMPILERMANAGER_H_
