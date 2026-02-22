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
 * @file    EnnDevice.h
 * @brief   This file has definitions for AndroidNN IDevice interface
 * @details This file has definitions for AndroidNN IDevice interface
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#include <nnapi/IBuffer.h>
#include <nnapi/IDevice.h>
#include <nnapi/IPreparedModel.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <nnapi/Validation.h>
#include <EnnDevice.h>
#include "NNAgent.h"
#include "Common.h"
#include "log.h"

using namespace ::android::nn;

namespace android {
namespace nn {
namespace enn_driver {


/**
 * @brief EnnDevice constructor
 * @details This function creates a NNAgent.
 */
EnnDevice::EnnDevice(std::string name)
    : name(std::move(name)), nnAgent(nullptr) {
    LOGI(ENN_DRIVER, "%s(+)\n", __func__);
    LOGI(ENN_DRIVER, "OnDevice_GG version %s", ONDEVICE_GG);
#ifdef GPU_HW
    HwType hwtype = CPU_GPU;
#elif NPU_HW
    HwType hwtype = NPU_DSP;
#elif ENN_HW
    HwType hwtype = NPU_GPU;
#endif
    nnAgent = std::make_shared<NNAgent>(hwtype);

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief EnnDevice destructor
 * @details This function delete a NNAgent.
 */
EnnDevice::~EnnDevice() {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    nnAgent = nullptr;

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}
const std::string& EnnDevice::getName() const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    return name;
}

const std::string& EnnDevice::getVersionString() const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
#ifdef GPU_HW
    static const std::string kVersionString = "G0.0.1";
#elif NPU_HW
    static const std::string kVersionString = "A0.0.1";
#elif ENN_HW
    static const std::string kVersionString = "E0.0.1";
#endif
    return kVersionString;
}

Version EnnDevice::getFeatureLevel() const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    return kVersionFeatureLevel8;
}

DeviceType EnnDevice::getType() const {
#ifdef GPU_HW
    return DeviceType::GPU;
#elif NPU_HW
    return DeviceType::ACCELERATOR;
#elif ENN_HW
    return DeviceType::ACCELERATOR;
#endif
}

const std::vector<Extension>& EnnDevice::getSupportedExtensions() const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    static const std::vector<Extension> kExtensions = {/*No Extensions */ };
	LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return kExtensions;
}

const Capabilities& EnnDevice::getCapabilities() const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    static const Capabilities kCapabilities = nnAgent->getCapabilities();
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return kCapabilities;
}

std::pair<uint32_t, uint32_t> EnnDevice::getNumberOfCacheFilesNeeded() const {
    return std::make_pair(/*numModelCache=*/0, /*numDataCache=*/0);
}

GeneralResult<void> EnnDevice::wait() const {
    /**
     * Blocks until the device is not in a bad state.*/
    // ToDo @nihar.desai Check more and Check if this API need to be implemented.
    return {};
}

GeneralResult<std::vector<bool>> EnnDevice::getSupportedOperations(const Model& model) const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    const auto result = validate(model);
    if (!result.ok()) {
       LOGE(ENN_DRIVER, "getSupportedOperations() failed:%s", result.error().c_str());
       return NN_ERROR(ErrorStatus::INVALID_ARGUMENT);
    }

    std::vector<bool> supportedOperations;
    int32_t retCode = nnAgent->getSupportedOperations(model, supportedOperations);
    if ( retCode != RET_OK) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE);
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return supportedOperations;
}

GeneralResult<SharedPreparedModel> EnnDevice::prepareModel(const Model& model,
                                                    ExecutionPreference preference,
                                                    Priority priority, OptionalTimePoint deadline,
                                                    const std::vector<SharedHandle>& modelCache,
                                                    const std::vector<SharedHandle>& dataCache,
                                                    const CacheToken& token,
                                                    const std::vector<nn::TokenValuePair>& hints,
                                                    const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const {
   LOGD(ENN_DRIVER, "%s(+)\n", __func__);
   // Validate arguments.
   const auto model_result = validate(model);
   if (!model_result.ok()) {
        LOGE(ENN_DRIVER, "prepareModel() failed:%s", model_result.error().c_str());
        return  NN_ERROR(ErrorStatus::INVALID_ARGUMENT);
    }

    const auto preference_result = validate(preference);
    if (!preference_result.ok()) {
        LOGE(ENN_DRIVER, "prepareModel() failed:%s", preference_result.error().c_str());
        return  NN_ERROR(ErrorStatus::INVALID_ARGUMENT);
    }

    const auto priority_result = validate(priority);
    if (!priority_result.ok()) {
        LOGE(ENN_DRIVER, "prepareModel() failed:%s", priority_result.error().c_str());
        return  NN_ERROR(ErrorStatus::INVALID_ARGUMENT);
    }

    std::shared_ptr<IPreparedModel> rPreparedModel;
    int32_t retCode =  nnAgent->prepareModel(model, preference, priority,
                                            deadline, &modelCache, &dataCache, &token, rPreparedModel);

    if (retCode != RET_OK) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE);
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return rPreparedModel;
}

GeneralResult<SharedPreparedModel> EnnDevice::prepareModelFromCache(
            OptionalTimePoint deadline, const std::vector<SharedHandle>& modelCache,
            const std::vector<SharedHandle>& dataCache, const CacheToken& token) const {

        return  NN_ERROR(ErrorStatus::GENERAL_FAILURE);
}

GeneralResult<SharedBuffer> EnnDevice::allocate(const BufferDesc& desc,
                                         const std::vector<SharedPreparedModel>& preparedModels,
                                         const std::vector<BufferRole>& inputRoles,
                                         const std::vector<BufferRole>& outputRoles) const {

   LOGD(ENN_DRIVER, "%s(+)\n", __func__);
   std::shared_ptr<DeviceBuffer> deviceBuffer;
   ErrorStatus ret = nnAgent->allocate(desc, preparedModels, inputRoles, outputRoles, deviceBuffer);
   if (ret != ErrorStatus::NONE) {
       return NN_ERROR(ret);
   }
   return deviceBuffer;
   LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}


Request::MemoryDomainToken DeviceBuffer::getToken() const {
     return Request::MemoryDomainToken{ktoken->get()};
}

GeneralResult<void> DeviceBuffer::copyTo(const SharedMemory& dst) const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    ErrorStatus ret;
    ret =  mnnAgent->copyToInternal(kBuffer, dst);
    if (ret != ErrorStatus::NONE) {
        return NN_ERROR(ret);
    }
    return {};
}

/**
 * @brief Sets the content of this buffer from a shared memory region.
 * @details This function copies data from shared memory to the device buffer
 * @param[in] src The source shared memory region
 * @param[in] dimensions Updated dimensional information
 * @returns None
 */
GeneralResult<void> DeviceBuffer::copyFrom(const SharedMemory& src,
                                           const Dimensions& dimensions) const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    if (kBuffer == nullptr) {
        LOGE(ENN_DRIVER, "DeviceBuffer:: Invalid buffer\n");
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE);
    }
    ErrorStatus ret =  mnnAgent->copyFromInternal(kBuffer, src, dimensions);
    if (ret != ErrorStatus::NONE) {
        return NN_ERROR(ret);
    }
    return {};

}

std::vector<SharedDevice> getDevices() {
#ifdef GPU_HW
    auto gpu_device = std::make_shared<enn_driver::EnnDevice>("enn-gpu");
    return {gpu_device};
#elif  NPU_HW
    auto npu_device = std::make_shared<enn_driver::EnnDevice>("enn-npu");
    return {npu_device};
#elif  ENN_HW
    auto enn_device = std::make_shared<enn_driver::EnnDevice>("enn");
    return {enn_device};
#endif

}
}  // namespace enn_driver
}  // namespace nn
}  // namespace android

