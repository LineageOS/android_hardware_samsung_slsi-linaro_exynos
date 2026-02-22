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
 * @file    DspCompilerManager.cpp
 * @brief   This is DspCompilerManager class file.
 * @details This header defines DspCompilerManager class.
 *          This class is implementing handshaking with compiler component.
 */

#include <iostream>
#include "log.h"
#include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "DspCompilerManager.h"
#include "DspPreparedModel.h"

#include "OfiAnnCompiler.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::DspCompilerManager"

namespace android {
namespace nn {
namespace eden_driver {

DspCompilerManager::DspCompilerManager() {
    LOGD(EDEN_DRIVER, "%s()\n", __func__);
}

int32_t DspCompilerManager::getSupportedOperations(const V1_3::Model& model, const std::vector<std::shared_ptr<void>>& constraints,
                                                std::vector<bool>& supportedOperations) {
    (void) constraints;
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    for (const V1_3::Operation& androidOperation : model.main.operations) {
        supportedOperations[static_cast<int32_t>(androidOperation.type)] = true;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

int32_t DspCompilerManager::isSupportedModel(const V1_3::Model& model) {
    std::unique_ptr<ofi::OfiAnnCompiler> ofic(new ofi::OfiAnnCompiler());
    if (ofic->isSupportedModel(model) == false) {
        LOGE(EDEN_DRIVER, "%s() is false\n", __func__);
        return UNSUPPORTED_OPERATION;
    }
    return RET_OK;
}

int32_t DspCompilerManager::compile(const V1_3::Model& model, V1_3::IPreparedModel** preparedModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::unique_ptr<ofi::OfiAnnCompiler> ofic(new ofi::OfiAnnCompiler());
    char *buffer = nullptr;
    unsigned int size = 0;
    if(ofic->Compile(model, buffer, size) == false) {
        LOGE(EDEN_DRIVER, "%s(-): Compile is failed\n", __func__);
        return FAIL_ON_DSP_COMPILER;
    }
    if(buffer == nullptr) {
        LOGE(EDEN_DRIVER, "%s(-) Compiled, but cgo was not generated.\n", __func__);
        return FAIL_ON_DSP_COMPILER;
    }

    reinterpret_cast<DspPreparedModel*>(*preparedModel)->setPreparedModel(buffer, size);
    delete[] buffer;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

