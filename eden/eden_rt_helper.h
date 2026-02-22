/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file eden_rt_helper.h
 * @brief implementation of eden_rt_stub api
 */

#include "EdenRuntime.h"  // RtRet, ModelTypeInMemory, ModelPreference

namespace eden {
namespace rt {

/**
 *  @brief Initialize helper functions
 *  @details This function processes an initialization on helper functions.
 *  @returns return code
 */
RtRet InitHelper();

/**
 *  @brief Deinitialize helper functions
 *  @details This function processes a deinitialization on helper functions.
 *  @returns return code
 */
RtRet DeinitHelper();

/**
 *  @brief Prepare hidlData to request OpenModelFromMemory.
 *  @details This function prepares hidlData to request OpenModelFromMemory.
 *           It creates opaque data structure, memcpy userdata into ION buffer.
 *           Then it completes preparing a hidlData and returns it
 *  @param[in] modelTypeInMemory it is representing for in-memory model such as Android NN Model.
 *  @param[in] addr address of in-memory model
 *  @param[in] size size of in-memory model
 *  @param[in] encrypted data on addr is encrypted
 *  @param[in] preference It is representing for a model preference.
 *  @param[out] hidlData Opaque data
 *  @returns return code
 */
RtRet PrepareOpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                                 bool encrypted, const EdenModelOptions& options, void** hidlData);

/**
 *  @brief Call service function for OpenModelFromMemory
 *  @details This function calls service's function for OpenModelFromMemory.
 *           hidlData returned by PrepareOpenModelFromMemory should be delivered.
 *  @param[in] hidlData Opaque data
 *  @param[out] modelId It is representing for constructed EdenModel with a unique id.
 *  @returns return code
 */
RtRet CallOpenModelFromMemory(void* hidlData, uint32_t* modelId);

/**
 *  @brief Unprepare resources for OpenModelFromMemory
 *  @details This function releases resources allocated/prepared for OpenModelFromMemory.
 *           hidlData returned by PrepareOpenModelFromMemory should be delivered.
 *  @param[in] hidlData Opaque data
 *  @returns return code
 */
RtRet UnprepareOpenModelFromMemory(void* hidlData);

RtRet PrepareGetCompileVersionFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                                        bool encrypted, void** hidlData);
RtRet CallGetCompileVersionFromMemory(void* hidlData, char versions[][VERSION_LENGTH_MAX]);
RtRet UnprepareGetCompileVersionFromMemory(void* hidlData);

}  // rt
}  // eden
