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
 * @file    DspPreparedModel.h
 * @brief   This is DspPreparedModel class file.
 * @details This header implements IPreparedModel interface.
 */

#ifndef DRIVER_DSPPREPAREDMODEL_H_
#define DRIVER_DSPPREPAREDMODEL_H_

#include <map>
#include <vector>
#include <cstdint>  // int32_t

#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
#include "BufferInfoOnExecute.h"
#include <ofi_mm_memory-public.h>
#include <ofi_api-public.h>

namespace android {
namespace nn {
namespace eden_driver {

using ::android::hardware::MQDescriptorSync;

class NNAgent;

class DspPreparedModel : public hal::V1_3::IPreparedModel {
 public:
    explicit DspPreparedModel(const hal::V1_3::Model& model);
    ~DspPreparedModel() override;

    hal::Return<void> setPreparedModel(const char* path);
    hal::Return<void> setPreparedModel(char *buffer, int size);

    // IPreparedModel@1.0
    hal::Return<hal::V1_0::ErrorStatus> execute(const hal::V1_0::Request& request,
            const sp<hal::V1_0::IExecutionCallback>& callback) override;

    // IPreparedModel@1.2
    hal::Return<hal::V1_0::ErrorStatus> execute_1_2(const hal::V1_0::Request& request,
            hal::V1_2::MeasureTiming measure,
            const sp<hal::V1_2::IExecutionCallback>& callback) override;

    // IPreparedModel@1.3
    hal::Return<hal::V1_3::ErrorStatus> execute_1_3(const hal::V1_3::Request& request,
            hal::V1_2::MeasureTiming measure,
            const hal::V1_3::OptionalTimePoint& deadline,
            const hal::V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
            const sp<hal::V1_3::IExecutionCallback>& callback_1_3) override;

    // IPreparedModel@1.2
    hal::Return<void> executeSynchronously(const hal::V1_0::Request& request,
            hal::V1_2::MeasureTiming measure,
            executeSynchronously_cb cb) override;

    // IPreparedModel@1.3
    hal::Return<void> executeSynchronously_1_3(const hal::V1_3::Request& request,
            hal::V1_2::MeasureTiming measure,
            const hal::V1_3::OptionalTimePoint& deadline,
            const hal::V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
            executeSynchronously_1_3_cb cb) override;

    // IPreparedModel@1.2
    hal::Return<void> configureExecutionBurst(const hal::sp<hal::V1_2::IBurstCallback>& callback,
            const MQDescriptorSync<hal::V1_2::FmqRequestDatum>& requestChannel,
            const MQDescriptorSync<hal::V1_2::FmqResultDatum>& resultChannel,
            configureExecutionBurst_cb _hidl_cb) override;

    // IPreparedModel@1.3
    hal::Return<void> executeFenced(const hal::V1_3::Request& request,
            const hal::hidl_vec<hal::hidl_handle>& waitFor,
            hal::V1_2::MeasureTiming measure,
            const hal::V1_3::OptionalTimePoint& deadline,
            const hal::V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
            const hal::V1_3::OptionalTimeoutDuration& duration,
            DspPreparedModel::executeFenced_cb cb) override;

    int32_t loadRequestData(const hal::V1_3::Request& request,
            BufferInfoOnExecute& bufferInfoOnExecute,
            int numOfExecution,
            ofi_memory*** ofiInputs,
            ofi_memory*** ofiOutputs);
    int32_t updateOutputRequest(const hal::V1_3::Request& request,
            BufferInfoOnExecute& bufferInfoOnExecute,
            int numOfExecution,
            ofi_memory** ofiOutputs);
    int32_t getNumOfExecution(const hal::V1_3::Request& request);
    uint32_t getModelId();

 private:
    void getDimensions(const hal::V1_3::Operand& operand,
            int32_t* dimsOfDsp,
            bool isNchw = false);
    int32_t loadTargetBuffers(int32_t n,
            ofi_memory** inputs,
            void* srcAddr,
            int32_t length,
            int32_t index);
    int32_t createTargetBuffers(int32_t n,
            ofi_memory** ofiOutputs,
            int32_t length,
            int32_t index);

    template <typename T_Callback>
    static int32_t executeBaseOnDspPreparedModel(
            DspPreparedModel* dspPreparedModel,
            const hal::V1_3::Request& request,
            const hal::hidl_vec<hal::hidl_handle>& waitFor,
            hal::V1_2::MeasureTiming measure,
            std::chrono::steady_clock::time_point driverStart,
            const hal::V1_3::OptionalTimePoint& deadline,
            const hal::V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
            const T_Callback& callback);

    template <typename T_Callback>
    static int32_t executeFencedOnDspPreparedModel(
            DspPreparedModel* dspPreparedModel,
            const hal::V1_3::Request& request,
            const hal::hidl_vec<hal::hidl_handle>& waitFor,
            hal::V1_2::MeasureTiming measure,
            std::chrono::steady_clock::time_point driverStart,
            const hal::V1_3::OptionalTimePoint& deadline,
            const hal::V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
            const T_Callback& callback);

 private:
    hal::V1_3::Model mModel;
    uint32_t mModelId;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_DSPPREPAREDMODEL_H_

