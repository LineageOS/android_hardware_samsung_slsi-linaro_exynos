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
 * @file    DspPreparedModel.cpp
 * @brief   This is DspPreparedModel class file.
 * @details This header implements IPreparedModel interface.
 */

#include <android/sync.h>
#include <sys/mman.h>  // munmap

#include <iostream>
#include <utility>
#include <thread>
#include <chrono>

#include "log.h"

#include "ExecutionBurstServer.h"  // ExecutionBurstServer
#include "ValidateHal.h"           // validateRequest
#include "Utils.h"                 // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "PrePostProcessor.h"
#include "MyUtils.h"    // DumpToStdio
#include "DspPreparedModel.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::DspPreparedModel"

namespace android {
namespace nn {
namespace eden_driver {

using namespace hal;

using executeSynchronously_cb = V1_2::IPreparedModel::executeSynchronously_cb;
using executeSynchronously_1_3_cb = V1_3::IPreparedModel::executeSynchronously_1_3_cb;
using executeFenced_cb = V1_3::IPreparedModel::executeFenced_cb;

static const Timing kNoTiming = {.timeOnDevice = UINT64_MAX, .timeInDriver = UINT64_MAX};
static const V1_3::OptionalTimePoint kNoDeadline = {};
static const V1_3::OptionalTimeoutDuration kNoLoopTimeoutDuration = {};
static const hidl_vec<hidl_handle> kNoWaitFor = {};
static const int FENCED_TIMEOUT = 5000;  // 5000ms (UD sets polling timeout to 5000ms)

class ExecutionCallback_1_2 : public V1_2::IExecutionCallback {
 public:
    ExecutionCallback_1_2(executeSynchronously_cb cb) : mCb(cb) {
    }

    Return<void> notify(V1_0::ErrorStatus /*status*/) {
        return Void();
    }

    Return<void> notify_1_2(V1_0::ErrorStatus status,
            const hidl_vec<OutputShape>& outputShapes,
            const Timing& timing) {
        mCb(status, outputShapes, timing);
        return Void();
    }

    executeSynchronously_cb mCb;
};

class ExecutionCallback_1_3 : public V1_3::IExecutionCallback {
 public:
    ExecutionCallback_1_3(executeSynchronously_1_3_cb cb) : mCb(cb) {
    }

    Return<void> notify(V1_0::ErrorStatus /*status*/) {
        return Void();
    }

    Return<void> notify_1_2(V1_0::ErrorStatus /*status*/,
            const hidl_vec<OutputShape>& /*outputShapes*/,
            const Timing& /*timing*/) {
        return Void();
    }

    Return<void> notify_1_3(V1_3::ErrorStatus status,
            const hidl_vec<OutputShape>& outputShapes,
            const Timing& timing) {
        mCb(status, outputShapes, timing);
        return Void();
    }

    executeSynchronously_1_3_cb mCb;
};

class DriverFencedExecutionCallback : public V1_3::IFencedExecutionCallback {
 public:
    DriverFencedExecutionCallback(Timing timingSinceLaunch, Timing timingAfterFence,
                                  ErrorStatus error)
        : kTimingSinceLaunch(timingSinceLaunch),
          kTimingAfterFence(timingAfterFence),
          kErrorStatus(error) {}

    Return<void> getExecutionInfo(getExecutionInfo_cb callback) override {
        callback(kErrorStatus, kTimingSinceLaunch, kTimingAfterFence);
        return Void();
    }

 private:
    const Timing kTimingSinceLaunch;
    const Timing kTimingAfterFence;
    const ErrorStatus kErrorStatus;
};

static Return<void> notify(
        const sp<V1_0::IExecutionCallback>& callback_1_0,
        const V1_3::ErrorStatus& status,
        const hidl_vec<OutputShape>& /*outputShapes*/,
        Timing /*timing*/,
        sp<DriverFencedExecutionCallback> /*fencedExecutionCallback*/) {
    return callback_1_0->notify(convertToV1_0(status));
}

static Return<void> notify(
        const sp<V1_2::IExecutionCallback>& callback_1_2,
        const V1_3::ErrorStatus& status,
        const hidl_vec<OutputShape>& outputShapes,
        Timing timing,
        sp<DriverFencedExecutionCallback> /*fencedExecutionCallback*/) {
    return callback_1_2->notify_1_2(convertToV1_0(status), outputShapes, timing);
}

static Return<void> notify(
        const sp<V1_3::IExecutionCallback>& callback_1_3,
        const V1_3::ErrorStatus& status,
        const hidl_vec<OutputShape>& outputShapes,
        Timing timing,
        sp<DriverFencedExecutionCallback> /*fencedExecutionCallback*/) {
    return callback_1_3->notify_1_3(status, outputShapes, timing);
}

static Return<void> notify(
        const executeFenced_cb& callback,
        const V1_3::ErrorStatus& status,
        const hidl_vec<OutputShape>& /*outputShapes*/,
        Timing /*timing*/,
        const sp<DriverFencedExecutionCallback>& fencedExecutionCallback) {
    callback(status, hidl_handle(nullptr), fencedExecutionCallback);
    return Void();
}

static void notify(
        const sp<V1_0::IExecutionCallback>& callback_1_0,
        V1_3::ErrorStatus status) {
    callback_1_0->notify(convertToV1_0(status));
}

static void notify(
        const sp<V1_2::IExecutionCallback>& callback_1_2,
        V1_3::ErrorStatus status) {
    callback_1_2->notify_1_2(convertToV1_0(status), {}, kNoTiming);
}

static void notify(
        const sp<V1_3::IExecutionCallback>& callback_1_3,
        V1_3::ErrorStatus status) {
    callback_1_3->notify_1_3(status, {}, kNoTiming);
}

static uint64_t microsecondsDuration(
        std::chrono::steady_clock::time_point end,
        std::chrono::steady_clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

/**
 * @brief DspPreparedModel constructor
 * @details This function keeps all related information for a given model internally.
 * @returns return code
 */
DspPreparedModel::DspPreparedModel(const V1_3::Model& model) :
    mModel(model) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    mModelId = 0;
    ofi_initialize();
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief DspPreparedModel destructor
 * @details This function releases all related information for a given model internally.
 * @returns
 */
DspPreparedModel::~DspPreparedModel() {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (mModelId != 0) {
        ofi_unload_model(mModelId);
    }
    ofi_deinitialize();
    mModelId = 0;
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

Return<void> DspPreparedModel::setPreparedModel(const char* path) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    mModelId = ofi_load_model(path);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

Return<void> DspPreparedModel::setPreparedModel(char *buffer, int size) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    mModelId = ofi_load_model_from_memory(buffer, size);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

int32_t DspPreparedModel::loadTargetBuffers(
        int32_t n,
        ofi_memory** ofiInputs,
        void* srcAddr,
        int32_t length,
        int32_t index) {
    LOGD(EDEN_DRIVER, "%s(+): n: %d, length: %d, index: %d\n", __func__, n, length, index);

    for (int i = 0; i < n; i++) {
        if (index == 0) {
            int numBuffers = 0;
            ofi_allocateInputs(mModelId, &numBuffers, &ofiInputs[i]);
            LOGD(EDEN_DRIVER, "%s() numBuffers: %d, index = %d, i = %d", __func__, numBuffers, index, i);
            if (numBuffers <= index) {
                LOGE(EDEN_DRIVER, "%s() numBuffers: %d, index = %d", __func__, numBuffers, index);
                return FAIL_TO_GET_INPUT_MEM;
            }
        }
        std::memcpy(ofiInputs[i][index]->va, ((char*)srcAddr + length *i), length);
#if 0
        for (int idx = 0; idx < length; idx++) {
            char c = ((char*)srcAddr + (length * i))[idx];
            LOGD(EDEN_DRIVER, "loadTargetBuffers: [%d] %x\n", idx, c);
        }
#endif
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

int32_t DspPreparedModel::createTargetBuffers(
        int32_t n,
        ofi_memory** ofiOutputs,
        int32_t length,
        int32_t index) {
    LOGD(EDEN_DRIVER, "%s(+): n = %d, length: %d, index: %d\n", __func__, n, length, index);

    for (int i = 0; i < n; i++) {
        if (index == 0) {
            int numBuffers = 0;
            ofi_allocateOutputs(mModelId, &numBuffers, &ofiOutputs[i]);
            if (numBuffers <= index) {
                LOGE(EDEN_DRIVER, "%s() numBuffers: %d, index = %d", __func__, numBuffers, index);
                return FAIL_TO_GET_INPUT_MEM;
            }
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

void DspPreparedModel::getDimensions(const V1_3::Operand& operand, int32_t* dimsOfDsp, bool isNchw) {
    LOGD(EDEN_DRIVER, "%s(+): dims.size() = %zu\n", __func__, operand.dimensions.size());

    int32_t N = 1;
    int32_t C = 1;
    int32_t H = 1;
    int32_t W = 1;
    size_t dims = operand.dimensions.size();

    if (isNchw == true) {
        N = static_cast<int32_t>(operand.dimensions[N_NCHW]); // 0
        C = static_cast<int32_t>(operand.dimensions[C_NCHW]); // 1
        H = static_cast<int32_t>(operand.dimensions[H_NCHW]); // 2
        W = static_cast<int32_t>(operand.dimensions[W_NCHW]); // 3
    } else {
        if (dims == 1) {
            C = static_cast<int32_t>(operand.dimensions[0]);
        } else if (dims == 2) {
            N = static_cast<int32_t>(operand.dimensions[0]);
            C = static_cast<int32_t>(operand.dimensions[1]);
        } else if (dims == 3) {
            C = static_cast<int32_t>(operand.dimensions[0]);
            H = static_cast<int32_t>(operand.dimensions[1]);
            W = static_cast<int32_t>(operand.dimensions[2]);
        } else {
            N = static_cast<int32_t>(operand.dimensions[N_NHWC]);  // 0
            C = static_cast<int32_t>(operand.dimensions[C_NHWC]);  // 3
            H = static_cast<int32_t>(operand.dimensions[H_NHWC]);  // 1
            W = static_cast<int32_t>(operand.dimensions[W_NHWC]);  // 2
        }
    }

    dimsOfDsp[N_NCHW] = N;
    dimsOfDsp[C_NCHW] = C;
    dimsOfDsp[H_NCHW] = H;
    dimsOfDsp[W_NCHW] = W;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

int32_t DspPreparedModel::getNumOfExecution(const V1_3::Request& request) {
    const std::vector<RequestArgument>& inputs = request.inputs;
    const RequestArgument &args = inputs[0];
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (args.hasNoValue) {
        LOGE(EDEN_DRIVER, "%s(-) inputs[0].hasNoValue == true.\n", __func__);;
        return INVALID_PARAMS;
    }

    int32_t poolIndex = args.location.poolIndex;
    const V1_3::Operand& operand = mModel.main.operands[mModel.main.inputIndexes[poolIndex]];
    int32_t dims[4] = {0, 0, 0, 0};
    getDimensions(operand, dims);

    LOGD(EDEN_DRIVER, "%s(-): numOfExecution: %d\n", __func__, dims[0]);

    return dims[0];
}

int32_t DspPreparedModel::loadRequestData(const V1_3::Request& request,
        BufferInfoOnExecute& bufferInfoOnExecute, int numOfExecution,
        ofi_memory*** ofiInputs, ofi_memory*** ofiOutputs) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t ret = RET_OK;
    ofi_memory** tmpInputs = *ofiInputs;
    ofi_memory** tmpOutputs = *ofiOutputs;
    const std::vector<RequestArgument>& inputs = request.inputs;
    for (size_t idx = 0; idx < inputs.size(); idx++) {
        const RequestArgument &args = inputs[idx];
        if (args.hasNoValue) {
            LOGE(EDEN_DRIVER, "%s(-) inputs[%zu].hasNoValue == true.\n", __func__, idx);;
            return INVALID_PARAMS;
        }

        // Get virtual address of input data at #idx
        int32_t poolIndex = args.location.poolIndex;
        int32_t offset = args.location.offset;
        int32_t length = args.location.length;

        LOGD(EDEN_DRIVER, "%s() input poolIndex: %d, offset: %d, length: %d\n",
                __func__, poolIndex, offset, length);
        LOGD(EDEN_DRIVER, "%s() mModel.main.inputIndexes.size() = %zu, mModel.main.inputIndexes[%d] = %d\n",
                __func__, mModel.main.inputIndexes.size(),  poolIndex, mModel.main.inputIndexes[poolIndex]);

        char* virtAddr = nullptr;
        ret = bufferInfoOnExecute.loadHidlMem(request.pools[poolIndex].hidlMemory(), true, virtAddr);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Fail on getVirtualAddressOnPool!\n", __func__);
            return ret;
        }

        ret = loadTargetBuffers(numOfExecution, tmpInputs, (void*)(virtAddr + offset), (length / numOfExecution), idx);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Fail on loadTargetBuffers(n: %d, length: %d, index: %zu)!\n",
                    __func__, numOfExecution, length, idx);
            return ret;
        }
    }

    const std::vector<RequestArgument>& outputs = request.outputs;
    for (size_t idx = 0; idx < outputs.size(); idx++) {
        const RequestArgument &args = outputs[idx];
        if (args.hasNoValue) {
            LOGE(EDEN_DRIVER, "outputs[%zu].hasNoValue == true.\n", idx);;
            return INVALID_PARAMS;
        }

        // Get virtual address of input data at #idx
        int32_t poolIndex = args.location.poolIndex;
        int32_t offset = args.location.offset;
        int32_t length = args.location.length;

        LOGD(EDEN_DRIVER, "%s() output poolIndex: %d, offset: %d, length: %d\n",
                __func__, poolIndex, offset, length);
        LOGD(EDEN_DRIVER, "%s() mModel.main.outputIndexes.size() = %zu, mModel.main.outputIndexes[%d] = %d\n",
                __func__, mModel.main.outputIndexes.size(),  poolIndex, mModel.main.outputIndexes[poolIndex]);

        ret = createTargetBuffers(numOfExecution, tmpOutputs, (length / numOfExecution), idx);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Fail on createTargetBuffers(n: %dlength: %d, index: %zu)!\n",
                    __func__, numOfExecution, length, idx);
            return ret;
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return ret;
}

int32_t DspPreparedModel::updateOutputRequest(const V1_3::Request& request,
        BufferInfoOnExecute& bufferInfoOnExecute, int numOfExecution, ofi_memory** ofiOutputs) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    int32_t ret = RET_OK;

    const std::vector<RequestArgument>& outputs = request.outputs;
    for (size_t idx = 0; idx < outputs.size(); idx++) {
        const RequestArgument &args = outputs[idx];
        if (args.hasNoValue) {
            LOGE(EDEN_DRIVER, "outputs[%zu].hasNoValue == true.\n", idx);;
            return INVALID_PARAMS;
        }

        // Get virtual address of input data at #idx
        int32_t poolIndex = args.location.poolIndex;
        int32_t offset = args.location.offset;
        int32_t length = args.location.length;

        LOGD(EDEN_DRIVER, "%s() output poolIndex: %d, offset: %d, length: %d\n",
                __func__, poolIndex, offset, length);
        LOGD(EDEN_DRIVER, "%s() mModel.main.outputIndexes.size() = %zu, mModel.main.outputIndexes[%d] = %d\n",
                __func__, mModel.main.outputIndexes.size(),  poolIndex, mModel.main.outputIndexes[poolIndex]);
        char* virtAddr = nullptr;
        ret = bufferInfoOnExecute.loadHidlMem(request.pools[poolIndex].hidlMemory(), true, virtAddr);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) Fail on getVirtualAddressOnPool!\n", __func__);
            return ret;
        }

        for (int i = 0; i < numOfExecution; i++) {
            std::memcpy(reinterpret_cast<void*>(virtAddr + offset + ((length / numOfExecution) * i)),
                    reinterpret_cast<void*>((ofiOutputs[i][idx])->va), length / numOfExecution);
        }
#if 0
        for (int i = 0; i < length; i++) {
            char c = ((char *)(virtAddr + offset))[i];
            LOGD(EDEN_DRIVER, "updateOutputReqeust: [%d] %x\n", i, c);
        }
#endif
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return ret;
}

uint32_t DspPreparedModel::getModelId() {
    return mModelId;
}

template <typename T_Callback>
int32_t DspPreparedModel::executeFencedOnDspPreparedModel(DspPreparedModel* dspPreparedModel,
        const V1_3::Request& request,
        const hidl_vec<hidl_handle>& waitFor,
        V1_2::MeasureTiming measure,
        std::chrono::steady_clock::time_point driverStart,
        const V1_3::OptionalTimePoint& /*deadline*/,
        const V1_3::OptionalTimeoutDuration& /*loopTimeoutDuration*/,
        const T_Callback& callback) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    int32_t ret = RET_OK;

    std::chrono::steady_clock::time_point driverStartAfterFence;
    for (const auto& handle : waitFor) {
        if (!handle.getNativeHandle()) {
            LOGE(EDEN_DRIVER, "Can't get sync fence handle\n");
            return FAIL_TO_WAIT_FOR_SYNC_FENCE;
        }
        int syncFenceFd = handle.getNativeHandle()->data[0];

        if (sync_wait(syncFenceFd, FENCED_TIMEOUT) < 0) {
            LOGE(EDEN_DRIVER, "Can't get sync fence handle\n");
            return FAIL_TO_WAIT_FOR_SYNC_FENCE;
        }
    }

    if (measure == V1_2::MeasureTiming::YES) {
        driverStartAfterFence = std::chrono::steady_clock::now();
    }

    BufferInfoOnExecute bufferInfoOnExecute;
    sp<DriverFencedExecutionCallback> fencedExecutionCallback =
        new DriverFencedExecutionCallback(kNoTiming, kNoTiming, V1_3::ErrorStatus::GENERAL_FAILURE);

    std::chrono::steady_clock::time_point deviceStart, deviceEnd, driverEnd;
    int numOfExecution = 0;
    ofi_memory** ofiInputs = nullptr;
    ofi_memory** ofiOutputs = nullptr;

    numOfExecution = dspPreparedModel->getNumOfExecution(request);
    ofiInputs = (ofi_memory**)malloc(sizeof(ofi_memory*) * numOfExecution);
    ofiOutputs = (ofi_memory**)malloc(sizeof(ofi_memory*) * numOfExecution);
    auto freeOfiMemories = [&]() {
        if (ofiInputs != nullptr) {
            for (int i = 0; i < numOfExecution; i++) {
                if (ofiInputs[i] != nullptr) {
                    ofi_freeInputs(dspPreparedModel->getModelId(), ofiInputs[i]);
                }
            }
            free(ofiInputs);
        }
        if (ofiOutputs != nullptr) {
            for (int i = 0; i < numOfExecution; i++) {
                if (ofiOutputs[i] != nullptr) {
                    ofi_freeOutputs(dspPreparedModel->getModelId(), ofiOutputs[i]);
                }
            }
            free(ofiOutputs);
        }
    };
    ret = dspPreparedModel->loadRequestData(request, bufferInfoOnExecute, numOfExecution, &ofiInputs, &ofiOutputs);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) fail to loadRequestData (ret = %d)\n", __func__, ret);
        notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, {}, {}, fencedExecutionCallback);
        freeOfiMemories();
        return FAIL_TO_GET_INPUT_MEM;
    }

    ret = ofi_verify_model(dspPreparedModel->getModelId());
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) fail to verify model. (ret = %d)\n", __func__, ret);
        notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, {}, {}, fencedExecutionCallback);
        freeOfiMemories();
        return FAIL_ON_OFI_VERIFY_MODEL;
    }

    if (measure == V1_2::MeasureTiming::YES) {
        deviceStart = std::chrono::steady_clock::now();
    }

    ofi_model_exe_attr attr[numOfExecution];
    for (int i = 0; i < numOfExecution; i++) {
        LOGD(EDEN_DRIVER, "%s() %d execute trigger", __func__, i);
        memset(&attr[i], 0x00, sizeof(ofi_model_exe_attr));
        attr[i].n_skip = 0;
        attr[i].is_wait = true;
        attr[i].in_buffers = ofiInputs[i];
        attr[i].numOfIn = request.inputs.size();
        attr[i].out_buffers = ofiOutputs[i];
        attr[i].numOfOut = request.outputs.size();
        attr[i].is_async = false;
        attr[i].function_desc.func_p = nullptr;
        attr[i].param.num_param = 0;
        attr[i].result = nullptr;

        ret = ofi_execute_model_attr(dspPreparedModel->getModelId(), &attr[i]);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) fail to execute model. (ret = %d)\n", __func__, ret);
            notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, {}, {}, fencedExecutionCallback);
            freeOfiMemories();
            return FAIL_ON_OFI_EXECUTE_MODEL;
        }
    }

    if (measure == V1_2::MeasureTiming::YES) {
        deviceEnd = std::chrono::steady_clock::now();
    }

    ret = dspPreparedModel->updateOutputRequest(request, bufferInfoOnExecute, numOfExecution, ofiOutputs);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) fail to updateOutputRequest (ret = %d)\n", __func__, ret);
        notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, {}, {}, fencedExecutionCallback);
        freeOfiMemories();
        return FAIL_TO_GET_OUTPUT_MEM;
    }

    hidl_vec<OutputShape> outputShapes;
    if (measure == V1_2::MeasureTiming::YES) {
        driverEnd = std::chrono::steady_clock::now();
        Timing timingSinceLaunch = {
            .timeOnDevice = microsecondsDuration(deviceEnd, deviceStart),
            .timeInDriver = microsecondsDuration(driverEnd, driverStart)
        };
        Timing timingAfterFence = {
            .timeOnDevice = microsecondsDuration(deviceEnd, deviceStart),
            .timeInDriver = microsecondsDuration(driverEnd, driverStartAfterFence)
        };
        fencedExecutionCallback =
            new DriverFencedExecutionCallback(timingSinceLaunch, timingAfterFence, V1_3::ErrorStatus::NONE);
    } else {
        fencedExecutionCallback =
            new DriverFencedExecutionCallback(kNoTiming, kNoTiming, V1_3::ErrorStatus::NONE);
    }

    Return<void> returned = notify(callback, V1_3::ErrorStatus::NONE, outputShapes, {}, fencedExecutionCallback);
    if (!returned.isOk()) {
        LOGE(EDEN_DRIVER, "hidl callback failed to return properly: %s\n", returned.description().c_str());
    }

    freeOfiMemories();

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);

    return ret;
}

template <typename T_Callback>
int32_t DspPreparedModel::executeBaseOnDspPreparedModel(DspPreparedModel* dspPreparedModel,
        const V1_3::Request& request,
        const hidl_vec<hidl_handle>& /*waitFor*/,
        V1_2::MeasureTiming measure,
        std::chrono::steady_clock::time_point driverStart,
        const V1_3::OptionalTimePoint& /*deadline*/,
        const V1_3::OptionalTimeoutDuration& /*loopTimeoutDuration*/,
        const T_Callback& callback) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    int32_t ret = RET_OK;

    BufferInfoOnExecute bufferInfoOnExecute;

    std::chrono::steady_clock::time_point deviceStart, deviceEnd, driverEnd;
    int numOfExecution = 0;
    ofi_memory** ofiInputs = nullptr;
    ofi_memory** ofiOutputs = nullptr;

    numOfExecution = dspPreparedModel->getNumOfExecution(request);
    ofiInputs = (ofi_memory**)malloc(sizeof(ofi_memory*) * numOfExecution);
    ofiOutputs = (ofi_memory**)malloc(sizeof(ofi_memory*) * numOfExecution);
    auto freeOfiMemories = [&]() {
        if (ofiInputs != nullptr) {
            for (int i = 0; i < numOfExecution; i++) {
                if (ofiInputs[i] != nullptr) {
                    ofi_freeInputs(dspPreparedModel->getModelId(), ofiInputs[i]);
                }
            }
            free(ofiInputs);
        }
        if (ofiOutputs != nullptr) {
            for (int i = 0; i < numOfExecution; i++) {
                if (ofiOutputs[i] != nullptr) {
                    ofi_freeOutputs(dspPreparedModel->getModelId(), ofiOutputs[i]);
                }
            }
            free(ofiOutputs);
        }
    };
    ret = dspPreparedModel->loadRequestData(request, bufferInfoOnExecute, numOfExecution, &ofiInputs, &ofiOutputs);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) fail to loadRequestData (ret = %d)\n", __func__, ret);
        notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming, nullptr);
        freeOfiMemories();
        return FAIL_TO_GET_INPUT_MEM;
    }

    ret = ofi_verify_model(dspPreparedModel->getModelId());
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) fail to verify model. (ret = %d)\n", __func__, ret);
        notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming, nullptr);
        freeOfiMemories();
        return FAIL_ON_OFI_VERIFY_MODEL;
    }

    if (measure == V1_2::MeasureTiming::YES) {
        deviceStart = std::chrono::steady_clock::now();
    }

    ofi_model_exe_attr attr[numOfExecution];
    for (int i = 0; i < numOfExecution; i++) {
        LOGD(EDEN_DRIVER, "%s() %d execute trigger", __func__, i);
        memset(&attr[i], 0x00, sizeof(ofi_model_exe_attr));
        attr[i].n_skip = 0;
        attr[i].is_wait = true;
        attr[i].in_buffers = ofiInputs[i];
        attr[i].numOfIn = request.inputs.size();
        attr[i].out_buffers = ofiOutputs[i];
        attr[i].numOfOut = request.outputs.size();
        attr[i].is_async = false;
        attr[i].function_desc.func_p = nullptr;
        attr[i].param.num_param = 0;
        attr[i].result = nullptr;

        ret = ofi_execute_model_attr(dspPreparedModel->getModelId(), &attr[i]);
        if (ret != RET_OK) {
            LOGE(EDEN_DRIVER, "%s(-) fail to execute model. (ret = %d)\n", __func__, ret);
            notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming, nullptr);
            freeOfiMemories();
            return FAIL_ON_OFI_EXECUTE_MODEL;
        }
    }

    if (measure == V1_2::MeasureTiming::YES) {
        deviceEnd = std::chrono::steady_clock::now();
    }

    ret = dspPreparedModel->updateOutputRequest(request, bufferInfoOnExecute, numOfExecution, ofiOutputs);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "%s(-) fail to updateOutputRequest (ret = %d)\n", __func__, ret);
        notify(callback, V1_3::ErrorStatus::GENERAL_FAILURE, {}, kNoTiming, nullptr);
        freeOfiMemories();
        return FAIL_TO_GET_OUTPUT_MEM;
    }

    hidl_vec<OutputShape> outputShapes;
    Timing timing;
    if (measure == V1_2::MeasureTiming::YES) {
        driverEnd = std::chrono::steady_clock::now();
        timing = {
            .timeOnDevice = microsecondsDuration(deviceEnd, deviceStart),
            .timeInDriver = microsecondsDuration(driverEnd, driverStart)
        };
    } else {
        timing = kNoTiming;
    }

    Return<void> returned = notify(callback, V1_3::ErrorStatus::NONE, outputShapes, timing, nullptr);
    if (!returned.isOk()) {
        LOGE(EDEN_DRIVER, "hidl callback failed to return properly: %s\n", returned.description().c_str());
    }

    freeOfiMemories();

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);

    return ret;
}

/**
 * @brief execute a given Request
 * @details This function executes a given Request. Once it is complete, result is notified via callback.
 * @param[in] request Request
 * @param[in] callback callback to be executed
 * @returns return code
 */
Return<V1_0::ErrorStatus> DspPreparedModel::execute(const V1_0::Request& request,
        const sp<V1_0::IExecutionCallback>& callback) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    std::chrono::steady_clock::time_point driverStart;

    if (!validateRequest(convertToV1_3(request), mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-) failed to validate request\n", __func__);
        return V1_0::ErrorStatus::INVALID_ARGUMENT;
    }

    if (!verifyDimension(convertToV1_3(request), mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-)\n failed to varify dimension", __func__);
        return V1_0::ErrorStatus::INVALID_ARGUMENT;
    }

    std::thread async(&DspPreparedModel::executeBaseOnDspPreparedModel<sp<V1_0::IExecutionCallback>>,
            this, convertToV1_3(request), kNoWaitFor, V1_2::MeasureTiming::NO, driverStart,
            kNoDeadline, kNoLoopTimeoutDuration, callback);
    async.detach();

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_0::ErrorStatus::NONE;
}

Return<V1_0::ErrorStatus> DspPreparedModel::execute_1_2(const V1_0::Request& request,
        V1_2::MeasureTiming measure, const sp<V1_2::IExecutionCallback>& callback) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) {
        driverStart = std::chrono::steady_clock::now();
    }

    if (!validateRequest(convertToV1_3(request), mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-) failed to validate request\n", __func__);
        return V1_0::ErrorStatus::INVALID_ARGUMENT;
    }

    if (!verifyDimension(convertToV1_3(request), mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-)\n failed to varify dimension", __func__);
        return V1_0::ErrorStatus::INVALID_ARGUMENT;
    }

    std::thread async(&DspPreparedModel::executeBaseOnDspPreparedModel<sp<V1_2::IExecutionCallback>>,
            this, convertToV1_3(request), kNoWaitFor, measure, driverStart, kNoDeadline,
            kNoLoopTimeoutDuration, callback);
    async.detach();

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_0::ErrorStatus::NONE;
}

Return<V1_3::ErrorStatus> DspPreparedModel::execute_1_3(const V1_3::Request& request,
        V1_2::MeasureTiming measure, const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        const sp<V1_3::IExecutionCallback>& callback) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) {
        driverStart = std::chrono::steady_clock::now();
    }

    if (!validateRequest(request, mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-) failed to validate request\n", __func__);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    if (!verifyDimension(request, mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-)\n failed to varify dimension", __func__);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    std::thread async(&DspPreparedModel::executeBaseOnDspPreparedModel<sp<V1_3::IExecutionCallback>>,
            this, request, kNoWaitFor, measure, driverStart, deadline, loopTimeoutDuration, callback);
    async.detach();

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_3::ErrorStatus::NONE;
}

Return<void> DspPreparedModel::executeSynchronously(const V1_0::Request& request,
        V1_2::MeasureTiming measure, executeSynchronously_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) {
        driverStart = std::chrono::steady_clock::now();
    }

    sp<V1_2::IExecutionCallback> callback = new ExecutionCallback_1_2(cb);

    if (!validateRequest(convertToV1_3(request), mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-)\n", __func__);
        return Void();
    }

    if (!verifyDimension(convertToV1_3(request), mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-)\n", __func__);
        return Void();
    }

    DspPreparedModel::executeBaseOnDspPreparedModel<sp<V1_2::IExecutionCallback>>(
            this, convertToV1_3(request), kNoWaitFor, measure, driverStart, kNoDeadline,
            kNoLoopTimeoutDuration, callback);

    return Void();
}

Return<void> DspPreparedModel::configureExecutionBurst(const sp<V1_2::IBurstCallback>& callback,
        const MQDescriptorSync<V1_2::FmqRequestDatum>& requestChannel,
        const MQDescriptorSync<V1_2::FmqResultDatum>& resultChannel,
        configureExecutionBurst_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    const sp<V1_2::IBurstContext> burst =
        ExecutionBurstServer::create(callback, requestChannel, resultChannel, this);

    if (burst == nullptr) {
        LOGE(EDEN_DRIVER, "burst is null!");
        cb(V1_0::ErrorStatus::GENERAL_FAILURE, {});
    } else {
        cb(V1_0::ErrorStatus::NONE, burst);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

Return<void> DspPreparedModel::executeSynchronously_1_3(const V1_3::Request& request,
        V1_2::MeasureTiming measure,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        executeSynchronously_1_3_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) {
        driverStart = std::chrono::steady_clock::now();
    }

    sp<V1_3::IExecutionCallback> callback = new ExecutionCallback_1_3(cb);

    if (!validateRequest(request, mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-)\n", __func__);
        return Void();
    }

    if (!verifyDimension(request, mModel)) {
        notify(callback, V1_3::ErrorStatus::INVALID_ARGUMENT);
        LOGE(EDEN_DRIVER, "%s(-)\n", __func__);
        return Void();
    }

    DspPreparedModel::executeBaseOnDspPreparedModel<sp<V1_3::IExecutionCallback>>(
            this, request, kNoWaitFor, measure, driverStart, deadline, loopTimeoutDuration,
            callback);

    return Void();
}

Return<void> DspPreparedModel::executeFenced(const V1_3::Request& request,
        const hidl_vec<hidl_handle>& waitFor,
        V1_2::MeasureTiming measure,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        const V1_3::OptionalTimeoutDuration& /*duration*/,
        executeFenced_cb cb) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::chrono::steady_clock::time_point driverStart;
    if (measure == V1_2::MeasureTiming::YES) {
        driverStart = std::chrono::steady_clock::now();
    }

    if (!validateRequest(request, mModel)) {
        cb(V1_3::ErrorStatus::INVALID_ARGUMENT, hidl_handle(nullptr), nullptr);
        LOGE(EDEN_DRIVER, "%s(-)\n", __func__);
        return Void();
    }

    if (!verifyDimension(request, mModel)) {
        cb(V1_3::ErrorStatus::INVALID_ARGUMENT, hidl_handle(nullptr), nullptr);
        LOGE(EDEN_DRIVER, "%s(-)\n", __func__);
        return Void();
    }

    // Wait for the dependent events to signal
    for (const auto& fenceHandle : waitFor) {
        if (!fenceHandle.getNativeHandle()) {
            cb(V1_3::ErrorStatus::INVALID_ARGUMENT, hidl_handle(nullptr), nullptr);
            return Void();
        }
        int syncFenceFd = fenceHandle.getNativeHandle()->data[0];
        if (syncWait(syncFenceFd, -1) != FenceState::SIGNALED) {
            LOGE(EDEN_DRIVER, "%s(-) syncWait failed\n", __func__);
            cb(V1_3::ErrorStatus::GENERAL_FAILURE, hidl_handle(nullptr), nullptr);
            return Void();
        }
    }

    DspPreparedModel::executeFencedOnDspPreparedModel<executeFenced_cb>(
            this, request, waitFor, measure, driverStart, deadline, loopTimeoutDuration, cb);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return Void();
}

template int32_t DspPreparedModel::executeBaseOnDspPreparedModel<sp<V1_0::IExecutionCallback>>(
        DspPreparedModel* dspPreparedModel,
        const V1_3::Request& request,
        const hidl_vec<hidl_handle>& waitFor,
        V1_2::MeasureTiming measure,
        std::chrono::steady_clock::time_point driverStart,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        const sp<V1_0::IExecutionCallback>& callback);
template int32_t DspPreparedModel::executeBaseOnDspPreparedModel<sp<V1_2::IExecutionCallback>>(
        DspPreparedModel* dspPreparedModel,
        const V1_3::Request& request,
        const hidl_vec<hidl_handle>& waitFor,
        V1_2::MeasureTiming measure,
        std::chrono::steady_clock::time_point driverStart,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        const sp<V1_2::IExecutionCallback>& callback);
template int32_t DspPreparedModel::executeBaseOnDspPreparedModel<sp<V1_3::IExecutionCallback>>(
        DspPreparedModel* dspPreparedModel,
        const V1_3::Request& request,
        const hidl_vec<hidl_handle>& waitFor,
        V1_2::MeasureTiming measure,
        std::chrono::steady_clock::time_point driverStart,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        const sp<V1_3::IExecutionCallback>& callback);
template int32_t DspPreparedModel::executeFencedOnDspPreparedModel<executeFenced_cb>(
        DspPreparedModel* dspPreparedModel,
        const V1_3::Request& request,
        const hidl_vec<hidl_handle>& waitFor,
        V1_2::MeasureTiming measure,
        std::chrono::steady_clock::time_point driverStart,
        const V1_3::OptionalTimePoint& deadline,
        const V1_3::OptionalTimeoutDuration& loopTimeoutDuration,
        const executeFenced_cb& callback);
}  // namespace eden_driver
}  // namespace nn
}  // namespace android

