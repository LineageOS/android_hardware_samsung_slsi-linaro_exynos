/*
 * Copyright (C) 2018 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/**
 * @file    eden_nn.cpp
 * @brief   This is EDEN NN class
 * @details This header defines EDEN NN class.
 *          This class is implementing the Eden NN framework.
 * @version 0.1 Basic scenario support.
 */

#include <limits>  // numeric_limits

// nn
#include "eden_nn.h"      // EdenNN
// runtime
#include "EdenRuntime.h"  // eden::rt::XXX, RtRet

#include "log.h"               // LOGD, LOGI, LOGE
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "NN::EdenNn"  // Change defined LOG_TAG on log.h for purpose.

namespace eden {
namespace nn {

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// public functions ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize EDEN NN.
 * @details This function initializes the EDEN NN framework.
 *          Internal data structure and related resource preparation is taken place.
 * @param void
 * @returns return code
 */
NnRet EdenNN::Initialize(void) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::Init();
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_INIT, (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_INIT_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_INIT\n");
                return RET_ERROR_ON_RT_INIT;
            break;
            case RT_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_INIT\n");
                return RET_ERROR_ON_RT_INIT;
            break;
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_INIT\n");
                return RET_ERROR_ON_RT_INIT;
            break;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

NnRet EdenNN::Initialize(uint32_t target) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::Init(target);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_INIT, (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_INIT_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_INIT\n");
                return RET_ERROR_ON_RT_INIT;
            break;
            case RT_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_INIT\n");
                return RET_ERROR_ON_RT_INIT;
            break;
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_INIT\n");
                return RET_ERROR_ON_RT_INIT;
            break;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 * @brief Open a model file and generates an in-memory model structure
 * @details This function reads a model file and construct an in-memory model structure.
 *          The model file should be one of the supported model file format.
 *          Once EDEN NN successes to parse a given model file,
 *          unique model id is returned via modelId.
 * @param[in] modelFile It is representing for EDEN model file such as file path.
 * @param[out] modelId It is representing for constructed EdenModel with a unique id.
 * @param[in] edenPreference It is representing for a model preference.
 * @returns return code
 */
NnRet EdenNN::OpenModel(EdenModelFile* modelFile, uint32_t* modelId, EdenPreference edenPreference) {
    LOGD(EDEN_NN, "Deprecated function\n");
    EdenModelOptions options;
    options.modelPreference.userPreference = edenPreference;
    options.modelPreference.nnApiType = NnApiType::EDEN_NN_API;
    options.priority = ModelPriority::P_DEFAULT;
    options.boundCore = NPU_UNBOUND;
    options.latency = 0;
    options.presetScenarioId = PRESET_DISABLE_UPPER_BOUND;
    options.reserved[0] = {0, };
    return OpenModel(modelFile, modelId, options);
}

NnRet EdenNN::OpenModel(EdenModelFile* modelFile, uint32_t* modelId, EdenModelOptions& options) {
    LOGD(EDEN_NN, "(+)\n");
    options.modelPreference.nnApiType = NnApiType::EDEN_NN_API;

    RtRet rtRet = eden::rt::OpenModelFromFile(modelFile, modelId, options);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_PARAM_INVALID:
                LOGE(EDEN_NN, "(-) RET_PARAM_INVALID\n");
                return RET_PARAM_INVALID;
            break;
            case RT_FAIL_TO_ALLOCATE_MEM:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_MEM_ALLOCATE\n");
                return RET_ERROR_ON_MEM_ALLOCATE;
            break;
            case RT_ERROR_ON_REGISTER_MODEL:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_NOT_REGISTERED\n");
                return RET_MODEL_ID_IS_NOT_REGISTERED;
            break;
            case RT_ERROR_ON_UNREGISTER_MODEL:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_NOT_REGISTERED\n");
                return RET_MODEL_ID_IS_NOT_REGISTERED;
            break;
            case RT_OPEN_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_OPEN_MODEL\n");
                return RET_ERROR_ON_RT_OPEN_MODEL;
            break;
            case RT_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_OPEN_MODEL\n");
                return RET_ERROR_ON_RT_OPEN_MODEL;
            break;
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_OPEN_MODEL\n");
                return RET_ERROR_ON_RT_OPEN_MODEL;
            break;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
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
NnRet EdenNN::OpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size, bool encrypted,
                                  uint32_t* modelId, EdenPreference edenPreference) {
    LOGD(EDEN_NN, "Deprecated function\n");
    EdenModelOptions options;
    options.modelPreference.userPreference = edenPreference;
    options.modelPreference.nnApiType = NnApiType::EDEN_NN_API;
    options.priority = ModelPriority::P_DEFAULT;
    options.boundCore = NPU_UNBOUND;
    options.latency = 0;
    options.reserved[0] = {0, };
    return OpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, modelId, options);
}

NnRet EdenNN::OpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size, bool encrypted,
                                uint32_t* modelId, EdenModelOptions& options) {
    LOGD(EDEN_NN, "(+)\n");

    RtRet rtRet = eden::rt::OpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, modelId, options);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) Error from Runtime, (rtRet=[%d])!\n", rtRet);
        return RET_ERROR_ON_RT_OPEN_MODEL;
    }

    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}
/**
 * @brief Allocate a buffer to execute a model
 * @details This function allocates an efficient buffer to execute a buffer.
 * @param[in] modelId The model id to be applied by.
 * @param[out] buffers Array of EdenBuffers for input
 * @param[out] numOfBuffers # of buffers
 * @returns return code
 */
NnRet EdenNN::AllocateInputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) {
    LOGD(EDEN_NN, "(+)\n");

    RtRet rtRet = eden::rt::AllocateInputBuffers(modelId, buffers, numOfBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_PARAM_INVALID:
                LOGE(EDEN_NN, "(-) RET_PARAM_INVALID\n");
                return RET_PARAM_INVALID;
            break;
            case RT_MODEL_ID_IS_INVALID:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_INVALID\n");
                return RET_MODEL_ID_IS_INVALID;
            break;
            case RT_ERROR_ON_ALLOCATE_BUFFERS_FOR_INPUT_OPERANDS:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_ALLOCATE_INPUT_BUFFERS\n");
                return RET_ERROR_ON_RT_ALLOCATE_INPUT_BUFFERS;
            break;
            case RT_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_ALLOCATE_INPUT_BUFFERS\n");
                return RET_ERROR_ON_RT_ALLOCATE_INPUT_BUFFERS;
            break;
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_ALLOCATE_INPUT_BUFFERS\n");
                return RET_ERROR_ON_RT_ALLOCATE_INPUT_BUFFERS;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 * @brief Allocate a buffer to execute a model
 * @details This function allocates an efficient buffer to execute a buffer.
 * @param[in] modelId The model id to be applied by.
 * @param[out] buffers Array of EdenBuffers for output
 * @param[out] numOfBuffers # of buffers
 * @returns return code
 */
NnRet EdenNN::AllocateOutputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) {
    LOGD(EDEN_NN, "(+)\n");

    RtRet rtRet = eden::rt::AllocateOutputBuffers(modelId, buffers, numOfBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_PARAM_INVALID:
                LOGE(EDEN_NN, "(-) RET_PARAM_INVALID\n");
                return RET_PARAM_INVALID;
            break;
            case RT_MODEL_ID_IS_INVALID:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_INVALID\n");
                return RET_MODEL_ID_IS_INVALID;
            break;
            case RT_ERROR_ON_ALLOCATE_BUFFERS_FOR_OUTPUT_OPERANDS:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_ALLOCATE_OUTPUT_BUFFERS\n");
                return RET_ERROR_ON_RT_ALLOCATE_OUTPUT_BUFFERS;
            break;
            case RT_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_ALLOCATE_OUTPUT_BUFFERS\n");
                return RET_ERROR_ON_RT_ALLOCATE_OUTPUT_BUFFERS;
            break;
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_ALLOCATE_OUTPUT_BUFFERS\n");
                return RET_ERROR_ON_RT_ALLOCATE_OUTPUT_BUFFERS;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 * @brief Load the buffers of user to execute a model
 * @details This function loads to a user buffer for execution
 * @param[in] modelId The model id to be applied by
 * @param[in] Array of Userbuffers for input
 * @param[in] numOfBuffers # of buffers.
 * @param[out] buffers Array of EdenBuffers for input
 * @returns return code
 */
NnRet EdenNN::LoadInputBuffers(uint32_t modelId, UserBuffer* userBuffers, int32_t numOfBuffers, EdenBuffer** edenBuffers) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::LoadInputBuffers(modelId, userBuffers, numOfBuffers, edenBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_LOAD_INPUT_BUFFERS, (rtRet=[%d])\n", rtRet);
        return RET_ERROR_ON_RT_LOAD_BUFFERS;
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 * @brief Load the buffers of user to execute a model
 * @details This function loads to a user buffer for execution
 * @param[in] modelId The model id to be applied by
 * @param[in] Array of Userbuffers for output
 * @param[in] numOfBuffers # of buffers.
 * @param[out] buffers Array of EdenBuffers for output
 * @returns return code
 */
NnRet EdenNN::LoadOutputBuffers(uint32_t modelId, UserBuffer* userBuffers, int32_t numOfBuffers,
                                EdenBuffer** edenBuffers) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::LoadOutputBuffers(modelId, userBuffers, numOfBuffers, edenBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_LOAD_OUTPUT_BUFFERS, (rtRet=[%d])\n", rtRet);
        return RET_ERROR_ON_RT_LOAD_BUFFERS;
    }
    LOGD(EDEN_NN, "(-)\n");
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
 * @param[in] edenPreference It is representing for a model preference.
 * @returns return code
 */
NnRet EdenNN::ExecuteModel(EdenRequest* request, addr_t* requestId, EdenPreference edenPreference) {
    LOGD(EDEN_NN, "Deprecated function\n");
    EdenRequestOptions options;
    options.userPreference = edenPreference;
    options.requestMode = RequestMode::NONE;
    options.reserved[0] = {0, };
    return ExecuteModel(request, requestId, options);
}

NnRet EdenNN::ExecuteModel(EdenRequest* request, addr_t* requestId, const EdenRequestOptions& options) {
    LOGD(EDEN_NN, "(+)\n");

    // Determine requestId for this request
    // Request id is an address of request.
    /** @todo If user reuses EdenRequest* with different inputs, */
    // it might be a problem!!!
    if (requestId == nullptr || request == nullptr) {
        return RET_PARAM_INVALID;
    }
    *requestId = (addr_t)request;
    EdenEvent* edenEvent;

#ifdef DUMP_EDEN_REQUEST
  // Dump EdenRequest
  {
    LOGD(EDEN_NN, "request->modelId=[%u]\n", request->modelId);
    LOGD(EDEN_NN, "request->inputBuffers=[%p]\n", request->inputBuffers);
    LOGD(EDEN_NN, "  request->inputBuffers->addr=[%p]\n", request->inputBuffers->addr);
    LOGD(EDEN_NN, "  request->inputBuffers->size=[%d]\n", request->inputBuffers->size);
    LOGD(EDEN_NN, "request->outputBuffers=[%p]\n", request->outputBuffers);
    LOGD(EDEN_NN, "  request->outputBuffers->addr=[%p]\n", request->outputBuffers->addr);
    LOGD(EDEN_NN, "  request->outputBuffers->size=[%d]\n", request->outputBuffers->size);
    LOGD(EDEN_NN, "request->callback=[%p]\n", request->callback);
    LOGD(EDEN_NN, "  request->callback->requestId=[%ld]\n", request->callback->requestId);
    // LOGD(EDEN_NN, "  request->callback->executionResult.inference=[%d]\n", request->callback->executionResult.inference);
    LOGD(EDEN_NN, "  request->callback->notify=[%p]\n", request->callback->notify);
    LOGD(EDEN_NN, "  request->callback->waitFor=[%p]\n", request->callback->waitFor);
  }
#endif

    RtRet rtRet = eden::rt::ExecuteReq(request, &edenEvent, options);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_PARAM_INVALID:
                LOGE(EDEN_NN, "(-) RET_PARAM_INVALID\n");
                return RET_PARAM_INVALID;
            break;
            case RT_MODEL_INVALID:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_INVALID\n");
                return RET_MODEL_ID_IS_INVALID;
            break;
            case RT_ERROR_ON_REGISTER_MODEL:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_NOT_REGISTERED\n");
                return RET_MODEL_ID_IS_NOT_REGISTERED;
            break;
            case RT_EXECUTE_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_EXECUTE_MODEL\n");
                return RET_ERROR_ON_RT_EXECUTE_MODEL;
            break;
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_EXECUTE_MODEL\n");
                return RET_ERROR_ON_RT_EXECUTE_MODEL;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 * @brief Release a buffer allocated by Eden framework
 * @details This function releases a buffer returned by AllocateXXXBuffers.
 * @param[in] modelId The model id to be applied by.
 * @param[in] buffers Buffer pointer allocated by AllocateXXXBuffers
 * @returns return code
 */
NnRet EdenNN::FreeBuffers(uint32_t modelId, EdenBuffer* buffers) {
    LOGD(EDEN_NN, "(+)\n");

    RtRet rtRet = eden::rt::FreeBuffers(modelId, buffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_PARAM_INVALID:
                LOGE(EDEN_NN, "(-) RET_PARAM_INVALID\n");
                return RET_PARAM_INVALID;
            break;
            case RT_MODEL_ID_IS_INVALID:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_INVALID\n");
                return RET_MODEL_ID_IS_INVALID;
            break;
            case RT_ERROR_ON_FREE_BUFFERS_FOR_OPERANDS:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_FREE_BUFFERS\n");
                return RET_ERROR_ON_RT_FREE_BUFFERS;
            break;
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_FREE_BUFFERS\n");
                return RET_ERROR_ON_RT_FREE_BUFFERS;
            break;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 * @brief Close a model
 * @details This function releases a model related resources and destroies a model.
 * @param[in] modelId The model id to be applied by.
 * @returns return code
 */
NnRet EdenNN::CloseModel(uint32_t modelId) {
    LOGD(EDEN_NN, "(+)\n");

    RtRet rtRet = eden::rt::CloseModel(modelId);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_PARAM_INVALID:
                LOGE(EDEN_NN, "(-) RET_PARAM_INVALID\n");
                return RET_PARAM_INVALID;
            break;
            case RT_MODEL_ID_IS_INVALID:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_INVALID\n");
                return RET_MODEL_ID_IS_INVALID;
            break;
            case RT_MODEL_INVALID:
                LOGE(EDEN_NN, "(-) RET_MODEL_ID_IS_INVALID\n");
                return RET_MODEL_ID_IS_INVALID;
            break;
            case RT_CLOSE_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_CLOSE_MODEL\n");
                return RET_ERROR_ON_RT_CLOSE_MODEL;
            break;
            case RT_ERROR_ON_UNREGISTER_MODEL:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_CLOSE_MODEL\n");
                return RET_ERROR_ON_RT_CLOSE_MODEL;
            break;
            case RT_ERROR_ON_EDEN_MODEL_CLOSE_MODEL:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_CLOSE_MODEL\n");
                return RET_ERROR_ON_RT_CLOSE_MODEL;
            break;
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_CLOSE_MODEL\n");
                return RET_ERROR_ON_RT_CLOSE_MODEL;
            break;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 *  @brief Shutdown NN framework
 *  @details This function destorys the Eden NN framework.
 *           Eden NN lets Eden Runtime know there is no more NN activity.
 *  @param void
 *  @returns return code
 */
NnRet EdenNN::Shutdown(void) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::Shutdown();
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_SHUTDOWN (rtRet=[%d])!\n", rtRet);
        switch (rtRet) {
            case RT_SERVICE_NOT_AVAILABLE:
                LOGE(EDEN_NN, "(-) RET_SERVICE_NOT_AVAILABLE\n");
                return RET_SERVICE_NOT_AVAILABLE;
            break;
            case RT_FAILED:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_SHUTDOWN\n");
                return RET_ERROR_ON_RT_SHUTDOWN;
            break;
            default:
                LOGE(EDEN_NN, "(-) RET_ERROR_ON_RT_SHUTDOWN\n");
                return RET_ERROR_ON_RT_SHUTDOWN;
            break;
        }
    }
    LOGD(EDEN_NN, "(-)\n");
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
NnRet EdenNN::GetInputBufferShape(uint32_t modelId, int32_t inputIndex, int32_t* width, int32_t* height, int32_t* channel, int32_t* number) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::GetInputBufferShape(modelId, inputIndex, width, height, channel, number);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) Error from Runtime, (rtRet=[%d])!\n", rtRet);
        return RET_ERROR_ON_RT_GET_INPUT_BUFFER_SHAPE;
    }
    LOGD(EDEN_NN, "(-)\n");
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
NnRet EdenNN::GetOutputBufferShape(uint32_t modelId, int32_t outputIndex, int32_t* width, int32_t* height, int32_t* channel, int32_t* number) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::GetOutputBufferShape(modelId, outputIndex, width, height, channel, number);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) Error from Runtime, (rtRet=[%d])!\n", rtRet);
        return RET_ERROR_ON_RT_GET_OUTPUT_BUFFER_SHAPE;
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 *  @brief Get Eden version
 *  @details This function gets EdenVersion with a current EDEN framework version.
 *           It includes hardware and software version too.
 *  @param[out] version It is representing for EDEN version information.
 *         This function is returned with version filled with a current information.
 *  @returns return code
 */
NnRet EdenNN::GetEdenVersion(uint32_t modelId, int32_t* versions) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::GetEdenVersion(modelId, versions);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) Error from Runtime, (rtRet=[%d])!\n", rtRet);
        return RET_ERROR_ON_RT_GET_VERSION;
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 *  @brief Get model compile version
 *  @details This function gets compile version
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] modelFile It is representing for EDEN model file such as file path.
 *  @param[out] version It is representing for compile version information.
 *         This function is returned with version filled with a current information.
 *  @returns return code
 */
NnRet EdenNN::GetCompileVersion(uint32_t modelId, EdenModelFile* modelFile, char versions[][VERSION_LENGTH_MAX]) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::GetCompileVersion(modelId, modelFile, versions);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) Error from Runtime, (rtRet=[%d])!\n", rtRet);
        return RET_ERROR_ON_RT_GET_VERSION;
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

/**
 *  @brief Get model compile version
 *  @details This function gets compile version
 *  @param[in] modelTypeInMemory it is representing for in-memory model such as Android NN Model.
 *  @param[in] addr address of in-memory model
 *  @param[in] size size of in-memory model
 *  @param[in] encrypted data on addr is encrypted
 *  @param[out] version It is representing for compile version information.
 *         This function is returned with version filled with a current information.
 *  @returns return code
 */
NnRet EdenNN::GetCompileVersionFromMemory(ModelTypeInMemory typeInMemory, int8_t* addr, int32_t size, bool encrypted,
                                            char versions[][VERSION_LENGTH_MAX]) {
    LOGD(EDEN_NN, "(+)\n");
    RtRet rtRet = eden::rt::GetCompileVersionFromMemory(typeInMemory, addr, size, encrypted, versions);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_NN, "(-) Error from Runtime, (rtRet=[%d])!\n", rtRet);
        return RET_ERROR_ON_RT_GET_VERSION;
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// protected functions ////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

NnRet EdenNN::GenerateRequestId(uint32_t* requestId) {
  LOGD(EDEN_NN, "(+)\n");

  NnRet nnRet = RET_OK;
  // Assign prepared model ID and store it internally
  *requestId = nextRequestId_;
  setOfRequestId_.insert(nextRequestId_);

  // Determine next model ID
  nextRequestId_ = (nextRequestId_ >= std::numeric_limits<uint32_t>::max()) ? (0) : (nextRequestId_ + 1);

  LOGD(EDEN_NN, "(-)\n");
  return nnRet;
}

#if 0

/**
 *  @brief Get Eden version
 *  @details This function gets EdenVersion with a current EDEN framework version.
 *           It includes hardware and software version too.
 *  @param version It is representing for EDEN version information.
 *         This function is returned with version filled with a current information.
 *  @returns return code
 */
NnRet EdenNN::GetVersion(EdenVersion* version) {
  LOGD(EDEN_NN, "GetVersion() is called\n");

  version->swVersion = 0x00000100;
  version->hwVersion = 0x00000100;

  return RET_OK;
}

/**
 *  @brief Get Eden capability
 *  @details This function gets EdenCapability with a current EDEN framework capability.
 *           It includes supported model file format, operators and devices.
 *  @param capability It is representing for EDEN capability information.
 *         This function is returned with capability filled with a current information.
 *  @returns return code
 */
NnRet EdenNN::GetCapability(EdenCapability* capability) {
  LOGD(EDEN_NN, "GetCapability() is called\n");

  capability->supportedFileFormat = NCP | TFLITE;
  capability->supportedDevice = CPU | GPU | NPU | DSP;
  capability->supportedOperators = EDEN_OP_CUSTOM;

  return RET_OK;
}

/**
 *  @brief Get Eden performance info
 *  @details This function gets EdenPerformanceInfo based on a current EDEN framework.
 *           The returned numbers contains static estimated information.
 *  @param performanceInfo It is representing for Eden performance information.
 *         This function is returned with performance information.
 *  @returns return code
 */
NnRet EdenNN::GetPerformanceInfo(EdenPerformanceInfo* performanceInfo) {
  LOGD(EDEN_NN, "GetPerformanceInfo() is called\n");
  return RET_OK;
}

/**
 *  @brief Parse model file and generates a in-memory model structure
 *  @details This function reads a model file and construct an in-memory model structure.
 *           The model file should be one of the supported model file format.
 *           Once EDEN NN successes to parse a given model file,
 *           unique model id is returned via modelId.
 *  @param modelFile It is representing for EDEN model file such as file path.
 *  @param modelId It is representing for constructed EdenModel with a unique id.
 *  @returns return code
 */
NnRet EdenNN::ParseModel(EdenModelFile* modelFile, uint32_t* modelId) {
  LOGD(EDEN_NN, "ParseModel() is called\n");
  return RET_OK;
}

/**
 *  @brief Construct common Model data structure based on a in-memory model structure
 *  @details This function reads a model file and construct an in-memory model structure.
 *           The model file should be one of the supported model file format.
 *           Once EDEN NN successes to parse a given model file,
 *           unique model id is returned via modelId.
 *  @param modelFile It is representing for EDEN model file such as file path.
 *  @param modelId It is representing for constructed EdenModel with a unique id.
 *  @returns return code
 */
NnRet EdenNN::ConstructModel(EdenModelFile* modelFile, uint32_t* modelId) {
  LOGD(EDEN_NN, "ConstructModel() is called\n");
  return RET_OK;
}

/**
 *  @brief Set input buffer data layout
 *  @details This function sets the input buffer data layout such as NCHW, NHWC or CUSTOM.
 *           Default value is NCHW.
 *  @param modelId The model id to be applied by.
 *  @param index Input buffer index.
 *  @param layout Buffer layout (NCHW, NHWC, CUSTOM)
 *  @returns return code
 */
NnRet EdenNN::SetInputBufferLayout(uint32_t modelId, int32_t index, EdenBufferLayout layout) {
  LOGD(EDEN_NN, "SetInputBufferLayout() is called\n");
  return RET_OK;
}

/**
 *  @brief Set Eden preference
 *  @details This function sets EdenPreference of a specified EdenModel.
 *  @param modelId The model id to be applied by.
 *  @param edenPreference The preference value
 *  @returns return code
 */
NnRet EdenNN::SetPreference(uint32_t modelId, EdenPreference edenPreference) {
  LOGD(EDEN_NN, "SetPreference() is called\n");
  return RET_OK;
}

/**
 *  @brief Set performance dump flag
 *  @details This function sets flags which performance data is dumped on execution.
 *           It can be enabled with multiple type.
 *  @param modelId The model id to be applied by.
 *  @param flag The preference flag which bit is representing for dump type.
 *  @returns return code
 */
NnRet EdenNN::SetPerformanceDump(uint32_t modelId, uint32_t flag) {
  LOGD(EDEN_NN, "SetPerformanceDump() is called\n");
  return RET_OK;
}

/**
 *  @brief Set rate control information
 *  @details This function sets rate control information on a specified model.
 *  @param modelId The model id to be applied by.
 *  @param rateControlInfo
 *  @returns return code
 */
NnRet EdenNN::SetRateControl(uint32_t modelId, EdenRateControlInfo* rateControlInfo) {
  LOGD(EDEN_NN, "SetRateControl() is called\n");
  return RET_OK;
}

/**
 *  @brief Set training information
 *  @details This function sets training information on a specified model.
 *  @param modelId The model id to be applied by.
 *  @param trainingInfo The training information which represents for configurable parameters on training.
 *  @returns return code
 */
NnRet EdenNN::SetTrainingInfo(uint32_t modelId, EdenTrainingInfo* trainingInfo) {
  LOGD(EDEN_NN, "SetTrainingInfo() is called\n");
  return RET_OK;
}

/**
 *  @brief Get the input buffer structure
 *  @details This function gets EdenBufferSetInfo for input buffers of a specified model.
 *           Returned bufferSetInfo contains input data structure and its name on model.
 *  @param modelId The model id to be applied by.
 *  @param bufferSetInfo The data structure for representing buffer set
 *  @returns return code
 */
NnRet EdenNN::GetInputBufferSetInfo(uint32_t modelId, EdenBufferSetInfo* bufferSetInfo) {
  LOGD(EDEN_NN, "GetInputBufferSetInfo() is called\n");

  /** @todo Below information should be filled with real input data */
  //              by calling a model->GetInputBufferSetInfo()
  // Dummy
  int32_t numberOfBuffers = 1;
  EdenBufferNameMap* bufferNameMap =
            static_cast<EdenBufferNameMap*>(malloc(sizeof(EdenBufferNameMap) * numberOfBuffers));

  EdenString* bufferName = static_cast<EdenString*>(malloc(sizeof(EdenString)));
  bufferName->name = static_cast<int8_t*>("input01");
  bufferName->length = 8;

  EdenBuffer* buffer = static_cast<EdenBuffer*>(malloc(sizeof(EdenBuffer)));
  int32_t sizeOfBuffer = 1024;
  buffer->addr = static_cast<void*>(malloc(sizeOfBuffer));
  buffer->size = sizeOfBuffer;

  EdenBufferInfo* bufferInfo = static_cast<EdenBufferInfo*>(malloc(sizeof(EdenBufferInfo)));
  bufferInfo->dataType = DATA_TYPE_INT8;
  bufferInfo->bufferLayout = BUFFER_LAYOUT_NCHW;
  bufferInfo->numOfContentsInBuffer = 1;
  bufferInfo->channel = 3;
  bufferInfo->height = 224;
  bufferInfo->width = 224;

  bufferNameMap->bufferName = bufferName;
  bufferNameMap->buffer = buffer;
  bufferNameMap->bufferInfo = bufferInfo;

  bufferSetInfo->numOfBuffers = numberOfBuffers;
  bufferSetInfo->bufferMaps = bufferNameMap;

  return RET_OK;
}

/**
 *  @brief Get the output buffer structure
 *  @details This function gets EdenBufferSetInfo for output buffers of a specified model.
 *           Returned bufferSetInfo contains output data structure and its name on model.
 *  @param modelId The model id to be applied by.
 *  @param bufferSetInfo The data structure representing for buffer set
 *  @returns return code
 */
NnRet EdenNN::GetOutputBufferSetInfo(uint32_t modelId, EdenBufferSetInfo* bufferSetInfo) {
  LOGD(EDEN_NN, "GetOutputBufferSetInfo() is called\n");

  /** @todo Below information should be filled with real input data */
  //              by calling a model->GetInputBufferSetInfo()
  // Dummy
  int32_t numberOfBuffers = 1;
  EdenBufferNameMap* bufferNameMap =
            static_cast<EdenBufferNameMap*>(malloc(sizeof(EdenBufferNameMap) * numberOfBuffers));

  EdenString* bufferName = static_cast<EdenString*>(malloc(sizeof(EdenString)));
  bufferName->name = static_cast<int8_t*>("input01");
  bufferName->length = 8;

  EdenBuffer* buffer = static_cast<EdenBuffer*>(malloc(sizeof(EdenBuffer)));
  int32_t sizeOfBuffer = 1024;
  buffer->addr = static_cast<void*>(malloc(sizeOfBuffer));
  buffer->size = sizeOfBuffer;

  EdenBufferInfo* bufferInfo = static_cast<EdenBufferInfo*>(malloc(sizeof(EdenBufferInfo)));
  bufferInfo->dataType = DATA_TYPE_INT8;
  bufferInfo->bufferLayout = BUFFER_LAYOUT_NCHW;
  bufferInfo->numOfContentsInBuffer = 1;
  bufferInfo->channel = 1;
  bufferInfo->height = 1;
  bufferInfo->width = 224;

  bufferNameMap->bufferName = bufferName;
  bufferNameMap->buffer = buffer;
  bufferNameMap->bufferInfo = bufferInfo;

  bufferSetInfo->numOfBuffers = numberOfBuffers;
  bufferSetInfo->bufferMaps = bufferNameMap;

  return RET_OK;
}

/**
 *  @brief Set buffer trait information
 *  @details This function sets a buffer traits such as image format, type etc to a specified buffer
 *  @param modelId The model id to be applied by.
 *  @param bufferTraitInfo The data structure representing for buffer trait.
 *  @returns return code
 */
NnRet EdenNN::SetBufferTraitInfo(uint32_t modelId, int32_t index, EdenBufferTraitInfo* bufferTraitInfo) {
  LOGD(EDEN_NN, "SetBufferTraitInfo() is called\n");
  return RET_OK;
}

/**
 *  @brief Set buffer quantization
 *  @details This function sets a buffer quantizaion information such as Qm.n or scale.
 *  @param modelId The model id to be applied by.
 *  @param bufferQuantizeInfo The data structure representing for buffer quantization.
 *  @returns return code
 */
NnRet EdenNN::SetBufferQuantizeInfo(uint32_t modelId, int32_t index, EdenBufferQuantizeInfo* bufferQuantizeInfo) {
  LOGD(EDEN_NN, "SetBufferQuantizeInfo() is called\n");
  return RET_OK;
}

/**
 *  @brief Allocate a buffer to execute a model
 *  @details This function allocates an efficient buffer to execute a buffer.
 *  @param modelId The model id to be applied by.
 *  @param bufferSize Size of buffer.
 *  @param addr Buffer address allocated.
 *  @returns return code
 */
NnRet EdenNN::AllocateBuffer(uint32_t modelId, int32_t bufferSize, void** addr) {
  LOGD(EDEN_NN, "AllocateBuffer() is called\n");
  return RET_OK;
}

/**
 *  @brief Release a buffer
 *  @details This function releases a buffer returned by AllocateBuffer.
 *  @param modelId The model id to be applied by.
 *  @param addr Buffer address allocated
 *  @returns return code
 */
NnRet EdenNN::FreeBuffer(uint32_t modelId, void* addr) {
  LOGD(EDEN_NN, "FreeBuffer() is called\n");
  return RET_OK;
}

/**
 *  @brief Get performance result
 *  @details This function gets a performance result on runtime.
 *           The data enabled by SetPerformanceDump is dumped.
 *  @param modelId The model id to be applied by.
 *  @param requestId Unique id representing for a request to dump performance.
 *  @param performanceResult Data structure representing for performance result captured on runtime.
 *  @returns return code
 */
NnRet EdenNN::GetPerformanceResult(uint32_t modelId, uint32_t requestId, EdenPerformanceResult* performanceResult) {
  LOGD(EDEN_NN, "GetPerformanceResult() is called\n");
  return RET_OK;
}

/**
 *  @brief Get training result
 *  @details This function gets a training result on runtime.
 *  @param modelId The model id to be applied by.
 *  @param requestId Unique id representing for a request to get training result.
 *  @param trainingResult Data structure representing for training result.
 *  @returns return code
 */
NnRet EdenNN::GetTrainingResult(uint32_t modelId, uint32_t requestId, EdenTrainingResult* trainingResult) {
  LOGD(EDEN_NN, "GetTrainingResult() is called\n");
  return RET_OK;
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// private functions //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
}  // namespace nn
}  // namespace eden

