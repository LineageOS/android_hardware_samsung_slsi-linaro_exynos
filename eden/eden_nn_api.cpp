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
 * @file    eden_nn_api.cpp
 * @brief   This is EDEN NN API implementation.
 * @details This is the implementation of EDEN NN API.
 * @version 0.2 Basic scenario support & modified
 *          Supported functions are as below.
 *          NnRet Initialize(void)
 *          NnRet OpenModel(EdenModelFile* modelFile, uint32_t* modelId, EdenPreference preference)
 *          NnRet AllocateInputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers)
 *          NnRet AllocateOutputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers)
 *          NnRet ExecuteModel(EdenRequest request, uint32_t* requestId, EdenPreference preference)
 *          NnRet FreeBuffers(uint32_t modelId, EdenBuffer* buffers)
 *          NnRet CloseModel(uint32_t modelId)
 *          NnRet Shutdown(void)
 */

// nn
#include "include/eden_nn_api.h"  // NN API
#include "eden_nn.h"              // EdenNN

#include "log.h"                   // LOGD, LOGI, LOGE
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "NN::EdenNnApi"  // Change defined LOG_TAG on log.h for purpose.

using namespace eden::nn;

EdenNN* edenNN = nullptr;

/**
 *  @brief Initialize EDEN NN.
 *  @details This function initializes the EDEN NN framework.
 *           Internal data structure and related resource preparation is taken place.
 *  @param void
 *  @returns return code
 */
NnRet Initialize(void) {
  if (edenNN == nullptr) {
    edenNN = new EdenNN();
  }
  return edenNN->Initialize();
}

NnRet InitializeTarget(uint32_t target) {
  if (edenNN == nullptr) {
    edenNN = new EdenNN();
  }
  return edenNN->Initialize(target);
}
/**
 * @brief Load a model file and construct in-memory model structure
 * @details This function reads a model file and construct an in-memory model structure.
 *          The model file should be one of the supported model file format.
 *          Once EDEN NN successes to parse a given model file,
 *          unique model id is returned via modelId.
 * @param[in] modelFile It is representing for EDEN model file such as file path.
 * @param[out] modelId It is representing for constructed EdenModel with a unique id.
 * @param[in] preference It is representing for a model preference.
 * @returns return code
 */
NnRet OpenModel(EdenModelFile* modelFile, uint32_t* modelId, EdenPreference preference) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->OpenModel(modelFile, modelId, preference);
}

NnRet OpenEdenModel(EdenModelFile* modelFile, uint32_t* modelId, EdenModelOptions& options) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->OpenModel(modelFile, modelId, options);
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
NnRet OpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size, bool encrypted,
                          uint32_t* modelId, EdenPreference preference) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->OpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, modelId, preference);
}

NnRet OpenEdenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size, bool encrypted,
                        uint32_t* modelId, EdenModelOptions& options) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->OpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, modelId, options);
}

/**
 * @brief Allocate a buffer to execute a model
 * @details This function allocates an efficient buffer to execute a buffer.
 * @param[in] modelId The model id to be applied by.
 * @param[out] buffers Array of EdenBuffers for input
 * @param[out] numOfBuffers # of buffers
 * @returns return code
 */
NnRet AllocateInputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->AllocateInputBuffers(modelId, buffers, numOfBuffers);
}

/**
 * @brief Allocate a buffer to execute a model
 * @details This function allocates an efficient buffer to execute a buffer.
 * @param[in] modelId The model id to be applied by.
 * @param[out] buffers Array of EdenBuffers for output
 * @param[out] numOfBuffers # of buffers
 * @returns return code
 */
NnRet AllocateOutputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->AllocateOutputBuffers(modelId, buffers, numOfBuffers);
}

/**
 * @brief Load a buffer of external to execute a model
 * @details This function loads to execute a buffer.
 * @param[in] modelId The model id to be applied by.
 * @param[in] Array of Userbuffers for input
 * @param[in] numOfBuffers # of buffers.
 * @param[out] buffers Array of EdenBuffers for input
 * @returns return code
 */
NnRet LoadInputBuffers(uint32_t modelId, UserBuffer* userBuffers, int32_t numOfBuffers, EdenBuffer** edenBuffers) {
    NnRet ret;

    if ((modelId == INVALID_MODEL_ID) || (userBuffers == nullptr) ||
            (numOfBuffers == 0) || edenBuffers == nullptr) {
        LOGE(EDEN_NN, "Given modelId=[%d], userBuffers=[%p], numOfBuffers=[%d] is invalid! please, check argument\n", modelId, userBuffers, numOfBuffers);
        return RET_PARAM_INVALID;
    }
    ret = edenNN->LoadInputBuffers(modelId, userBuffers, numOfBuffers, edenBuffers);
    if (ret != RET_OK) {
        LOGE(EDEN_NN, "LoadInputBuffers error! modelId=[%d], userBuffers=[%p], numOfBuffers=[%d]", modelId, userBuffers, numOfBuffers);
    }
    return ret;
}

/**
 * @brief Load a buffer of external to execute a model
 * @details This function loads to execute a buffer.
 * @param[in] modelId The model id to be applied by.
 * @param[in] Array of Userbuffers for output
 * @param[in] numOfBuffers # of buffers.
 * @param[out] buffers Array of EdenBuffers for output
 * @returns return code
 */
NnRet LoadOutputBuffers(uint32_t modelId, UserBuffer* userBuffers, int32_t numOfBuffers, EdenBuffer** edenBuffers) {
    NnRet ret;

    if ((modelId == INVALID_MODEL_ID) || (userBuffers == nullptr) ||
            (numOfBuffers == 0) || edenBuffers == nullptr) {
        LOGE(EDEN_NN, "Given modelId=[%d], userBuffers=[%p], numOfBuffers=[%d] is invalid! please,"
             "check argument\n", modelId, userBuffers, numOfBuffers);
        return RET_PARAM_INVALID;
    }
    ret = edenNN->LoadOutputBuffers(modelId, userBuffers, numOfBuffers, edenBuffers);
    if (ret != RET_OK) {
        LOGE(EDEN_NN, "LoadOutputBuffers error! modelId=[%d], userBuffers=[%p], numOfBuffers=[%d]",
             modelId, userBuffers, numOfBuffers);
    }
    return ret;
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
NnRet ExecuteModel(EdenRequest* request, addr_t* requestId, EdenPreference preference) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->ExecuteModel(request, requestId, preference);
}

NnRet ExecuteEdenModel(EdenRequest* request, addr_t* requestId, const EdenRequestOptions& options) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->ExecuteModel(request, requestId, options);
}

/**
 * @brief Release a buffer allocated by Eden framework
 * @details This function releases a buffer returned by AllocateXXXBuffers.
 * @param[in] modelId The model id to be applied by.
 * @param[in] buffers Buffer pointer allocated by AllocateXXXBuffers
 * @returns return code
 */
NnRet FreeBuffers(uint32_t modelId, EdenBuffer* buffers) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->FreeBuffers(modelId, buffers);
}

/**
 * @brief Close a model
 * @details This function releases a model related resources and destroies a model.
 * @param[in] modelId The model id to be applied by.
 * @returns return code
 */
NnRet CloseModel(uint32_t modelId) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->CloseModel(modelId);
}

/**
 *  @brief Shutdown NN framework
 *  @details This function destorys the Eden NN framework.
 *           Eden NN lets Eden Runtime know there is no more NN activity.
 *  @param void
 *  @returns return code
 */
NnRet Shutdown(void) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->Shutdown();
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
NnRet GetInputBufferShape(uint32_t modelId, int32_t inputIndex, int32_t* width, int32_t* height, int32_t* channel, int32_t* number) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->GetInputBufferShape(modelId, inputIndex, width, height, channel, number);
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
NnRet GetOutputBufferShape(uint32_t modelId, int32_t outputIndex, int32_t* width, int32_t* height, int32_t* channel, int32_t* number) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->GetOutputBufferShape(modelId, outputIndex, width, height, channel, number);
}

/**
 *  @brief Get Eden version
 *  @details This function gets EdenVersion with a current EDEN framework version.
 *           It includes hardware and software version too.
 *  @param[in] version It is representing for EDEN version information.
 *             This function is returned with version filled with a current information.
 *  @returns return code
 */
NnRet GetEdenVersion(uint32_t modelId, int32_t* versions) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->GetEdenVersion(modelId, versions);
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
NnRet GetCompileVersion(uint32_t modelId, EdenModelFile* modelFile, char versions[][VERSION_LENGTH_MAX]) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }
  return edenNN->GetCompileVersion(modelId, modelFile, versions);
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
NnRet GetCompileVersionFromMemory(ModelTypeInMemory typeInMemory, int8_t* addr, int32_t size, bool encrypted,
                                    char versions[][VERSION_LENGTH_MAX]) {
    if (edenNN == nullptr) {
        LOGE(EDEN_NN, "edenNN is NULL!\n");
        return RET_EDEN_NN_IS_NULL;
    }
    return edenNN->GetCompileVersionFromMemory(typeInMemory, addr, size, encrypted, versions);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// APIs not yet supported //////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

#if 0

/**
 *  @brief Get Eden capability
 *  @details This function gets EdenCapability with a current EDEN framework capability.
 *           It includes supported model file format, operators and devices.
 *  @param capability It is representing for EDEN capability information.
 *         This function is returned with capability filled with a current information.
 *  @returns return code
 */
NnRet GetCapability(EdenCapability* capability) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->GetCapability(capability);

  return RET_OK;
}

/**
 *  @brief Get Eden performance info
 *  @details This function gets EdenPerformanceInfo based on a current EDEN framework.
 *           The returned numbers contains static estimated information.
 *  @param[in] performanceInfo It is representing for Eden performance information.
 *         This function is returned with performance information.
 *  @returns return code
 */
NnRet GetPerformanceInfo(EdenPerformanceInfo* performanceInfo) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->GetPerformanceInfo(performanceInfo);

  return RET_OK;
}

/**
 *  @brief Set Eden preference
 *  @details This function sets EDEN_PREFERENCE of a specified EdenModel.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] preference The preference value
 *  @returns return code
 */
NnRet SetPreference(uint32_t modelId, EDEN_PREFERENCE preference) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->SetPreference(modelId, preference);

  return RET_OK;
}

/**
 *  @brief Set performance dump flag
 *  @details This function sets flags which performance data is dumped on execution.
 *           It can be enabled with multiple type.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] flag The preference flag which bit is representing for dump type.
 *  @returns return code
 */
NnRet SetPerformanceDump(uint32_t modelId, uint32_t flag) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->SetPerformanceDump(modelId, flag);

  return RET_OK;
}

/**
 *  @brief Set rate control information
 *  @details This function sets rate control information on a specified model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] rateControlInfo
 *  @returns return code
 */
NnRet SetRateControl(uint32_t modelId, EdenRateControlInfo* rateControlInfo) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->SetRateControl(modelId, rateControlInfo);

  return RET_OK;
}

/**
 *  @brief Set training information
 *  @details This function sets training information on a specified model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] trainingInfo The training information which represents for configurable parameters on training.
 *  @returns return code
 */
NnRet SetTrainingInfo(uint32_t modelId, EdenTrainingInfo* trainingInfo) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->SetTrainingInfo(modelId, trainingInfo);

  return RET_OK;
}

/**
 *  @brief Set input buffer data layout
 *  @details This function sets the input buffer data layout such as NCHW, NHWC or CUSTOM.
 *           Default value is NCHW.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] index Input buffer index.
 *  @param[in] layout Buffer layout (NCHW, NHWC, CUSTOM)
 *  @returns return code
 */
NnRet SetInputBufferLayout(uint32_t modelId, int32_t index, EDEN_BUFFER_LAYOUT layout) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->SetInputBufferLayout(modelId, index, layout);

  return RET_OK;
}

/**
 *  @brief Get the input buffer structure
 *  @details This function gets EdenBufferSetInfo for input buffers of a specified model.
 *           Returned bufferSetInfo contains input data structure and its name on model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[out] bufferSetInfo The data structure for representing buffer set
 *  @returns return code
 */
NnRet GetInputBufferSetInfo(uint32_t modelId, EdenBufferSetInfo* bufferSetInfo) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->GetInputBufferSetInfo(modelId, bufferSetInfo);

  return RET_OK;
}

/**
 *  @brief Get the output buffer structure
 *  @details This function gets EdenBufferSetInfo for output buffers of a specified model.
 *           Returned bufferSetInfo contains output data structure and its name on model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[out] bufferSetInfo The data structure representing for buffer set
 *  @returns return code
 */
NnRet GetOutputBufferSetInfo(uint32_t modelId, EdenBufferSetInfo* bufferSetInfo) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->GetOutputBufferSetInfo(modelId, bufferSetInfo);

  return RET_OK;
}

/**
 *  @brief Set buffer trait information
 *  @details This function sets a buffer traits such as image format, type etc to a specified buffer
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] bufferTraitInfo The data structure representing for buffer trait.
 *  @returns return code
 */
NnRet SetBufferTraitInfo(uint32_t modelId, int32_t index, EdenBufferTraitInfo* bufferTraitInfo) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->SetBufferTraitInfo(modelId, index, bufferTraitInfo);

  return RET_OK;
}

/**
 *  @brief Set buffer quantization
 *  @details This function sets a buffer quantizaion information such as Qm.n or scale.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] bufferQuantizeInfo The data structure representing for buffer quantization.
 *  @returns return code
 */
NnRet SetBufferQuantizeInfo(uint32_t modelId, int32_t index, EdenBufferQuantizeInfo* bufferQuantizeInfo) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->SetBufferQuantizeInfo(modelId, index, bufferQuantizeInfo);

  return RET_OK;
}

/**
 *  @brief Allocate a buffer to execute a model
 *  @details This function allocates an efficient buffer to execute a buffer.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] bufferSize Size of buffer.
 *  @param[out] addr Buffer address allocated.
 *  @returns return code
 */
NnRet AllocateBuffer(uint32_t modelId, int32_t bufferSize, void** addr) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->AllocateBuffer(modelId, bufferSize, addr);

  return RET_OK;
}

/**
 *  @brief Release a buffer
 *  @details This function releases a buffer returned by AllocateBuffer.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] addr Buffer address allocated
 *  @returns return code
 */
NnRet FreeBuffer(uint32_t modelId, void* addr) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->FreeBuffer(modelId, addr);

  return RET_OK;
}

/**
 *  @brief Get performance result
 *  @details This function gets a performance result on runtime.
 *           The data enabled by SetPerformanceDump is dumped.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] requestId Unique id representing for a request to dump performance.
 *  @param[out] performanceResult Data structure representing for performance result captured on runtime.
 *  @returns return code
 */
NnRet GetPerformanceResult(uint32_t modelId, uint32_t requestId, EdenPerformanceResult* performanceResult) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->GetPerformanceResult(modelId, requestId, performanceResult);

  return RET_OK;
}

/**
 *  @brief Get training result
 *  @details This function gets a training result on runtime.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] requestId Unique id representing for a request to get training result.
 *  @param[out] trainingResult Data structure representing for training result.
 *  @returns return code
 */
NnRet GetTrainingResult(uint32_t modelId, uint32_t requestId, EdenTrainingResult* trainingResult) {
  if (edenNN == nullptr) {
    LOGE(EDEN_NN, "edenNN is NULL!\n");
    return RET_EDEN_NN_IS_NULL;
  }

  edenNN->GetTrainingResult(modelId, requestId, trainingResult);

  return RET_OK;
}

#endif

