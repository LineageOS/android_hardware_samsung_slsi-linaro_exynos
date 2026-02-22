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
 * @file eden_rt_helper_vendor.cpp
 * @brief Helper functions for eden runtime on vendor
 */

#include <cutils/native_handle.h>
#include <sys/mman.h>           // mmap
#include <errno.h>              // errno

#include <sys/types.h>          // pid_t
#include <unistd.h>             // getpid
#include <shared_mutex>

#include <hardware/exynos/ion.h>  // ion
#include <vendor/samsung_slsi/hardware/eden_runtime/1.0/IEdenruntime.h>

#include "eden_rt_helper.h"

#include "log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EDEN_RT_STUB"

#define ALIGN(a, b) (((a + (b - 1)) / b) * b)

namespace eden {
namespace rt {

using namespace vendor::samsung_slsi::hardware::eden_runtime;
using namespace vendor::samsung_slsi::hardware::eden_runtime::V1_0;
using android::sp;

using ::vendor::samsung_slsi::hardware::eden_runtime::V1_0::IEdenruntime;

typedef struct __IONData
{
    int client;
    int ionFd;
    void* ionAddr;
    int alignedSize;
} IONData;

typedef struct __ModelFromMemoryVendorData
{
    EdenModelFromFdHidl emff;
    IONData ionData;
    EdenModelOptionsHidl mo;
} ModelFromMemoryVendorData;

static int _InitIONBuffer(int& client);
static int _AllocIONBuffer(int client, int size, int align, IONData& ionData);
static int _ReleaseIONBuffer(IONData& ionData);
static int _DeinitIONBuffer(int& client);

extern sp<IEdenruntime> service;
extern std::shared_timed_mutex mutex_service;

static int gIonClient = -1;

/**
 *  @brief Initialize helper functions
 *  @details This function processes an initialization on helper functions.
 *  @returns return code
 */
RtRet InitHelper() {
    LOGD(EDEN_RT_STUB, "InitHelper on vendor (+)\n");
    if (_InitIONBuffer(gIonClient) < 0)
        return RT_FAILED;
    return RT_SUCCESS;
}

/**
 *  @brief Deinitialize helper functions
 *  @details This function processes a deinitialization on helper functions.
 *  @returns return code
 */
RtRet DeinitHelper() {
    LOGD(EDEN_RT_STUB, "DeinitHelper on vendor (+)\n");
    if (_DeinitIONBuffer(gIonClient) < 0)
        return RT_FAILED;
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

    ModelFromMemoryVendorData* omfmvd = new ModelFromMemoryVendorData;
    EdenModelFromFdHidl& emff = omfmvd->emff;
    IONData& ionData = omfmvd->ionData;
    EdenModelOptionsHidl& mo = omfmvd->mo;

    emff.modelTypeInMemory = (ModelTypeInMemoryHidl)modelTypeInMemory;
    emff.encrypted = encrypted;

    // Allocate ION buffer and memcpy from user buffer to ION buffer
    // Now align is 8 to consider encrypted case
    const int align = 8;
    int ret = _AllocIONBuffer(gIonClient, size, align, ionData);
    if (ret < 0) {
        LOGE(EDEN_RT_STUB, "Error on _AllocIONBuffer()\n");
        delete omfmvd;
        return RT_ERROR_ON_ALLOC_ION_BUFFER;
    }

    // memcpy
    std::memcpy(ionData.ionAddr, addr, size);

    // emff.hd
    int32_t num = 1;  // # of fd
    int32_t idx = 0;
    native_handle_t* handle = native_handle_create(num, num);
    handle->data[idx] = ionData.ionFd;
    handle->data[num + idx] = size;
    LOGD(EDEN_RT_STUB, "ionFd: %d, size: %d\n", ionData.ionFd, size);
    emff.hd = handle;

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
    *hidlData = static_cast<void*>(omfmvd);

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

    ModelFromMemoryVendorData* omfmvd = static_cast<ModelFromMemoryVendorData*>(hidlData);

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
        auto hidlRet = service->openModelFromFd(stub_pid, omfmvd->emff, omfmvd->mo,
            [&modelId, &rtRetOnStub](uint32_t rtRetOnService, uint32_t mId) {
                if (rtRetOnService == RT_SUCCESS) {
                    *modelId = mId;
                } else {
                   *modelId = INVALID_MODEL_ID;
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

    ModelFromMemoryVendorData* omfmvd = static_cast<ModelFromMemoryVendorData*>(hidlData);

    // Release ION buffer
    int ret =_ReleaseIONBuffer(omfmvd->ionData);
    if (ret < 0) {
        LOGE(EDEN_RT_STUB, "Error on _ReleaseIONBuffer()\n");
        return RT_ERROR_ON_RELEASE_ION_BUFFER;
    }

    delete omfmvd;

    return RT_SUCCESS;
}

RtRet PrepareGetCompileVersionFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                                        bool encrypted, void** hidlData) {
    LOGD(EDEN_RT_STUB, "(+)\n");
    ModelFromMemoryVendorData* omfmvd = new ModelFromMemoryVendorData;
    EdenModelFromFdHidl& emff = omfmvd->emff;
    IONData& ionData = omfmvd->ionData;

    emff.modelTypeInMemory = (ModelTypeInMemoryHidl)modelTypeInMemory;
    emff.encrypted = encrypted;

    // Allocate ION buffer and memcpy from user buffer to ION buffer
    // Now align is 8 to consider encrypted case
    const int align = 8;
    int ret = _AllocIONBuffer(gIonClient, size, align, ionData);
    if (ret < 0) {
        LOGE(EDEN_RT_STUB, "Error on _AllocIONBuffer()\n");
        delete omfmvd;
        return RT_ERROR_ON_ALLOC_ION_BUFFER;
    }

    // memcpy
    std::memcpy(ionData.ionAddr, addr, size);

    // emff.hd
    int32_t num = 1;  // # of fd
    int32_t idx = 0;
    native_handle_t* handle = native_handle_create(num, num);
    handle->data[idx] = ionData.ionFd;
    handle->data[num + idx] = size;
    LOGD(EDEN_RT_STUB, "ionFd: %d, size: %d\n", ionData.ionFd, size);
    emff.hd = handle;

    // Return opaque data to caller
    *hidlData = static_cast<void*>(omfmvd);
    return RT_SUCCESS;
}

RtRet CallGetCompileVersionFromMemory(void* hidlData, char versions[][VERSION_LENGTH_MAX]) {
    LOGD(EDEN_RT_STUB, "(+)\n");

    pid_t stub_pid = getpid();  // Get current process id
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);
    if (hidlData == nullptr || versions == nullptr) {
        return RT_PARAM_INVALID;
    }

    ModelFromMemoryVendorData* omfmvd = static_cast<ModelFromMemoryVendorData*>(hidlData);
    //// Call openModelFromFd on service
    RtRet result = RT_SUCCESS;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGE(EDEN_RT_STUB, "service is null (-)\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> hidlRet;
        auto ret = service->getCompileVersionFromFd(stub_pid, omfmvd->emff, [&](uint32_t rtRet, const auto hv) {
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
    ModelFromMemoryVendorData* omfmvd = static_cast<ModelFromMemoryVendorData*>(hidlData);
    // Release ION buffer
    int ret = _ReleaseIONBuffer(omfmvd->ionData);
    if (ret < 0) {
        LOGE(EDEN_RT_STUB, "Error on _ReleaseIONBuffer()\n");
        return RT_ERROR_ON_RELEASE_ION_BUFFER;
    }
    delete omfmvd;
    LOGD(EDEN_RT_STUB, "(-)\n");
    return RT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// static functions ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 *  @brief Initialize an ION device and prepare to use ION buffer
 *  @details This function opens an ION device.
 *  @param[in/out] client ION client
 *  @returns 0(Success), -1(Fail)
 */
static int _InitIONBuffer(int& client)
{
    LOGD(EDEN_RT_STUB, "(+)\n");
    // Already opened.
    if (client >= 0) {
        LOGD(EDEN_RT_STUB, "Already opned (%d)\n", client);
        return 0;
    }
    // Open ION device
    client = exynos_ion_open();
    if (client < 0) {
        LOGE(EDEN_RT_STUB, "Error on exynos_ion_open(), (error=%d)\n", errno);
        return -1;
    }
    LOGD(EDEN_RT_STUB, "(-) Now client(%d)\n", client);
    return 0;
}

/**
 *  @brief Deinitialize an ION device
 *  @details This function closes an ION device.
 *  @param[in/out] client ION client
 *  @returns 0(Success), -1(Fail)
 */
static int _DeinitIONBuffer(int& client)
{
    LOGD(EDEN_RT_STUB, "(+)\n");
    LOGD(EDEN_RT_STUB, "Trying to close a client(%d)\n", client);
    if (client < 0) {
        LOGD(EDEN_RT_STUB, "Already closed (%d)\n", client);
        return 0;
    }
    // Close ION device
    int ret = exynos_ion_close(client);
    if (ret < 0) {
        LOGE(EDEN_RT_STUB, "(-) Error on exynos_ion_close(), (errno=%d)\n", errno);
        return -1;
    }
    client = -1;
    LOGD(EDEN_RT_STUB, "(-)\n");
    return 0;
}

/**
 *  @brief Allocate ION buffer for a given size
 *  @details This function allocates ION buffer for a given size and return IONData.
 *  @param[in] size Size of ION buffer
 *  @param[in] align Align size (e.g. 8, 16, 32 etc)
 *  @param[out] ionData IONData which has allocated information.
 *  @returns 0(Success), -1(Fail)
 */
static int _AllocIONBuffer(int client, int size, int align, IONData& ionData)
{
    LOGD(EDEN_RT_STUB, "(+)\n");

    if (client < 0 || size <= 0) {
        LOGE(EDEN_RT_STUB, "Error on arguments, (client=%d, size=%d)\n", client, size);
        return -1;
    }

    LOGD(EDEN_RT_STUB, "Try to allocate ION buffer, (size=%d, align=%d)\n", size, align);

    // Allocate ION buffer
    int alignedSize = ALIGN(size, align);
    int ionFd = exynos_ion_alloc(client, alignedSize, EXYNOS_ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED);
    if (ionFd <= 0) {
        LOGE(EDEN_RT_STUB, "Error on exynos_ion_alloc(), (client=%d, alignedSize=%d, errno=%d)\n",
             client, alignedSize, errno);
        return -1;
    }

    // Get virtual address of ION buffer
    void* ionAddr = mmap(NULL, alignedSize, PROT_READ | PROT_WRITE, MAP_SHARED, ionFd, 0);
    if (ionAddr == MAP_FAILED) {
        LOGE(EDEN_RT_STUB, "Error on mmap(), (alignedSize=%d, ionFd=%d, errno=%d)\n",
             alignedSize, ionFd, errno);
        return -1;
    }

    // Now allocate IONData and set it to caller
    ionData.client = client;
    ionData.ionFd = ionFd;
    ionData.ionAddr = ionAddr;
    ionData.alignedSize = alignedSize;

    return 0;
}

/**
 *  @brief Release ION buffer for a given IONData
 *  @details This function releases ION buffer for a given IONData.
 *  @param[in] ionData IONData which is returned by _AllocIONBuffer.
 *  @returns 0(Success), -1(Fail)
 */
static int _ReleaseIONBuffer(IONData& ionData)
{
    LOGD(EDEN_RT_STUB, "(+)\n");

    LOGD(EDEN_RT_STUB, "Try to release ION buffer, (client=%d, ionFd=%d, ionAddr=%p, alignedSize=%d)\n",
         ionData.client, ionData.ionFd, ionData.ionAddr, ionData.alignedSize);

    // Unmap virtual address of ION buffer
    int ret = munmap(ionData.ionAddr, ionData.alignedSize);
    if (ret < 0) {
        LOGE(EDEN_RT_STUB, "Error on munmap(), (errno=%d)\n", errno);
        return -1;
    }

    // (Optional??) Close ionFd
    ret = close(ionData.ionFd);
    if (ret < 0) {
        LOGE(EDEN_RT_STUB, "Error on close(), (errno=%d)\n", errno);
        return -1;
    }

    return 0;
}

}  // rt
}  // eden

