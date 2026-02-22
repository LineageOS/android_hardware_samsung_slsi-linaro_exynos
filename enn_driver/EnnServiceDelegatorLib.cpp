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
 * @file    EnnServiceDelegatorLib.cpp
 * @brief   This is EnnServiceDelegatorLib class file.
 * @details This header defines EnnServiceDelegatorLib class.
 *          This class is implementing proxy role for Eden Runtime service.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#include <sys/types.h>  // getpid
#include <unistd.h>     // getpid

#include <iostream>

#include "EnnServiceDelegatorLib.h"
#include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER
#include "Common.h"

#include "../eden/include/log.h"
#include "../eden/include/eden_nn_api.h"
#include "../eden/include/eden_nn_types.h"   // NnRet

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::EnnServiceDelegatorLib"

namespace android {
namespace nn {
namespace eden_driver {

/**
 *  @brief Initialize ExynosNN
 *  @details This API function initializes the CPU/GPU/NPU handler.
 *  @param void
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennInitialize(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    NnRet ret = Initialize();
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "Initialize() is failed.\n");
        return FAIL_ON_EDEN_INIT;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Read a in-memory model on address and open it as a EdenModel
 *  @details This function reads a in-memory model on a given address and convert it to EdenModel.
 *           The in-memory model should be one of the supported model type in memory.
 *           Once it successes to parse a given in-memory model,
 *           unique model id is returned via modelId.
 *  @param[in] modelTypeInMemory it is representing for in-memory model such as Android NN Model.
 *  @param[in] addr address of in-memory model
 *  @param[in] size size of in-memory model
 *  @param[in] encrypted data on addr is encrypted
 *  @param[out] modelId It is representing for constructed EdenModel with a unique id.
 *  @param[in] preference It is representing for a model preference.
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennOpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                                                     bool encrypted, uint32_t* modelId, EdenModelOptions& options) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (addr == nullptr || modelId == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "addr : %p, size : %d, modelId : %u\n", addr, size, *modelId);

    NnRet ret = OpenEdenModelFromMemory(modelTypeInMemory, addr, size, encrypted, modelId, options);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "OpenEdenModelFromMemory() is failed.\n");
        return FAIL_ON_EDEN_OPEN_MODEL_FROM_MEMORY;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Execute a model with given buffers in nonblocking mode.
 * @details This function executes a model with input/output buffers.
 *          Internally EDEN NN creates a request to execute a model with buffers,
 *          and this request is numbered with an unique id.
 *          This unique id is returned to caller via requestId.
 *          When the execution is complete, the callback's notify is executed by EDEN NN.
 * @param[in] request It is representing for eden model, input/output buffers and callback.
 * @param[out] requestId Unique id representing for this request.
 * @param[in] preference It is representing for a model preference.
 * @returns return code
 */
int32_t EnnServiceDelegatorLib::ennExecuteModel(EdenRequest* request, addr_t* requestId, EdenPreference preference) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (request == nullptr || requestId == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    NnRet ret = ExecuteModel(request, requestId, preference);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "ExecuteModel() is failed.\n");
        return FAIL_ON_EDEN_EXECUTE_REQ;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Close EDEN Model
 *  @details This API function releases resources related with the EDEN Model.
 *  @param[in] modelId It is a unique id for EDEN Model.
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennCloseModel(uint32_t modelId) {
    LOGD(EDEN_DRIVER, "%s() is called with modelId : %u\n", __func__, modelId);

    NnRet ret = CloseModel(modelId);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "CloseModel() is failed.\n");
        return FAIL_ON_EDEN_CLOSE_MODEL;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Shutdown EDEN Runtime
 *  @details This API function close all EDEN Models with related resources for shutdown EDEN Framework.
 *  @param void
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennShutdown(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    NnRet ret = Shutdown();
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "Shutdown() is failed.\n");
        return FAIL_ON_EDEN_SHUTDOWN;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Allocate a buffer for input to execute a model
 *  @details This function allocates an efficient buffer to execute a model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[out] buffers Array of EdenBuffers for input
 *  @param[out] numOfBuffers # of buffers
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennAllocateInputBuffers(uint32_t modelId, EdenBuffer** buffers,
                                                        int32_t* numOfBuffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr || numOfBuffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "Try to allocate buffers for inputs, modelId : %u\n", modelId);

    NnRet ret = AllocateInputBuffers(modelId, buffers, numOfBuffers);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "AllocateInputBuffers() is failed.\n");
        return FAIL_ON_EDEN_ALLOCATE_INPUT_BUFFERS;
    }

    LOGD(EDEN_DRIVER, "Complete to allocate buffers for inputs, modelId : %u, numofBuffers : %d\n",
                                                                            modelId, *numOfBuffers);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Allocate a buffer for output to execute a model
 *  @details This function allocates an efficient buffer to execute a model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[out] buffers Array of EdenBuffers for input
 *  @param[out] numOfBuffers # of buffers
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennAllocateOutputBuffers(uint32_t modelId, EdenBuffer** buffers,
                                                         int32_t* numOfBuffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr || numOfBuffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "Try to allocate buffers for outputs, modelId : %u\n", modelId);

    NnRet ret = AllocateOutputBuffers(modelId, buffers, numOfBuffers);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "AllocateOutputBuffers() is failed.\n");
        return FAIL_ON_EDEN_ALLOCATE_OUTPUT_BUFFERS;
    }

    LOGD(EDEN_DRIVER, "Complete to allocate buffers for outputs, modelId : %u, numofBuffers : %d\n",
                                                                            modelId, *numOfBuffers);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Release a buffer allocated by Eden framework
 *  @details This function releases a buffer returned by AllocateXXXBuffers.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] buffers Buffer pointer allocated by AllocateXXXBuffers
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennFreeBuffers(uint32_t modelId, EdenBuffer* buffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    NnRet ret = FreeBuffers(modelId, buffers);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "FreeBuffers() is failed.\n");
        return FAIL_ON_EDEN_FREE_BUFFERS;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Get the input buffer information
 *  @details This function gets buffer shape for input buffer of a specified model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] inputIndex Input index starting 0.
 *  @param[out] width Width
 *  @param[out] height Height
 *  @param[out] channel Channel
 *  @param[out] number Number
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennGetInputBufferShape(uint32_t modelId, int32_t inputIndex,
                                                       int32_t* width, int32_t* height,
                                                       int32_t* channel, int32_t* number) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (width == nullptr || height == nullptr || channel == nullptr || number == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "modelId : %u, inputIndex : %d, width : %d, height : %d, channel : %d, number : %d\n",
                       modelId, inputIndex, *width, *height, *channel, *number);

    NnRet ret = GetInputBufferShape(modelId, inputIndex, width, height, channel, number);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "GetInputBufferShape() is fail.\n");
        return FAIL_ON_EDEN_GET_INPUT_BUFFER_SHAPE;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Get the output buffer information
 *  @details This function gets buffer shape for output buffers of a specified model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] outputIndex Output index starting 0.
 *  @param[out] width Width
 *  @param[out] height Height
 *  @param[out] channel Channel
 *  @param[out] number Number
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennGetOutputBufferShape(uint32_t modelId, int32_t outputIndex,
                                                        int32_t* width, int32_t* height,
                                                        int32_t* channel, int32_t* number) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (width == nullptr || height == nullptr || channel == nullptr || number == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "modelId : %u, outputIndex : %d, width : %d, height : %d, channel : %d, number : %d\n",
                       modelId, outputIndex, *width, *height, *channel, *number);

    NnRet ret = GetOutputBufferShape(modelId, outputIndex, width, height, channel, number);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "GetOutputBufferShape() is fail.\n");
        return FAIL_ON_EDEN_GET_OUTPUT_BUFFER_SHAPE;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Get Eden version
 *  @details This function gets EdenVersion with a current EDEN framework version.
 *           It includes hardware and software version too.
 *  @param[in] version It is representing for EDEN version information.
 *             This function is returned with version filled with a current information.
 *  @returns return code
 */
int32_t EnnServiceDelegatorLib::ennGetEdenVersion(uint32_t modelId, int32_t* versions) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (versions == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    NnRet ret = GetEdenVersion(modelId, versions);
    if (ret != NnRet::RET_OK) {
        LOGE(EDEN_DRIVER, "GetEdenVersion() fail.\n");
        return ret;
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

