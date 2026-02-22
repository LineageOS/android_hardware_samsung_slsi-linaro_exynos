/*
 * Copyright (C) 2020 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/**
 * @file    GraphGenManager.cpp
 * @brief   This is GraphGenManager class file.
 * @details This header defines GraphGenManager class.
 *          This class is implementing handshaking with on-deivce GraphGen component.
 * @author  hjjun.lee@samsung.com
 */

#include <iostream>
#include <string>                   // string -> will be deleted when GraphGen API changed
// #include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "GraphGenManager.h"
#include "EnnServiceDelegatorLib.h"

#include "graphgen/include/graphgen.h"
#include "../eden/include/log.h"
#include "../eden/include/eden_types.h"


#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::GraphGenManager"

namespace android {
namespace nn {
namespace eden_driver {


void setDeviceType(const V1_3::Model& model, graphgen::GraphGenDev& DeviceType);
/**
 * @brief GraphGenManager constructor
 * @details This function creates GraphGen instances
 * @param void
 */
GraphGenManager::GraphGenManager() {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    graphgen_ = std::make_shared<graphgen::GraphGen>();
    isSupported = false;
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Initialize on-device GraphGen
 * @details Initialize on-device GraphGen and check that if GraphGen can be supported
 * @param[in] ennServiceDelegator ennServiceDelegator instance to use ennGetEdenVersion()
 * @return error code
 */
int32_t GraphGenManager::initGraphGenManager(std::shared_ptr<EnnServiceDelegator> ennServiceDelegator) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    ennServiceDelegator_ = ennServiceDelegator;

    int32_t ret = graphgen_->getStatus();
    if (ret != graphgen::GRAPHGEN_NORMAL) {
        LOGE(EDEN_DRIVER, "%s(-) GraphGenState is not GRAPHGEN_NORMAL : %d\n", __func__, ret);
        isSupported = false;
        return GRAPHGEN_STATUS_ERROR;
    }

    uint32_t modelId = 0;
    int32_t versions[VERSION_MAX] = {0, };
    ret = ennServiceDelegator_->ennGetEdenVersion(modelId, versions);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) Fail to GetEdenVersion : %d\n", __func__, ret);
        return ret;
    }

    int32_t socType = versions[PLATFORM_VERSION_SOC];
    LOGD(EDEN_DRIVER, "SOC Type = %d\n", socType);

    isSupported = graphgen_->isSupportedSoC(socType);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Check that on-device GraphGen is supported
 * @details Check that on-device GraphGen is supported
 * @param void
 * @return True if GraphGen suuports given soc
 */
bool GraphGenManager::isGraphGenSupported() {
    return isSupported;
}

/**
 * @brief Check that on-device GraphGen can support specific ANN Model
 * @details Check that on-device GraphGen can support specific ANN Model
 * @param[in] model Android NN model
 * @return True if GraphGen suuports given ANN Model
 */
bool GraphGenManager::isSupportedModel(const V1_3::Model& model) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (graphgen_->getStatus() != graphgen::GRAPHGEN_NORMAL) {
        LOGE(EDEN_DRIVER, "%s(-) GraphGenState is not GRAPHGEN_NORMAL\n", __func__);
        isSupported = false;
        return false;
    }

    bool ret = graphgen_->isSupportedANNModel(model);
    if (ret) {
        LOGD(EDEN_DRIVER, "GraphGen can support the model\n");
    } else {
        LOGD(EDEN_DRIVER, "GraphGen can't support the model\n");
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);

    return ret;
}

/**
 * @brief Request to generate NNC with ANN model
 * @details Request to generate NNC with ANN model
 * @param[in] model Android NN model
 * @param[out] buffer Memory pointer that contains NNC
 * @param[out] bytes Size of buffer
 * @return error code
 */
int32_t GraphGenManager::generateNNC(const V1_3::Model& model, NNCBuf& buffer, NNCOpInfo& nncOpInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    int32_t retCode = RET_OK;

    if (graphgen_->getStatus() != graphgen::GRAPHGEN_NORMAL) {
        LOGE(EDEN_DRIVER, "%s(-) GraphGenState is not GRAPHGEN_NORMAL\n", __func__);
        isSupported = false;
        return GRAPHGEN_STATUS_ERROR;
    }

    uint32_t modelId = 0;
    int32_t versions[VERSION_MAX] = {0, };
    retCode = ennServiceDelegator_->ennGetEdenVersion(modelId, versions);
    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) Fail to GetEdenVersion : %d\n", __func__, retCode);
        return retCode;
    }

    int32_t socType = versions[PLATFORM_VERSION_SOC];
    graphgen::NNCBuffer tempBuffer;
    graphgen::NNCInfo nnc_info;

    graphgen::GraphGenDev DeviceType;
    setDeviceType(model, DeviceType);
    graphgen::GraphGenResult ret = graphgen_->generate(model, DeviceType, socType, &tempBuffer, &nnc_info);
    if (ret != graphgen::GRAPHGEN_SUCCESS) {
        LOGE(EDEN_DRIVER, "fail to generate NNC : %d\n", ret);
        retCode = FAIL_ON_GENERATE_NNC;
    }

    LOGD(EDEN_DRIVER, "NNC buffer is generated. addr : %p, size : %d \n", tempBuffer.addr, tempBuffer.size);

    buffer.addr = static_cast<int8_t*>(static_cast<void*>(tempBuffer.addr));
    buffer.size = static_cast<int32_t>(tempBuffer.size);

    nncOpInfo.hasNpuOps = nnc_info.contain_npu_op;
    nncOpInfo.hasDspOps = nnc_info.contain_dsp_op;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}


void setDeviceType(const V1_3::Model& model, graphgen::GraphGenDev& DeviceType) {
    const V1_3::Operation  first_operation = model.main.operations[0];
    int32_t input_operand_id = first_operation.inputs[0];
    const V1_3::Operand& input_operand = model.main.operands[input_operand_id];
    if( input_operand.type == V1_3::OperandType::TENSOR_QUANT8_ASYMM) {
        DeviceType = graphgen::GraphGenDev::NPU_DSP;
    }
    else {
        DeviceType = graphgen::GraphGenDev::CPU_GPU;
    }
}

/**
 * @brief Free NNC buffer memory
 * @details Free NNC buffer memory
 * @param[in] buffer Memory pointer that contains NNC
 * @return void
 */
void GraphGenManager::freeMemory(NNCBuf& buffer) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (graphgen_->getStatus() != graphgen::GRAPHGEN_NORMAL) {
        LOGE(EDEN_DRIVER, "%s(-) GraphGenState is not GRAPHGEN_NORMAL\n", __func__);
        isSupported = false;
        return;
    }

    if (buffer.addr == nullptr || buffer.size <= 0) {
        LOGE(EDEN_DRIVER, "%s(-) Invalid NNC buffer\n", __func__);
        return;
    }

    LOGD(EDEN_DRIVER, "buffer info. addr : %p, size : %d \n", buffer.addr, buffer.size);

    graphgen::NNCBuffer tempBuffer;
    tempBuffer.addr = static_cast<unsigned char*>(static_cast<void*>(buffer.addr));
    tempBuffer.size = static_cast<unsigned int>(buffer.size);

    graphgen_->freeMemory(&tempBuffer);

    buffer.addr = nullptr;
    buffer.size = 0;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

