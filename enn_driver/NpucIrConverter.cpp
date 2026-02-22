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
 * @file    NpucIrConverter.cpp
 * @brief   This is NpucIrConverter class file.
 * @details This header defines NpucIrConverter class.
 *          This class is implementing handshaking with compiler component.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#include <iostream>

#include "log.h"
#include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "Common.h"  // RET_OK
#include "NpucIrConverter.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::NpucIrConverter"

namespace android {
namespace nn {
namespace eden_driver {

/**
 * @brief Compile operations on operationList of a given model and returns generated binary(NCP) on buffer
 * @details This function compiles an operation list of a given model for NPU and generates a result on buffer.
 *          Memory allocation is not caller's responsibility.
 *          This function cooperates with npuc_ir_converter to complete compilation for NPU.
 * @param[in] model Android NN Model
 * @param[in] operationList operation index list corresponding on Model's Operation[] to be compiled for NPU
 * @param[out] network NNNetwork which is an input for npuc
 * @return error code
 */
int32_t NpucIrConverter::convertToNNNetwork(const V1_3::Model& model, ModelInfo& modelInfo, const std::vector<int32_t>& operationList, std::shared_ptr<NPUC::NNNetwork>& network) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // 1) Convert Android NN Model to NNGraph-specific data structure for a given operation list
    NNNetworkBuilder builder;

    auto nn_network = builder.buildNNNetwork(model, modelInfo, operationList);
    if(nn_network == nullptr) {
        LOGE(EDEN_DRIVER, "Fail to generate NN network.\n");
        return FAIL_ON_NPUC_IR_CONVERTER;
    }
    network = nn_network;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

