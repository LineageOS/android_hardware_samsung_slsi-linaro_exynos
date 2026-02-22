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
 * @file eden_rt_helper_system.cpp
 * @brief Helper functions for eden runtime on system
 */

#include <sys/types.h>          // pid_t
#include <unistd.h>             // getpid
#include <shared_mutex>

#include <vendor/samsung_slsi/hardware/eden_runtime/1.0/IEdenruntime.h>

#include <android/hidl/allocator/1.0/IAllocator.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include "eden_rt_helper.h"

#include "log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EDEN_RT_STUB"

#define ALIGN(a, b) (((a + (b - 1)) / b) * b)

namespace eden {
namespace rt {

using ::android::hardware::hidl_memory;
using ::android::hidl::memory::V1_0::IMemory;
using ::android::hidl::allocator::V1_0::IAllocator;

using namespace vendor::samsung_slsi::hardware::eden_runtime;
using namespace vendor::samsung_slsi::hardware::eden_runtime::V1_0;
using android::sp;

using ::vendor::samsung_slsi::hardware::eden_runtime::V1_0::IEdenruntime;

extern sp<IEdenruntime> service;
extern std::shared_timed_mutex mutex_service;

typedef struct __ModelFromMemorySystemData
{
    EdenModelFromMemoryHidl emfm;
    EdenModelOptionsHidl mo;
} ModelFromMemorySystemData;

/**
 *  @brief Initialize helper functions
 *  @details This function processes an initialization on helper functions.
 *  @returns return code
 */
RtRet InitHelper() {
    LOGD(EDEN_RT_STUB, "InitHelper on system (+)\n");
    return RT_SUCCESS;
}

/**
 *  @brief Deinitialize helper functions
 *  @details This function processes a deinitialization on helper functions.
 *  @returns return code
 */
RtRet DeinitHelper() {
    LOGD(EDEN_RT_STUB, "DeinitHelper on system (+)\n");
    return RT_SUCCESS;
}

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
                                 bool encrypted, const EdenModelOptions& options, void** hidlData) {
    LOGD(EDEN_RT_STUB, "(+)\n");

    ModelFromMemorySystemData* omfmsd = new ModelFromMemorySystemData;
    EdenModelFromMemoryHidl& emfm = omfmsd->emfm;
    EdenModelOptionsHidl& mo = omfmsd->mo;

    emfm.modelTypeInMemory = (ModelTypeInMemoryHidl)modelTypeInMemory;
    emfm.size = size;
    emfm.encrypted = encrypted;

    // Get service for ashmem
    sp<IAllocator> mAllocator = IAllocator::getService("ashmem");
    if (mAllocator == nullptr) {
        LOGE(EDEN_RT_STUB, "Fail to get allocator from ashmem service.\n");
        delete omfmsd;
        return RT_ERROR_ON_GET_SERVICE_ASHMEM;
    }

    // Allocator memory on ashmem
    bool success = false;
    hidl_memory hidlMem;
    const int align = 8;
    int alignedSize = ALIGN(size, align);
    auto hidlStatus = mAllocator->allocate(alignedSize,
        [&success, &hidlMem](bool s, const hidl_memory& m) {
            success = s;
            hidlMem = m;
        });
    if (!hidlStatus.isOk()) {
        LOGE(EDEN_RT_STUB, "Fail on hidl transaction!\n");
        delete omfmsd;
        return RT_ERROR_ON_ALLOCATE_ASHMEM;
    }
    if (!success) {
        LOGE(EDEN_RT_STUB, "Fail on mAllocator->allocate!, (size=%d, alignedSize=%d\n", size, alignedSize);
        delete omfmsd;
        return RT_ERROR_ON_ALLOCATE_ASHMEM;
    }

    // Copy data on addr to hidl_memory allocated by ashmem.
    sp<IMemory> memory = mapMemory(hidlMem);
    void* data = memory->getPointer();
    char* ptr = (char*)data;
    memory->update();
    std::memcpy((void*)ptr, (void*)addr, size);
    memory->commit();

    // emff.mem
    emfm.mem = hidlMem;

    // Prepare model preference
    mo.priority = (ModelPriorityHidl) options.priority;
    mo.modelPreference.nnApiType = (NnApiTypeHidl) options.modelPreference.nnApiType;
    mo.modelPreference.userPreference.hw = (HwPreferenceHidl) options.modelPreference.userPreference.hw;
    mo.modelPreference.userPreference.mode = (ModePreferenceHidl) options.modelPreference.userPreference.mode;

    mo.modelPreference.userPreference.inputBufferMode.enable =
        static_cast<bool>(options.modelPreference.userPreference.inputBufferMode.enable);
    mo.modelPreference.userPreference.inputBufferMode.setInputAsFloat =
        static_cast<bool>(options.modelPreference.userPreference.inputBufferMode.setInputAsFloat);

    mo.latency = static_cast<uint32_t>(options.latency);
    mo.boundCore = static_cast<uint32_t>(options.boundCore);
    mo.presetScenarioId = static_cast<int32_t>(options.presetScenarioId);

    // Return opaque data to caller
    *hidlData = static_cast<void*>(omfmsd);

    return RT_SUCCESS;
}

/**
 *  @brief Call service function for OpenModelFromMemory
 *  @details This function calls service's function for OpenModelFromMemory.
 *           hidlData returned by PrepareOpenModelFromMemory should be delivered.
 *  @param[in] hidlData Opaque data
 *  @param[out] modelId It is representing for constructed EdenModel with a unique id.
 *  @returns return code
 */
RtRet CallOpenModelFromMemory(void* hidlData, uint32_t* modelId) {
    LOGD(EDEN_RT_STUB, "(+)\n");

    ModelFromMemorySystemData* omfmsd = static_cast<ModelFromMemorySystemData*>(hidlData);

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    //// Call openModelFromFd on service
    RtRet rtRetOnStub = RT_SUCCESS;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGE(EDEN_RT_STUB, "service is null (-)\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> hidlRet;
        auto hidlRet = service->openModelFromMemory(stub_pid, omfmsd->emfm, omfmsd->mo,
            [&modelId, &rtRetOnStub](uint32_t rtRetOnService, uint32_t mId) {
                if (rtRetOnService == RT_SUCCESS) {
                    *modelId = mId;
                } else {
                   rtRetOnStub = (RtRet)rtRetOnService;
                }
            });

        if (!hidlRet.isOk()) {
            LOGE(EDEN_RT_STUB, "hidlRet.isOk() is false (-)\n");
            rtRetOnStub = RT_FAILED;
        }
    }

    LOGD(EDEN_RT_STUB, "modelId: %d, rtRetOnStub: %d (-)\n", *modelId, rtRetOnStub);
    return rtRetOnStub;
}

/**
 *  @brief Unprepare resources for OpenModelFromMemory
 *  @details This function releases resources allocated/prepared for OpenModelFromMemory.
 *           hidlData returned by PrepareOpenModelFromMemory should be delivered.
 *  @param[in] hidlData Opaque data
 *  @returns return code
 */
RtRet UnprepareOpenModelFromMemory(void* hidlData) {
    LOGD(EDEN_RT_STUB, "(+)\n");

    ModelFromMemorySystemData* omfmsd = static_cast<ModelFromMemorySystemData*>(hidlData);

    // Release ashmem if needed

    delete omfmsd;

    return RT_SUCCESS;
}

RtRet PrepareGetCompileVersionFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                                        bool encrypted, void** hidlData) {
    ModelFromMemorySystemData* omfmsd = new ModelFromMemorySystemData;
    EdenModelFromMemoryHidl& emfm = omfmsd->emfm;

    emfm.modelTypeInMemory = (ModelTypeInMemoryHidl)modelTypeInMemory;
    emfm.size = size;
    emfm.encrypted = encrypted;

    // Get service for ashmem
    sp<IAllocator> mAllocator = IAllocator::getService("ashmem");
    if (mAllocator == nullptr) {
        LOGE(EDEN_RT_STUB, "Fail to get allocator from ashmem service.\n");
        delete omfmsd;
        return RT_ERROR_ON_GET_SERVICE_ASHMEM;
    }

    // Allocator memory on ashmem
    bool success = false;
    hidl_memory hidlMem;
    const int align = 8;
    int alignedSize = ALIGN(size, align);
    auto hidlStatus = mAllocator->allocate(alignedSize,
        [&success, &hidlMem](bool s, const hidl_memory& m) {
            success = s;
            hidlMem = m;
        });
    if (!hidlStatus.isOk()) {
        LOGE(EDEN_RT_STUB, "Fail on hidl transaction!\n");
        delete omfmsd;
        return RT_ERROR_ON_ALLOCATE_ASHMEM;
    }
    if (!success) {
        LOGE(EDEN_RT_STUB, "Fail on mAllocator->allocate!, (size=%d, alignedSize=%d\n", size, alignedSize);
        delete omfmsd;
        return RT_ERROR_ON_ALLOCATE_ASHMEM;
    }

    // Copy data on addr to hidl_memory allocated by ashmem.
    sp<IMemory> memory = mapMemory(hidlMem);
    void* data = memory->getPointer();
    char* ptr = (char*)data;
    memory->update();
    std::memcpy((void*)ptr, (void*)addr, size);
    memory->commit();

    // emff.mem
    emfm.mem = hidlMem;

    // Return opaque data to caller
    *hidlData = static_cast<void*>(omfmsd);
    return RT_SUCCESS;
}

RtRet CallGetCompileVersionFromMemory(void* hidlData, char versions[][VERSION_LENGTH_MAX]) {
    LOGD(EDEN_RT_STUB, "(+)\n");

    pid_t stub_pid = getpid();  // Get current process id
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);
    if (hidlData == nullptr || versions == nullptr) {
        return RT_PARAM_INVALID;
    }

    ModelFromMemorySystemData* omfmsd = static_cast<ModelFromMemorySystemData*>(hidlData);
    //// Call openModelFromFd on service
    RtRet result = RT_SUCCESS;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGE(EDEN_RT_STUB, "service is null (-)\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> hidlRet;
        auto ret = service->getCompileVersionFromMemory(stub_pid, omfmsd->emfm, [&](uint32_t rtRet, const auto hv) {
            if (rtRet == RT_SUCCESS) {
                int32_t index_count = 0;
                for (const auto var : hv) {
                    std::string each(var);
                    LOGD(EDEN_RT_STUB, "[%d] Information -> %s\n", index_count, each.c_str());
                    snprintf(versions[index_count++], strlen(each.c_str()) + 1, "%s", each.c_str());
                }
            } else {
                result = (RtRet)rtRet;
            }
        });

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "hidlRet.isOk() is false (-)\n");
            return RT_FAILED;
        }
    }
    LOGD(EDEN_RT_STUB, "(-)\n");
    return result;
}

RtRet UnprepareGetCompileVersionFromMemory(void* hidlData) {
    LOGD(EDEN_RT_STUB, "(+)\n");

    ModelFromMemorySystemData* omfmsd = static_cast<ModelFromMemorySystemData*>(hidlData);
    // Release ashmem if needed
    delete omfmsd;

    return RT_SUCCESS;
}

}  // rt
}  // eden

