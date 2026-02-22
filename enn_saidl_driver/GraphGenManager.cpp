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
 * @file    GraphGenManager.cpp
 * @brief   This is GraphGenManager class file.
 * @details This header defines GraphGenManager class.
 *          This class is implementing handshaking with on-deivce GraphGen component.
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#include <iostream>
#include <string>                   // string -> will be deleted when GraphGen API changed
#include <fstream>
#include <vector>

#include "GraphGenManager.h"
//#include "UEnnServiceDelegatorLib.h"

#include "graphgen/include/graphgen.h"
#include "log.h"
#include "../enn/include/enn_api-type.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EnnDriver::GraphGenManager"

using namespace ::android::nn;

namespace android {
namespace nn {
namespace enn_driver {

static graphgen::GraphGenDev SelectHardware(const Model& model, std::vector<int32_t> npu_op_list) {
    for(auto &op : model.main.operations) {
        int32_t opType = static_cast<int32_t>(op.type);
        OperandType inputDataType = model.main.operands[model.main.inputIndexes[0]].type;
        std::vector<int>::iterator it = std::find(npu_op_list.begin(), npu_op_list.end(), opType);
        if (it != npu_op_list.end() &&
                     (inputDataType == OperandType::TENSOR_QUANT8_ASYMM ||
                      inputDataType == OperandType::TENSOR_QUANT8_ASYMM_SIGNED)) {
            continue;
        }
        else {
            return graphgen::CPU_GPU;
        }
    }
    return graphgen::NPU_DSP;
}

static int32_t getSocType() {
    int len = 0;
    char  socString[20];
    int32_t socType = -1;
    int fd = open("/sys/devices/soc0/soc_id", O_RDONLY);
    if( -1 == fd) {
        LOGE(ENN_DRIVER, "[SYSFS] Failed to open /sys/devices/soc0/soc_id.\n");
        return socType;
    }
    int ret = read(fd, socString, sizeof(socString));
    close(fd);
    if(-1 == ret) {
        LOGE(ENN_DRIVER, "[SYSFS] Reading soc id failed.\n");
        return socType;
    }

    socString[ret-1] = '\0';
    len = strlen(socString);
    LOGD(ENN_DRIVER, "SoC String : %s %d\n", socString,len);
    if(0 == strncmp(socString,"S5E9925",len-1)) {
        socType = 9925;
    } else if (0 == strncmp(socString,"S5E9935",len-1)) {
        socType = 9935;
    } else if (0 == strncmp(socString,"S5E8835",len-1)) {
        socType = 8835;
    } else {
       LOGE(ENN_DRIVER, "Device not supported");
    }
    return socType;
}
/**
 * @brief GraphGenManager constructor
 * @details This function creates GraphGen instances
 * @param void
 */
GraphGenManager::GraphGenManager() {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    graphgen_ = std::make_shared<graphgen::GraphGen>();
    isSupported = false;
    socType_ = -1;
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Initialize on-device GraphGen
 * @details Initialize on-device GraphGen and check that if GraphGen can be supported
 * @param[in] ennServiceDelegator ennServiceDelegator instance to use ennGetEnnVersion()
 * @return error code
 */
int32_t GraphGenManager::initGraphGenManager(std::shared_ptr<UEnnServiceDelegator> uennServiceDelegator) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    uennServiceDelegator_ = uennServiceDelegator;

    int32_t ret = graphgen_->getStatus();
    if (ret != graphgen::GRAPHGEN_NORMAL) {
        LOGE(ENN_DRIVER, "%s(-) GraphGenState is not GRAPHGEN_NORMAL : %d\n", __func__, ret);
        isSupported = false;
        return GRAPHGEN_STATUS_ERROR;
    }

    socType_ = getSocType();
    if(-1 == socType_) {
        LOGE(ENN_DRIVER, "UnSupported SoC");
        return INVALID_TARGET_DEVICE;
    }
    LOGD(ENN_DRIVER, "SOC Type = %d\n", socType_);

    isSupported = graphgen_->isSupportedSoC(socType_);

    // CreateNpuSupported Operation List
    // Run All Test on GPU for 8835 SoC.
    if (socType_ != 8835) {
        npu_op_list.push_back(static_cast<int32_t>(ANEURALNETWORKS_CONV_2D));
        npu_op_list.push_back(static_cast<int32_t>(ANEURALNETWORKS_DEPTHWISE_CONV_2D));
        npu_op_list.push_back(static_cast<int32_t>(ANEURALNETWORKS_TRANSPOSE_CONV_2D));
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
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
int32_t GraphGenManager::getSupportedOperations(const Model& model, std::vector<bool> &supportedOperations) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    if (graphgen_->getStatus() != graphgen::GRAPHGEN_NORMAL) {
        LOGE(ENN_DRIVER, "%s(-) GraphGenState is not GRAPHGEN_NORMAL\n", __func__);
        isSupported = false;
        return GRAPHGEN_STATUS_ERROR;
    }
    bool *supportedOpList;
    supportedOpList = new bool[model.main.operations.size()];
#ifdef GPU_HW
    graphgen::GraphGenDev devType = graphgen::CPU_GPU;
#elif NPU_HW
    graphgen::GraphGenDev devType = graphgen::NPU_DSP;
#elif ENN_HW
    graphgen::GraphGenDev devType = SelectHardware(model, npu_op_list);
#endif

    int32_t retCode = RET_OK;
    bool ret = graphgen_->getSupportedOperations(model, devType, socType_, supportedOpList);

    supportedOperations.resize(model.main.operations.size());
    for (size_t idx = 0; idx < model.main.operations.size(); idx++) {
        supportedOperations.at(idx) = supportedOpList[idx];
        if (supportedOpList[idx] == false) {
            LOGW(ENN_DRIVER, "Operation Not Supported\n");
            retCode = MODEL_FULLY_NOT_SUPPORTED;
            break;
        }
    }
    delete[] supportedOpList;
    if (ret != true) {
        LOGE(ENN_DRIVER, "GetSupporedOperations Failed\n");
        retCode = GRAPHGEN_STATUS_ERROR;
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);

    return retCode;
}

/**
 * @brief Request to generate NNC with ANN model
 * @details Request to generate NNC with ANN model
 * @param[in] model Android NN model
 * @param[out] buffer Memory pointer that contains NNC
 * @param[out] bytes Size of buffer
 * @return error code
 */
int32_t GraphGenManager::generateNNC(const Model& model, NNCBuf& buffer, NNCOpInfo& nncOpInfo) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    int32_t retCode = RET_OK;

    if (graphgen_->getStatus() != graphgen::GRAPHGEN_NORMAL) {
        LOGE(ENN_DRIVER, "%s(-) GraphGenState is not GRAPHGEN_NORMAL\n", __func__);
        isSupported = false;
        return GRAPHGEN_STATUS_ERROR;
    }

#ifdef GPU_HW
    graphgen::GraphGenDev devType = graphgen::CPU_GPU;
#elif NPU_HW
    graphgen::GraphGenDev devType = graphgen::NPU_DSP;
#elif ENN_HW
    graphgen::GraphGenDev devType = SelectHardware(model, npu_op_list);
#endif
    graphgen::NNCBuffer tempBuffer;
    graphgen::NNCInfo nnc_info;

    graphgen::GraphGenResult ret = graphgen_->generate(model, devType, socType_, &tempBuffer, &nnc_info);

    if (ret != graphgen::GRAPHGEN_SUCCESS) {
        LOGE(ENN_DRIVER, "fail to generate NNC : %d\n", ret);
        retCode = FAIL_ON_GENERATE_NNC;
    }

    LOGD(ENN_DRIVER, "NNC buffer is generated. addr : %p, size : %d \n", tempBuffer.addr, tempBuffer.size);

    buffer.addr = static_cast<int8_t*>(static_cast<void*>(tempBuffer.addr));
    buffer.size = static_cast<int32_t>(tempBuffer.size);

    nncOpInfo.hasNpuOps = nnc_info.contain_npu_op;
    nncOpInfo.hasDspOps = nnc_info.contain_dsp_op;

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return retCode;
}

/**
 * @brief Free NNC buffer memory
 * @details Free NNC buffer memory
 * @param[in] buffer Memory pointer that contains NNC
 * @return void
 */
void GraphGenManager::freeMemory(NNCBuf& buffer) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    if (graphgen_->getStatus() != graphgen::GRAPHGEN_NORMAL) {
        LOGE(ENN_DRIVER, "%s(-) GraphGenState is not GRAPHGEN_NORMAL\n", __func__);
        isSupported = false;
        return;
    }

    if (buffer.addr == nullptr || buffer.size <= 0) {
        LOGE(ENN_DRIVER, "%s(-) Invalid NNC buffer\n", __func__);
        return;
    }

    LOGD(ENN_DRIVER, "buffer info. addr : %p, size : %d \n", buffer.addr, buffer.size);

    graphgen::NNCBuffer tempBuffer;
    tempBuffer.addr = static_cast<unsigned char*>(static_cast<void*>(buffer.addr));
    tempBuffer.size = static_cast<unsigned int>(buffer.size);

    graphgen_->freeMemory(&tempBuffer);

    buffer.addr = nullptr;
    buffer.size = 0;

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

