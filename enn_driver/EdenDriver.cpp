/*
 * Copyright (C) 2017 The Android Open Source Project
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


#include <iostream>
#include <vector>
#include <cstdint>  // uint32_t
#include <memory>   // nullptr

#include "log.h"

#include <hidl/LegacySupport.h>  // configureRpcThreadpool, joinRpcThreadpool

#include "ValidateHal.h"         // validateModel, validateExecutionPreference
#include "Utils.h"               // convertToV1_x, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "EdenDriver.h"          // EdenDriver
#include "NNAgent.h"             // NNAgent

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::EdenDriver"

namespace android {
namespace nn {
namespace eden_driver {

static void callNotifyWithError(const sp<V1_0::IPreparedModelCallback>& callback_1_0, V1_3::ErrorStatus status) {
    callback_1_0->notify(convertToV1_0(status), nullptr);
}

static void callNotifyWithError(const sp<V1_2::IPreparedModelCallback>& callback_1_2, V1_3::ErrorStatus status) {
    callback_1_2->notify_1_2(convertToV1_0(status), nullptr);
}

static void callNotifyWithError(const sp<V1_3::IPreparedModelCallback>& callback_1_3, V1_3::ErrorStatus status) {
    callback_1_3->notify_1_3(status, nullptr);
}

template <typename T_Model, typename T_Callback>
static Return<V1_3::ErrorStatus> prepareModelBaseOnEdenDriver(std::shared_ptr<NNAgent> nnAgent,
                                                                        const T_Model& model, ExecutionPreference preference,
                                                                        V1_3::Priority priority, const OptionalTimePoint& deadline,
                                                                        const hidl_vec<hidl_handle>* modelCache,
                                                                        const hidl_vec<hidl_handle>* dataCache,
                                                                        const HidlToken* token,
                                                                        const T_Callback& callback) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
#if 0
    if (VLOG_IS_ON(DRIVER)) {
        VLOG(DRIVER) << "prepareModelBaseOnEdenDriver";
        logModelToInfo(model);
    }
#endif
    if (callback.get() == nullptr) {
        LOGE(EDEN_DRIVER, "invalid callback passed to prepareModelBaseOnEdenDriver");
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model) || !validateExecutionPreference(preference) ||
                                 !validatePriority(priority)) {
        callNotifyWithError(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    if (!nnAgent->checkSupportedModel(convertToV1_3(model))) {
        callNotifyWithError(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        return V1_3::ErrorStatus::NONE;
    }

    int32_t retCode = nnAgent->prepareModel(convertToV1_3(model), preference,
                                            priority, deadline, modelCache, dataCache, token, callback);
    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "Error on preparedModel(), (retCode=%d)\n", retCode);
        callNotifyWithError(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_3::ErrorStatus::NONE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// IDEVICE_1_0 /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Edendriver_1_0 constructor
 * @details This function creates a NNAgent.
 */
EdenDriver_1_0::EdenDriver_1_0(const char* name) : name(name), nnAgent(nullptr) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    nnAgent = std::make_shared<NNAgent>();

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Edendriver_1_0 destructor
 * @details This function delete a NNAgent.
 */
EdenDriver_1_0::~EdenDriver_1_0() {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    nnAgent = nullptr;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Get capabilities for 1.0
 * @details This function gets capabilities from NN HAL.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<void> EdenDriver_1_0::getCapabilities(getCapabilities_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    return getCapabilities_1_1(
        [&](V1_0::ErrorStatus error, const V1_1::Capabilities& capabilities) {
            // TODO(dgross): Do we need to check compliantWithV1_0(capabilities)?
            cb(error, convertToV1_0(capabilities));
        });
}

/**
 * @brief Get supported operations on a given model for 1.0
 * @details This function checks operations on a given model and returns which operations are supported.
 * @param[in] model Android NN Model.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<void> EdenDriver_1_0::getSupportedOperations(const V1_0::Model& model,
                                                getSupportedOperations_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (!validateModel(model)) {
        std::vector<bool> supportedOperations;

        cb(V1_0::ErrorStatus::INVALID_ARGUMENT, supportedOperations);
        return Void();
    }

    return getSupportedOperations_1_1(convertToV1_1(model), cb);
}

/**
 * @brief Prepare model matched to a given Android NN Model for 1.0
 * @details This function prepares a model matched to a given Android NN Model.
 * @param[in] model Android NN Model.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<V1_0::ErrorStatus> EdenDriver_1_0::prepareModel(const V1_0::Model& model,
                                             const sp<V1_0::IPreparedModelCallback>& callback_1_0) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (callback_1_0.get() == nullptr) {
        LOGE(EDEN_DRIVER, "invalid callback passed to prepareModel\n");
        return V1_0::ErrorStatus::INVALID_ARGUMENT;
    }
    if (!validateModel(model)) {
        callback_1_0->notify(V1_0::ErrorStatus::INVALID_ARGUMENT, nullptr);
        return V1_0::ErrorStatus::INVALID_ARGUMENT;
    }
    return prepareModel_1_1(convertToV1_1(model), ExecutionPreference::FAST_SINGLE_ANSWER, callback_1_0);
}

/**
 * @brief Get status of NN HAL
 * @details This function returns status of NN HAL.
 * @returns DeviceStatus
 */
Return<DeviceStatus> EdenDriver_1_0::getStatus() {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    DeviceStatus status;
    nnAgent->getStatus(status);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return status;
}

/**
 * @brief main function to start NN HAL as a service
 * @details This function is a main function to be launched as a service.
 * @returns return code
 */
int EdenDriver_1_0::run() {
    android::hardware::configureRpcThreadpool(4, true);
    if (registerAsService(name) != android::OK) {
        LOGE(EDEN_DRIVER, "Could not register service %s\n", name.c_str());
        return 1;
    } else {
        LOGI(EDEN_DRIVER, "Register service %s\n", name.c_str());
    }
    android::hardware::joinRpcThreadpool();
    LOGI(EDEN_DRIVER, "Service exited!\n");
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// IDEVICE_1_1 /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Edendriver_1_1 constructor
 * @details This function creates a NNAgent.
 */
EdenDriver_1_1::EdenDriver_1_1(const char* name) : EdenDriver_1_0(name) {
    LOGD(EDEN_DRIVER, "%s(+-)\n", __func__);
}

/**
 * @brief Edendriver_1_1 destructor
 * @details This function delete a NNAgent.
 */
EdenDriver_1_1::~EdenDriver_1_1() {
    LOGD(EDEN_DRIVER, "%s(+-)\n", __func__);
}

/**
 * @brief Get capabilities for 1.1
 * @details This function gets capabilities from NN HAL.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<void> EdenDriver_1_1::getCapabilities_1_1(getCapabilities_1_1_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    return getCapabilities_1_2(
        [&](V1_0::ErrorStatus error, const V1_2::Capabilities& capabilities) {
            // TODO(dgross): Do we need to check compliantWithV1_0(capabilities)?
            cb(error, convertToV1_1(capabilities));
        });
}

/**
 * @brief Get supported operations on a given model for 1.1
 * @details This function checks operations on a given model and returns which operations are supported.
 * @param[in] model Android NN Model.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<void> EdenDriver_1_1::getSupportedOperations_1_1(const V1_1::Model& model,
                                                    getSupportedOperations_1_1_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (!validateModel(model)) {
        std::vector<bool> supportedOperations;
        cb(V1_0::ErrorStatus::INVALID_ARGUMENT, supportedOperations);
        return Void();
    }

    return getSupportedOperations_1_2(convertToV1_2(model), cb);
}

/**
 * @brief Prepare model matched to a given Android NN Model for 1.1
 * @details This function prepares a model matched to a given Android NN Model.
 * @param[in] model Android NN Model.
 * @param[in] preference ExecutionPreference
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<V1_0::ErrorStatus> EdenDriver_1_1::prepareModel_1_1(const V1_1::Model& model, ExecutionPreference preference,
                                                   const sp<V1_0::IPreparedModelCallback>& callback_1_0) {
    return convertToV1_0(prepareModelBaseOnEdenDriver(this->nnAgent, model, preference, convertToV1_3(Priority::MEDIUM),
                                                      {}, nullptr, nullptr, nullptr, callback_1_0));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// IDEVICE_1_2 /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Edendriver constructor
 * @details This function creates a NNAgent.
 */
EdenDriver_1_2::EdenDriver_1_2(const char* name) : EdenDriver_1_1(name) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
}

/**
 * @brief Edendriver destructor
 * @details This function delete a NNAgent.
 */
EdenDriver_1_2::~EdenDriver_1_2() {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
}

Return<void> EdenDriver_1_2::getVersionString(getVersionString_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    cb(V1_0::ErrorStatus::NONE, "EdenDriver_1_3");

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

Return<void> EdenDriver_1_2::getType(getType_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    cb(V1_0::ErrorStatus::NONE, V1_2::DeviceType::ACCELERATOR);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

/**
 * @brief Get capabilities for 1.2
 * @details This function gets capabilities from NN HAL.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<void> EdenDriver_1_2::getCapabilities_1_2(getCapabilities_1_2_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    return getCapabilities_1_3(
        [&](V1_3::ErrorStatus error, const V1_3::Capabilities& capabilities) {
            // TODO(dgross): Do we need to check compliantWithV1_0(capabilities)?
            cb(convertToV1_0(error), convertToV1_2(capabilities));
        });
}

Return<void> EdenDriver_1_2::getSupportedExtensions(getSupportedExtensions_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    cb(V1_0::ErrorStatus::NONE, {/* No extensions. */});

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

/**
 * @brief Get supported operations on a given model for 1.2
 * @details This function checks operations on a given model and returns which operations are supported.
 * @param[in] model Android NN Model.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<void> EdenDriver_1_2::getSupportedOperations_1_2(const V1_2::Model& model,
                                                    getSupportedOperations_1_2_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (!validateModel(model)) {
        std::vector<bool> supportedOperations;
        cb(V1_0::ErrorStatus::INVALID_ARGUMENT, supportedOperations);
        return Void();
    }

    return getSupportedOperations_1_3(convertToV1_3(model),
                                      [&](V1_3::ErrorStatus error, std::vector<bool> supportedOperations) {
                                          cb(convertToV1_0(error), supportedOperations);
                                      });
}

Return<void> EdenDriver_1_2::getNumberOfCacheFilesNeeded(getNumberOfCacheFilesNeeded_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    cb(V1_0::ErrorStatus::NONE, /*numModelCache=*/0, /*numDataCache=*/0);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

/**
 * @brief Prepare model matched to a given Android NN Model for 1.2
 * @details This function prepares a model matched to a given Android NN Model.
 * @param[in] model Android NN Model.
 * @param[in] preference ExecutionPreference
 * @param[in] modelCache
 * @param[in] dataCache
 * @param[in] token
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<V1_0::ErrorStatus> EdenDriver_1_2::prepareModel_1_2(const V1_2::Model& model, ExecutionPreference preference,
                                                                     const hidl_vec<hidl_handle>& modelCache,
                                                                     const hidl_vec<hidl_handle>& dataCache,
                                                                     const HidlToken& token,
                                                                     const sp<V1_2::IPreparedModelCallback>& callback_1_2) {
    return convertToV1_0(prepareModelBaseOnEdenDriver(this->nnAgent, model, preference, convertToV1_3(Priority::MEDIUM),
                                                      {}, &modelCache, &dataCache, &token, callback_1_2));
}

Return<V1_0::ErrorStatus> EdenDriver_1_2::prepareModelFromCache(const hidl_vec<hidl_handle>& /*modelCache*/,
                                                      const hidl_vec<hidl_handle>& /*dataCache*/,
                                                      const HidlToken& /*token*/,
                                                      const sp<V1_2::IPreparedModelCallback>& callback_1_2) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    callback_1_2->notify_1_2(V1_0::ErrorStatus::GENERAL_FAILURE, nullptr);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_0::ErrorStatus::NONE;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// IDEVICE_1_3 /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Edendriver constructor
 * @details This function creates a NNAgent.
 */
EdenDriver::EdenDriver(const char* name) : EdenDriver_1_2(name) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
}

/**
 * @brief Edendriver destructor
 * @details This function delete a NNAgent.
 */
EdenDriver::~EdenDriver() {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
}

/**
 * @brief Get capabilities for 1.3
 * @details This function gets capabilities from NN HAL.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<void> EdenDriver::getCapabilities_1_3(getCapabilities_1_3_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    android::nn::initVLogMask();

    V1_3::Capabilities capabilities;
    int32_t retCode = nnAgent->getCapabilities(capabilities);
    if (retCode != RET_OK) {
        LOGE(EDEN_DRIVER, "Error on getCapabilities(), (retCode=%d)\n", retCode);
        cb(V1_3::ErrorStatus::INVALID_ARGUMENT, capabilities);
    } else {
        cb(V1_3::ErrorStatus::NONE, capabilities);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

/**
 * @brief Get supported operations on a given model for 1.3
 * @details This function checks operations on a given model and returns which operations are supported.
 * @param[in] model Android NN Model.
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<void> EdenDriver::getSupportedOperations_1_3(const V1_3::Model& model,
                                                        getSupportedOperations_1_3_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::vector<bool> supportedOperations;
    V1_3::ErrorStatus status = V1_3::ErrorStatus::NONE;
    if (validateModel(model)) {
        int32_t retCode = nnAgent->getSupportedOperations(model, supportedOperations);
        if (retCode != RET_OK) {
            LOGE(EDEN_DRIVER, "Error on getSupportedOperations(), (retCode=%d)\n", retCode);
            status = V1_3::ErrorStatus::INVALID_ARGUMENT;
        }
    } else {
        status = V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    cb(status, supportedOperations);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

/**
 * @brief Prepare model matched to a given Android NN Model for 1.3
 * @details This function prepares a model matched to a given Android NN Model.
 * @param[in] model Android NN Model.
 * @param[in] preference ExecutionPreference
 * @param[in] modelCache
 * @param[in] dataCache
 * @param[in] token
 * @param[in] cb callback to be executed.
 * @returns return code
 */
Return<V1_3::ErrorStatus> EdenDriver::prepareModel_1_3(const V1_3::Model& model, ExecutionPreference preference,
                                                                 V1_3::Priority priority, const OptionalTimePoint& deadline,
                                                                 const hidl_vec<hidl_handle>& modelCache,
                                                                 const hidl_vec<hidl_handle>& dataCache,
                                                                 const HidlToken& token,
                                                                 const sp<V1_3::IPreparedModelCallback>& callback_1_3) {
    return convertToV1_3(prepareModelBaseOnEdenDriver(this->nnAgent, model, preference, priority,
                                                      deadline, &modelCache, &dataCache, &token, callback_1_3));
}

Return<V1_3::ErrorStatus> EdenDriver::prepareModelFromCache_1_3(const OptionalTimePoint& /*deadline*/,
                                                                         const hidl_vec<hidl_handle>& /*modelCache*/,
                                                                         const hidl_vec<hidl_handle>& /*dataCache*/,
                                                                         const HidlToken& /*token*/,
                                                                         const sp<V1_3::IPreparedModelCallback>& callback_1_3) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    callback_1_3->notify_1_3(V1_3::ErrorStatus::GENERAL_FAILURE, nullptr);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_3::ErrorStatus::NONE;
}

/**
 * @brief allocates a device buffer
 * @details Implements allocate() API
 * @param[in] desc A buffer descriptor specifying the properties of the buffer to allocate.
 * @param[in] preparedModels A vector of IPreparedModel objects.
 * @param[in] inputRoles A vector of roles with each specifying an input to a prepared model.
 * @param[in]  outputRoles A vector of roles with each specifying an output to a prepared model.
 * @return Error Status
 * @return IBuffer instance of allocated buffer
 * @return Unique toke associated with allocated buffer
 */
Return<void> EdenDriver::allocate(const V1_3::BufferDesc& desc,
                                       const hidl_vec<sp<V1_3::IPreparedModel>>& preparedModels,
                                       const hidl_vec<V1_3::BufferRole>& inputRoles,
                                       const hidl_vec<V1_3::BufferRole>& outputRoles,
                                       allocate_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    nnAgent->allocate(desc, preparedModels, inputRoles, outputRoles, cb);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

/**
 * @brief Retrieves the content of this buffer to a shared memory region.
 * @details This function copies data from device buffer to the given shared memory
 * @param[in] dst The destination shared memory region
 * @returns Error Status
 */
Return<V1_3::ErrorStatus> ENNBuffer::copyTo(const hardware::hidl_memory& dst) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    return mnnAgent->copyToInternal(kBuffer, dst);
}

/**
 * @brief Sets the content of this buffer from a shared memory region.
 * @details This function copies data from shared memory to the device buffer
 * @param[in] src The source shared memory region
 * @param[in] dimensions Updated dimensional information
 * @returns None
 */
Return<V1_3::ErrorStatus> ENNBuffer::copyFrom(const hardware::hidl_memory& src,
                                                        const hidl_vec<uint32_t>& dimensions) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (kBuffer == nullptr) {
        LOGE(EDEN_DRIVER, "ENNBuffer:: Invalid buffer\n");
        return V1_3::ErrorStatus::GENERAL_FAILURE;
    }

    return mnnAgent->copyFromInternal(kBuffer, src, dimensions);
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

