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
 * @file    EnnServiceDelegatorLib.cpp
 * @brief   This is EnnServiceDelegatorLib class file.
 * @details This header defines EnnServiceDelegatorLib class.
 *          This class is implementing proxy role for Enn Runtime service.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#include <sys/types.h>  // getpid
#include <unistd.h>     // getpid

#include <iostream>

#include "UEnnServiceDelegatorLib.h"
#include "Common.h"

#include "log.h"
#include "../enn/include/enn_api-public.hpp"
#include "../enn/include/enn_api-type.h"   // EnnReturn

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EnnDriver::UEnnServiceDelegatorLib"

namespace android {
namespace nn {
namespace enn_driver {

/**
 *  @brief Initialize ExynosNN
 *  @details This API function initializes the CPU/GPU/NPU handler.
 *  @param void
 *  @returns return code
 */
int32_t UEnnServiceDelegatorLib::uennInitialize(void) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    EnnReturn ret = enn::api::EnnInitialize();
    if (ret != EnnReturn::ENN_RET_SUCCESS) {
        LOGE(ENN_DRIVER, "Initialize() is failed.\n");
        return FAIL_ON_ENN_INIT;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Read a model from a file and open it as a EnnModel
 *  @details This function reads a NNC model from a given file location and convert it to EnnModel.
 *           Once it successes to parse a given  model,
 *           unique model id is returned via modelId.
 *  @param[in] model loacation of a NNC model file in device
 *  @param[out] modelId It is representing for constructed EnnModel with a unique id.
 *  @param[in] preference It is representing for a model preference.
 *  @returns return code
 */
int32_t UEnnServiceDelegatorLib::uennOpenModel(const char* va, const uint32_t size, EnnModelId *model_id) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    if ( model_id == nullptr) {
        LOGE(ENN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    EnnReturn ret = enn::api::EnnOpenModelFromMemory(va, size, model_id);
    if (ret != EnnReturn::ENN_RET_SUCCESS) {
        LOGE(ENN_DRIVER, "EnnOpenModel() is failed.\n");
        return FAIL_ON_ENN_OPEN_MODEL_FROM_MEMORY;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Execute a model with given buffers in blocking mode.
 * @details This function executes a model with input/output buffers.
 * @param[in] EnnModelId model to be executed
 * @returns return code
 */
int32_t UEnnServiceDelegatorLib::uennExecuteModel(const EnnModelId model_id) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    EnnReturn ret = enn::api::EnnExecuteModel(model_id);
    if (ret != EnnReturn::ENN_RET_SUCCESS) {
        LOGE(ENN_DRIVER, "ExecuteModel() is failed.\n");
        return FAIL_ON_ENN_EXECUTE_REQ;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Close ENN Model
 *  @details This API function releases resources related with the ENN Model.
 *  @param[in] modelId It is a unique id for ENN Model.
 *  @returns return code
 */
int32_t UEnnServiceDelegatorLib::uennCloseModel(const EnnModelId modelId) {
    LOGD(ENN_DRIVER, "%s() is called with modelId : %u\n", __func__, modelId);

    EnnReturn ret = enn::api::EnnCloseModel(modelId);
    if (ret != EnnReturn::ENN_RET_SUCCESS) {
        LOGE(ENN_DRIVER, "CloseModel() is failed.\n");
        return FAIL_ON_ENN_CLOSE_MODEL;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Deinitialize ENN Runtime
 *  @details This API function close all ENN Models with related resources for shutdown ENN Framework.
 *  @param void
 *  @returns return code
 */
int32_t UEnnServiceDelegatorLib::uennDeinitialize(void) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    EnnReturn ret = enn::api::EnnDeinitialize();
    if (ret != EnnReturn::ENN_RET_SUCCESS) {
        LOGE(ENN_DRIVER, "Deinitialize() is failed.\n");
        return FAIL_ON_ENN_SHUTDOWN;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Allocate a buffer for input and output to execute a model
 *  @details This function allocates an efficient buffer to execute a model.
 *  @param[in] buffer_set set of buffers required by a model
 *  @param[in] model_id The model id to be applied by.
 *  @param[out] numOfInputBuffers total number of input buffers
 *  @param[out] numOfOutputBuffers toal number of Output Buffers
 *  @returns return code
 */

int32_t UEnnServiceDelegatorLib::uennAllocateAllBuffers(const EnnModelId model_id,
                                           EnnBufferPtr **buffer_set,
                                           uint32_t* numOfInputBuffers,
                                           uint32_t* numOfOutputBuffers) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    if ( numOfInputBuffers == nullptr || numOfOutputBuffers == nullptr) {
        LOGE(ENN_DRIVER, "Invalid Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(ENN_DRIVER, "Try to allocate buffers, modelId : %u\n", model_id);
    enn::api::EnnGenerateBufferSpace(model_id);

    NumberOfBuffersInfo buffersInfo;
    EnnReturn ret = enn::api::EnnAllocateAllBuffers(model_id, buffer_set, &buffersInfo, 0, false);
    if (ret != EnnReturn::ENN_RET_SUCCESS) {
        LOGE(ENN_DRIVER, "AllocateAllBuffers() is failed.\n");
        return FAIL_ON_ENN_ALLOCATE_BUFFERS;
    }

    *numOfInputBuffers = buffersInfo.n_in_buf;
    *numOfOutputBuffers = buffersInfo.n_out_buf;

    ret = enn::api::EnnBufferCommit(model_id);
    if (ret != EnnReturn::ENN_RET_SUCCESS) {
           LOGE(ENN_DRIVER, "EnnBufferCommit() is failed.\n");
           return FAIL_ON_ENN_EXECUTE_REQ;
    }

    LOGD(ENN_DRIVER, "Complete to allocate buffers for inputs, outputs, modelId : %u, numofInBuffers : %u numOfOutBuffers =%u\n",
                                                                            model_id, *numOfInputBuffers, *numOfOutputBuffers);

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}


//@ToDo: nihar.desai Make use of This API!!!
/**
 *  @brief Release a buffer allocated by ENN framework
 *  @details This function releases a buffer returned by AllocateXXXBuffers.
 *  @param[in] buffers Buffer pointer allocated by AllocateXXXBuffers
 *  @returns return code
 */
int32_t UEnnServiceDelegatorLib::uennFreeBuffers(EnnBufferPtr *buffers, const EnnModelId& model_id) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr) {
        LOGE(ENN_DRIVER, "Invalid Params.\n");
        return INVALID_PARAMS;
    }
    NumberOfBuffersInfo buffersInfo;
    EnnReturn ret = enn::api::EnnGetBuffersInfo(&buffersInfo, model_id);

    const int32_t numOfBuffers = buffersInfo.n_in_buf + buffersInfo.n_out_buf;
    ret = enn::api::EnnReleaseBuffers(buffers, numOfBuffers );
    if (ret != EnnReturn::ENN_RET_SUCCESS) {
        LOGE(ENN_DRIVER, "ReleaseBuffers() is failed.\n");
        return FAIL_ON_ENN_FREE_BUFFERS;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

}  // namespace enn_driver
}  // namespace nn
}  // namespace android
