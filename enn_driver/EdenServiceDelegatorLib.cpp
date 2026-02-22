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
 * @file    EdenServiceDelegatorLib.cpp
 * @brief   This is EdenServiceDelegatorLib class file.
 * @details This header defines EdenServiceDelegatorLib class.
 *          This class is implementing proxy role for Eden Runtime service.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#include <iostream>

#include <sys/types.h>  // getpid
#include <unistd.h>     // getpid
#include "log.h"
#include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "Common.h"
#include "EdenServiceDelegatorLib.h"

#include "../include/EdenRuntime.h"
#include "../include/EdenRuntimeType.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::EdenServiceDelegatorLib"

namespace android {
namespace nn {
namespace eden_driver {

/**
 *  @brief Init EDEN Runtime
 *  @details This API function initializes the CPU/GPU/NPU handler.
 *  @param void
 *  @returns return code
 */
int32_t EdenServiceDelegatorLib::Init(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    RtRet ret = eden::rt::Init();
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt:init() is failed.\n");
        return FAIL_ON_EDEN_INIT;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Open EDEN Model
 *  @details This API function set a unique ID to EdenModel, preference, and compute the running path.
 *  @param[in] model it contains overall information.
 *  @param[out] modelId unique id for EDEN Model.
 *  @param[in] preference it determines how to run model with preference.
 *  @returns return code
 */
int32_t EdenServiceDelegatorLib::OpenModel(EdenModel* model, uint32_t* modelId, ModelPreference preference) {
    LOGD(EDEN_DRIVER, "%s() is called with modelId:%u\n", __func__, *modelId);

    if (model == nullptr || modelId == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    // not used this function
    RtRet ret = eden::rt::OpenModel(model, modelId, preference);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt:OpenModel() is failed.\n");
        return FAIL_ON_EDEN_OPEN_MODEL;
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
int32_t EdenServiceDelegatorLib::OpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                                                     bool encrypted, uint32_t* modelId, ModelPreference preference) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (addr == nullptr || modelId == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "addr : %p, size : %d, modelId : %u\n", addr, size, *modelId);

    RtRet ret = eden::rt::OpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, modelId, preference);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt:init() is failed.\n");
        return FAIL_ON_EDEN_OPEN_MODEL_FROM_MEMORY;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Execute EDEN Req
 *  @details This API function executes EdenRequest with preference.
 *  @param[in] req It consists of EDEN Model ID, input/output buffers and callback.
 *  @param[in] evt Callback function defined by User.
 *  @param[in] preference it determines how to run EdenModel with preference.
 *  @returns return code
 */
int32_t EdenServiceDelegatorLib::ExecuteReq(EdenRequest* req, EdenEvent** evt, RequestPreference preference) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (req == nullptr || evt == nullptr || *evt == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    RtRet ret = eden::rt::ExecuteReq(req, evt, preference);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::ExecuteReq() is failed.\n");
        return FAIL_ON_EDEN_EXECUTE_REQ;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Execute EDEN Request with RequestOptions
 *  @details This API function executes EdenRequest with requestOptions.
 *  @param[in] request It consists of EDEN Model ID, input/output buffers and callback.
 *  @param[in] requestOptions
 *  @returns return code
 */
int32_t EdenServiceDelegatorLib::ExecuteRequest(EdenRequest* request, RequestOptions requestOptions) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (request == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    RtRet ret = eden::rt::ExecuteRequest(request, requestOptions);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::ExecuteRequest() is failed.\n");
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
int32_t EdenServiceDelegatorLib::CloseModel(uint32_t modelId) {
    LOGD(EDEN_DRIVER, "%s() is called with modelId : %u\n", __func__, modelId);

    RtRet ret = eden::rt::CloseModel(modelId);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt:CloseModel() is failed.\n");
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
int32_t EdenServiceDelegatorLib::Shutdown(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    RtRet ret = eden::rt::Shutdown();
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt:CloseModel() is failed.\n");
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
int32_t EdenServiceDelegatorLib::AllocateInputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr || numOfBuffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "Try to allocate buffers for inputs, modelId : %u\n", modelId);

    RtRet ret = eden::rt::AllocateInputBuffers(modelId, buffers, numOfBuffers);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::AllocateInputBuffers() is failed.\n");
        return FAIL_ON_EDEN_ALLOCATE_INPUT_BUFFERS;
    }

    LOGD(EDEN_DRIVER, "Complete to allocate buffers for inputs, modelId : %u, numofBuffers : %d\n", modelId, *numOfBuffers);

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
int32_t EdenServiceDelegatorLib::AllocateOutputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr || numOfBuffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "Try to allocate buffers for outputs, modelId : %u\n", modelId);

    RtRet ret = eden::rt::AllocateOutputBuffers(modelId, buffers, numOfBuffers);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::AllocateOutputBuffers() is failed.\n");
        return FAIL_ON_EDEN_ALLOCATE_OUTPUT_BUFFERS;
    }

    LOGD(EDEN_DRIVER, "Complete to allocate buffers for outputs, modelId : %u, numofBuffers : %d\n", modelId, *numOfBuffers);

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
int32_t EdenServiceDelegatorLib::FreeBuffers(uint32_t modelId, EdenBuffer* buffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    RtRet ret = eden::rt::FreeBuffers(modelId, buffers);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::AllocateOutputBuffers() is failed.\n");
        return FAIL_ON_EDEN_FREE_BUFFERS;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Get Processing Units state
 *  @details This API function returns state of CPU/GPU/NPU.
 *  @param[out] state
 *  @returns return code
 */
int32_t EdenServiceDelegatorLib::GetState(EdenState* state) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (state == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    RtRet ret = eden::rt::GetState(state);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::GetState() is failed.\n");
        return ret;
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
int32_t EdenServiceDelegatorLib::GetInputBufferShape(uint32_t modelId, int32_t inputIndex, int32_t* width, int32_t* height,
        int32_t* channel, int32_t* number) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (width == nullptr || height == nullptr || channel == nullptr || number == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "modelId : %u, inputIndex : %d, width : %d, height : %d, channel : %d, number : %d\n",
                       modelId, inputIndex, *width, *height, *channel, *number);

    RtRet ret = eden::rt::GetInputBufferShape(modelId, inputIndex, width, height, channel, number);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::GetInputBufferShape() is called.\n");
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
int32_t EdenServiceDelegatorLib::GetOutputBufferShape(uint32_t modelId, int32_t outputIndex, int32_t* width, int32_t* height,
        int32_t* channel, int32_t* number) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (width == nullptr || height == nullptr || channel == nullptr || number == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "modelId : %u, outputIndex : %d, width : %d, height : %d, channel : %d, number : %d\n",
                       modelId, outputIndex, *width, *height, *channel, *number);

    RtRet ret = eden::rt::GetOutputBufferShape(modelId, outputIndex, width, height, channel, number);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::GetOutputBufferShape() is called.\n");
        return FAIL_ON_EDEN_GET_OUTPUT_BUFFER_SHAPE;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Get Ncp Buffer from EdenManager
 *  @details This function gets NCP buffer to release NCP buffer by compiler.
 *  @param[in] modelId
 *  @param[out] NCP buffer
 *  @returns return code
 */
int32_t EdenServiceDelegatorLib::GetNcpBuffer(uint32_t modelId, void*& ncpBuffer) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    RtRet ret = eden::rt::GetNcpBuffer(modelId, ncpBuffer);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::GetNcpBuffer() fail(%d).\n", ret);
        return ret;
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

