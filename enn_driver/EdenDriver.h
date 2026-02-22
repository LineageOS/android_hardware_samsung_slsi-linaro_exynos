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

#ifndef DRIVER_EDENDRIVER_H_
#define DRIVER_EDENDRIVER_H_

#include <string>

#include "HalInterfaces.h"   // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
#include "NeuralNetworks.h"  // ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN
#include "BufferManager.h"

namespace android {
namespace nn {
namespace eden_driver {

using ::android::hardware::Return;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_vec;
using ::android::hardware::neuralnetworks::V1_0::DeviceStatus;
using ::android::hardware::neuralnetworks::V1_1::ExecutionPreference;
using ::android::hardware::neuralnetworks::V1_3::OptionalTimePoint;
using HidlToken = hardware::hidl_array<uint8_t, ANEURALNETWORKS_BYTE_SIZE_OF_CACHE_TOKEN>;


class NNAgent;

class EdenDriver_1_0 : public V1_3::IDevice {
 public:
    explicit EdenDriver_1_0(const char* name);
    ~EdenDriver_1_0() override;

    // IDevice@1.0
    Return<void> getCapabilities(getCapabilities_cb cb) override;
    Return<void> getSupportedOperations(const V1_0::Model& model,
                                        getSupportedOperations_cb cb) override;
    Return<V1_0::ErrorStatus> prepareModel(const V1_0::Model& model,
                                     const sp<V1_0::IPreparedModelCallback>& callback_1_0) override;
    Return<DeviceStatus> getStatus() override;

    // Starts and runs the driver service. Typically called from main().
    // This will return only once the service shuts down.
    int run();

 protected:
    std::string name;
    std::shared_ptr<NNAgent> nnAgent;
};

class EdenDriver_1_1 : public EdenDriver_1_0 {
 public:
    explicit EdenDriver_1_1(const char* name);
    ~EdenDriver_1_1() override;

    // IDevice@1.1
    Return<void> getCapabilities_1_1(getCapabilities_1_1_cb cb) override;
    Return<void> getSupportedOperations_1_1(const V1_1::Model& model,
                                            getSupportedOperations_1_1_cb cb) override;
    Return<V1_0::ErrorStatus> prepareModel_1_1(const V1_1::Model& model, ExecutionPreference preference,
                                         const sp<V1_0::IPreparedModelCallback>& callback_1_0) override;
};

class EdenDriver_1_2 : public EdenDriver_1_1 {
 public:
    explicit EdenDriver_1_2(const char* name);
    ~EdenDriver_1_2() override;

    // IDevice@1.2
    Return<void> getVersionString(getVersionString_cb cb) override;
    Return<void> getType(getType_cb cb) override;
    Return<void> getCapabilities_1_2(getCapabilities_1_2_cb cb) override;
    Return<void> getSupportedExtensions(getSupportedExtensions_cb) override;
    Return<void> getSupportedOperations_1_2(const V1_2::Model& model,
                                                 getSupportedOperations_1_2_cb cb) override;
    Return<void> getNumberOfCacheFilesNeeded(getNumberOfCacheFilesNeeded_cb cb) override;
    Return<V1_0::ErrorStatus> prepareModel_1_2(const V1_2::Model& model, ExecutionPreference preference,
                                                         const hidl_vec<hidl_handle>& modelCache,
                                                         const hidl_vec<hidl_handle>& dataCache,
                                                         const HidlToken& token,
                                                         const sp<V1_2::IPreparedModelCallback>& callback_1_2) override;
    Return<V1_0::ErrorStatus> prepareModelFromCache(const hidl_vec<hidl_handle>& modelCache,
                                                              const hidl_vec<hidl_handle>& dataCache,
                                                              const HidlToken& token,
                                                              const sp<V1_2::IPreparedModelCallback>& callback_1_2) override;
};

class EdenDriver : public EdenDriver_1_2 {
 public:
    explicit EdenDriver(const char* name);
    ~EdenDriver() override;

    // IDevice@1.3
    Return<void> getCapabilities_1_3(getCapabilities_1_3_cb cb) override;
    Return<void> getSupportedOperations_1_3(const V1_3::Model& model,
                                                 getSupportedOperations_1_3_cb cb) override;
    Return<V1_3::ErrorStatus> prepareModel_1_3(const V1_3::Model& model, ExecutionPreference preference,
                                                         V1_3::Priority priority, const OptionalTimePoint& deadline,
                                                         const hidl_vec<hidl_handle>& modelCache,
                                                         const hidl_vec<hidl_handle>& dataCache,
                                                         const HidlToken& token,
                                                         const sp<V1_3::IPreparedModelCallback>& callback_1_3) override;
    Return<V1_3::ErrorStatus> prepareModelFromCache_1_3(const OptionalTimePoint& deadline,
                                                                  const hidl_vec<hidl_handle>& modelCache,
                                                                  const hidl_vec<hidl_handle>& dataCache,
                                                                  const HidlToken& token,
                                                                  const sp<V1_3::IPreparedModelCallback>& callback_1_3) override;
    Return<void> allocate(const V1_3::BufferDesc& desc,
                               const hidl_vec<sp<V1_3::IPreparedModel>>& preparedModels,
                               const hidl_vec<V1_3::BufferRole>& inputRoles,
                               const hidl_vec<V1_3::BufferRole>& outputRoles,
                               allocate_cb cb) override;
};

class ENNBuffer : public V1_3::IBuffer {
 public:
    explicit ENNBuffer(std::shared_ptr<ManagedBuffer> buffer,
                       std::unique_ptr<BufferTracker::Token> token,
                       NNAgent* nnAgent):
       kBuffer(std::move(buffer)),
       ktoken(std::move(token)),
       mnnAgent(nnAgent) {}

    Return<V1_3::ErrorStatus> copyTo(const hardware::hidl_memory& dst) override;
    Return<V1_3::ErrorStatus> copyFrom(const hardware::hidl_memory& src,
                                                    const hidl_vec<uint32_t>& dimensions) override;
 private:
    const std::shared_ptr<ManagedBuffer> kBuffer;
    const std::unique_ptr<BufferTracker::Token> ktoken;
    NNAgent* mnnAgent;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_EDENDRIVER_H_

