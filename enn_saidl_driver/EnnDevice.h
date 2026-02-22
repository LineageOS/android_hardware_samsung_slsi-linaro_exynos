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

#ifndef DRIVER_ENNDRIVER_H_
#define DRIVER_ENNDRIVER_H_

#include <nnapi/IDevice.h>
#include <nnapi/IBuffer.h>
#include <nnapi/OperandTypes.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>
#include <NeuralNetworks.h>
#include <BufferManager.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>

using namespace ::android::nn;
namespace android {
namespace nn {
namespace enn_driver {


class NNAgent;

class EnnDevice final : public IDevice {
 public:
    explicit EnnDevice(std::string name);
    ~EnnDevice() override;

    const std::string& getName() const override;
    const std::string& getVersionString() const override;
    Version getFeatureLevel() const override;
    DeviceType getType() const override;
    const std::vector<Extension>& getSupportedExtensions() const override;
    const Capabilities& getCapabilities() const override;
    std::pair<uint32_t, uint32_t> getNumberOfCacheFilesNeeded() const override;

    GeneralResult<void> wait() const override;

    GeneralResult<std::vector<bool>> getSupportedOperations(const Model& model) const override;

    GeneralResult<SharedPreparedModel> prepareModel(const Model& model,
                                        ExecutionPreference preference,
                                        Priority priority, OptionalTimePoint deadline,
                                        const std::vector<SharedHandle>& modelCache,
                                        const std::vector<SharedHandle>& dataCache,
                                        const CacheToken& token,
                                        const std::vector<nn::TokenValuePair>& hints,
                                        const std::vector<nn::ExtensionNameAndPrefix>& extensionNameToPrefix) const override;

    GeneralResult<SharedPreparedModel> prepareModelFromCache(
            OptionalTimePoint deadline, const std::vector<SharedHandle>& modelCache,
            const std::vector<SharedHandle>& dataCache, const CacheToken& token) const override;

    GeneralResult<SharedBuffer> allocate(const BufferDesc& desc,
                                         const std::vector<SharedPreparedModel>& preparedModels,
                                         const std::vector<BufferRole>& inputRoles,
                                         const std::vector<BufferRole>& outputRoles) const override;

   private:
    const std::string name;
//    const std::shared_ptr<ManagedBuffer> kBuffer = BufferManager::create();
    std::shared_ptr<NNAgent> nnAgent;
};


class DeviceBuffer : public IBuffer {
 public:
    explicit DeviceBuffer(std::shared_ptr<ManagedBuffer> buffer,
                          std::unique_ptr<BufferTracker::Token> token,
                          NNAgent* nnAgent):
       kBuffer(std::move(buffer)),
       ktoken(std::move(token)),
       mnnAgent(nnAgent) {}

    Request::MemoryDomainToken getToken() const override;
    GeneralResult<void> copyTo(const SharedMemory& dst) const  override;
    GeneralResult<void>  copyFrom(const SharedMemory& src,
                                  const Dimensions& dimensions) const override;
 private:
    const std::shared_ptr<ManagedBuffer> kBuffer;
    const std::unique_ptr<BufferTracker::Token> ktoken;
    NNAgent* mnnAgent;
};

std::vector<SharedDevice> getDevices();
}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_ENNDRIVER_H_
