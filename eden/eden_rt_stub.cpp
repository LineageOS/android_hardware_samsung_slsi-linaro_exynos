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
 * @file eden_rt_stub.cpp
 * @brief implementation of eden_rt_stub api
 */

#include "eden_rt_helper.h"
#include "EdenRuntime.h"
#include "log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EDEN_RT_STUB"

#include <cutils/native_handle.h>

#include <hidl/LegacySupport.h>
#include <hidl/Status.h>

#include <chrono>
#include <map>
#include <sys/mman.h>           // mmap
#include <errno.h>              // errno

#include <sys/types.h>          // pid_t
#include <unistd.h>             // getpid
#include <stdlib.h>             // atexit
#include <mutex>
#include <shared_mutex>

#include <vendor/samsung_slsi/hardware/eden_runtime/1.0/IEdenruntime.h>

#define CLRHIGH(u64)    ((u64) & 0x00000000FFFFFFFF)
#define SIZE_4KB        0x1000
namespace eden {
namespace rt {

typedef struct __EmaBuffer {
    uint32_t fd;
    void *addr;
    int32_t size;
    int32_t numOfBuffers;
} EmaBuffer;

using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::Void;

using namespace std::chrono_literals;
using namespace vendor::samsung_slsi::hardware::eden_runtime;
using namespace vendor::samsung_slsi::hardware::eden_runtime::V1_0;
using android::sp;

using ::vendor::samsung_slsi::hardware::eden_runtime::V1_0::IEdenruntime;
using ::android::hidl::base::V1_0::IBase;

class Callback;
struct StubDeathRecipient;

sp<Callback> eCallback;
sp<IEdenruntime> service;
sp<StubDeathRecipient> stubDeathRecipient;
std::mutex mutex_mMapClientbufServerbuf;
std::shared_timed_mutex mutex_service;

//          <client buf addr, <server buf addr, gtest EdenBuffer addr>>
std::map<uint64_t, std::pair<uint64_t, EdenBuffer **>> mMapClientbufServerbuf;

class Callback : public IEdenruntimeCallback {
    public:
    Callback(NotifyFuncPtr nfp) {
        LOGD(EDEN_RT_STUB, "Callback constructor is called!!\n");
        notify = nfp;
    }

    virtual ~Callback() {
        LOGD(EDEN_RT_STUB, "Callback desstructor is called!!\n");
        // Get current process id
        pid_t stub_pid = getpid();
        // Print out pid
        LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);
    }

    android::hardware::Return<uint32_t> executeCallback(uint64_t cbAddr,
                                                        uint64_t value,
                                                        const hidl_handle& hd) override {
        LOGD(EDEN_RT_STUB, "(+)\n");
        if (!value) {
            LOGD(EDEN_RT_STUB, "%s: value is null\n", __func__);
            return RT_FAILED;
        }

        EdenRequest* req = (EdenRequest*)value;
        mutex_mMapClientbufServerbuf.lock();

        if (!req->outputBuffers) {
            LOGE(EDEN_RT_STUB, "req->outputBuffer is nullptr");
            mutex_mMapClientbufServerbuf.unlock();
            return RT_FAILED;
        }

        auto it = mMapClientbufServerbuf.find((uint64_t)req->outputBuffers);
        if (it == mMapClientbufServerbuf.end()) {
            mutex_mMapClientbufServerbuf.unlock();
            LOGE(EDEN_RT_STUB, "failed to find ServerOutputBuffers according to buffers");
            return RT_FAILED;
        }
        mutex_mMapClientbufServerbuf.unlock();

        const native_handle_t* handle = hd.getNativeHandle();
        int num = handle->numFds;
        EdenBuffer** buffers = it->second.second;
        bool isBinding = handle->data[num] & (1 << 31);

        if (!isBinding) {
            for (int idx = 0; idx < num; idx++) {
                EdenBuffer* buffer = &((*buffers)[idx]);
                uint32_t fd = handle->data[idx];
                LOGD(EDEN_RT_STUB, "callback unmap before size: %d", buffer->size);
                munmap((void *)buffer->addr, buffer->size);
                buffer->size = handle->data[num + idx];
                buffer->addr = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (buffer->addr == (void *)-1) {
                    LOGE(EDEN_RT_STUB, "mmap failed. errno: %d", errno);
                    return RT_FAILED;
                }
                LOGD(EDEN_RT_STUB, "callback fd: %d, size: %d\n", fd, buffer->size);
                LOGD(EDEN_RT_STUB, "callback mmap addr: %p\n", buffer->addr);
            }
        }
        notify((addr_t*)cbAddr, value);
        LOGD(EDEN_RT_STUB, "(-)\n");
        return RT_SUCCESS;
    }

    android::hardware::Return<uint32_t> isAlive() {
        return RT_SUCCESS;
    }

    void setNotifyFunc(NotifyFuncPtr nfp) {
        LOGD(EDEN_RT_STUB, "setNotifyFunc is called!!\n");
        notify = nfp;
    }

    private:
        NotifyFuncPtr notify;
};

void ClearGlobalValue(void) {
    service = nullptr;
    eCallback = nullptr;
    stubDeathRecipient = nullptr;
}

struct StubDeathRecipient : public android::hardware::hidl_death_recipient {
    StubDeathRecipient() = default;
    virtual void serviceDied(uint64_t /*cookie*/, const android::wp<IBase>& who) {
        if (service != nullptr && android::hardware::interfacesEqual(service, who.promote())) {
            // Get current process id
            pid_t stub_pid = getpid();
            // Print out pid
            LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

            std::lock_guard<std::shared_timed_mutex> wlock(mutex_service);

            LOGE(EDEN_RT_STUB, "eden runtime service died");
            ClearGlobalValue();

            uint32_t count = 0;
            while (service == nullptr) {
                service = IEdenruntime::getService();
                count++;
                if (count > 0xffff) {
                    LOGE(EDEN_RT_STUB, "time-out to get a new eden service after death");
                    break;
                }
            }
            eCallback = new Callback(nullptr);

            // android::hardware::Return<uint32_t> ret;
            auto ret = service->init(stub_pid, eCallback);
            if (!ret.isOk()) {
                LOGE(EDEN_RT_STUB, "Error!! ret.isOk() is false (-)\n");
                ClearGlobalValue();
                return;
            } else {
                uint32_t result = static_cast<uint32_t>(ret);
                if (!result) {
                    LOGD(EDEN_RT_STUB, "service->init success\n");
                } else {
                    LOGE(EDEN_RT_STUB, "service->init: %d\n", result);
                    ClearGlobalValue();
                    return;
                }
            }

            stubDeathRecipient = new StubDeathRecipient();
            bool linkRet = service->linkToDeath(stubDeathRecipient, 0);
            if (!linkRet) {
                LOGE(EDEN_RT_STUB, "Could not link to death\n");
            }

            LOGI(EDEN_RT_STUB, "success to get a new eden service after death");
        } else {
            LOGE(EDEN_RT_STUB, "unknown service died");
        }
    }
};

RtRet getSocType(int32_t& socType) {
    uint32_t modelId = 0;
    int32_t versions[VERSION_MAX];
    RtRet ret = GetEdenVersion(modelId, versions);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_RT_STUB, "GetEdenVersion failed [%d]\n", ret);
        return ret;
    }

    socType = versions[PLATFORM_VERSION_SOC];
    return ret;
}

/**
 *  @brief Init EDEN Runtime
 *  @details This API function initializes the CPU/GPU/NPU handler.
 *  @param void
 *  @returns return code
 */
RtRet Init(void) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_service);

        // Open ION device to support OpenModelFromMemory
        if (InitHelper() != RT_SUCCESS) {
            LOGE(EDEN_RT_STUB, "Fail on InitHelper()!\n");
            ClearGlobalValue();
            return RT_ERROR_ON_INIT_HELPER;
        }

        if (service != nullptr) {
            LOGI(EDEN_RT_STUB, "(-) already getService\n");
            return RT_SUCCESS;
        } else {
            service = IEdenruntime::getService();
            if (service == nullptr) {
                LOGE(EDEN_RT_STUB, "service is nullptr\n");
                return RT_SERVICE_NOT_AVAILABLE;
            }
        }

        eCallback = new Callback(nullptr);

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->init(stub_pid, eCallback);
        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "Error!! ret.isOk() is false (-)\n");
            ClearGlobalValue();
            return RT_FAILED;
        } else {
            uint32_t result = static_cast<uint32_t>(ret);
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->init success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->init: %d\n", result);
                ClearGlobalValue();
                return RT_INIT_FAILED;
            }
        }

        stubDeathRecipient = new StubDeathRecipient();
        bool linkRet = service->linkToDeath(stubDeathRecipient, 0);
        if (!linkRet) {
            LOGE(EDEN_RT_STUB, "Could not link to death\n");
        }

    }

    LOGI(EDEN_RT_STUB, "(-)\n");
    return RT_SUCCESS;
}

RtRet Init(uint32_t target) {
    LOGD(EDEN_RT_STUB, "Not support init with sepecific target device in stub layer\n");
    return Init();
}

/**
 *  @brief Open a model file and generates an in-memory model structure
 *  @details This function reads a model file and construct an in-memory model structure.
 *           The model file should be one of the supported model file format.
 *           Once it successes to parse a given model file,
 *           unique model id is returned via modelId.
 *  @param[in] modelFile It is representing for EDEN model file such as file path.
 *  @param[out] modelId It is representing for constructed EdenModel with a unique id.
 *  @param[in] preference It is representing for a model preference.
 *  @returns return code
 */
RtRet OpenModelFromFile(EdenModelFile* modelFile, uint32_t* modelId, ModelPreference preference) {
    LOGD(EDEN_RT_STUB, "Deprecated function\n");
    EdenModelOptions options;
    options.modelPreference = preference;
    options.priority = ModelPriority::P_DEFAULT;
    options.boundCore = NPU_UNBOUND;
    options.latency = 0;
    options.presetScenarioId = PRESET_DISABLE_UPPER_BOUND;
    options.reserved[0] = {0};
    return OpenModelFromFile(modelFile, modelId, options);
}

RtRet OpenModelFromFile(EdenModelFile* modelFile, uint32_t* modelId, const EdenModelOptions& options) {
    LOGI(EDEN_RT_STUB, "(+) with modelFile: %s\n", modelFile->pathToModelFile);

    EdenModelHidl emh;
    int is_found_path = 0;

    emh.modelFileFormat = (EdenModelFileHidl)modelFile->modelFileFormat;

    if (modelFile->pathToModelFile != NULL) {
        emh.pathToModelFile =
            hidl_string((char*)modelFile->pathToModelFile, modelFile->lengthOfPath);
        emh.lengthOfPath = modelFile->lengthOfPath;
        is_found_path = 1;
    } else {
        emh.lengthOfPath = 0;
    }

    if (modelFile->pathToWeightBiasFile != NULL) {
        emh.pathToWeightBiasFile =
            hidl_string((char*)modelFile->pathToWeightBiasFile,
                                                modelFile->lengthOfWeightBias);
        emh.lengthOfWeightBias = modelFile->lengthOfWeightBias;
        is_found_path = 1;
    } else {
        emh.lengthOfWeightBias = 0;
    }

    if (is_found_path == 0) {
        LOGD(EDEN_RT_STUB, "not found path\n");
        return RT_PARAM_INVALID;
    }

    EdenModelOptionsHidl mo;
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

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    RtRet result = RT_SUCCESS;

    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGE(EDEN_RT_STUB, "service is null (-)\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret =
            service->openModel(stub_pid, emh, mo, [&modelId, &result](uint32_t rtRet, uint32_t mId) {
                if (rtRet == RT_SUCCESS) {
                    *modelId = mId;
                } else {
                    result = (RtRet)rtRet;
                }
            });

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        }
    }

    LOGI(EDEN_RT_STUB, "(-) modelId: %d, result: %d (-)\n", *modelId, result);
    return result;
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
RtRet OpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                          bool encrypted, uint32_t* modelId, ModelPreference preference) {
    LOGD(EDEN_RT_STUB, "Deprecated function\n");
    EdenModelOptions options;
    options.modelPreference = preference;
    options.priority = ModelPriority::P_DEFAULT;
    options.boundCore = NPU_UNBOUND;
    options.latency = 0;
    options.presetScenarioId = PRESET_DISABLE_UPPER_BOUND;
    options.reserved[0] = {0};
    return OpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, modelId, options);
}

RtRet OpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size, bool encrypted,
                          uint32_t* modelId, const EdenModelOptions& options) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    RtRet result = RT_SUCCESS;
    void* hidlData;

    do {
        // Prepare HIDL data to be delivered
        RtRet rtRet = PrepareOpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, options, &hidlData);
        if (rtRet != RT_SUCCESS) {
            LOGE(EDEN_RT_STUB, "Error on PrepareOpenModelFromMemory! (rtRet=%d) (modelTypeInMemory=%d, addr=%p, size=%d, encrypted=%d\n",
                 rtRet, modelTypeInMemory, addr, size, encrypted);
            result = RT_ERROR_ON_PREPARE_OPEN_MODEL_FROM_MEMORY;
            // Escape routine since there's no way to going further.
            break;
        }

        // Send HIDL data to server
        rtRet = CallOpenModelFromMemory(hidlData, modelId);
        if (rtRet != RT_SUCCESS) {
            LOGE(EDEN_RT_STUB, "Error on CallOpenModelFromMemory! (rtRet=%d) (modelTypeInMemory=%d, addr=%p, size=%d, encrypted=%d\n",
                 rtRet, modelTypeInMemory, addr, size, encrypted);
            result = RT_ERROR_ON_CALL_OPEN_MODEL_FROM_MEMORY;
            // Error occured, but it needs to go below to release allocated memory on hidlData.
            // And this release would be taken place at UnprepareOpenModelFromMemory(..)
        }

        // Unprepare
        rtRet = UnprepareOpenModelFromMemory(hidlData);
        if (rtRet != RT_SUCCESS) {
            LOGE(EDEN_RT_STUB, "Error on PrepareOpenModelFromMemory! (rtRet=%d) (modelTypeInMemory=%d, addr=%p, size=%d, encrypted=%d\n",
                 rtRet, modelTypeInMemory, addr, size, encrypted);
            // No override result when it already keeps an error code.
            if (result == RT_SUCCESS) {
                result = RT_ERROR_ON_UNPREPARE_OPEN_MODEL_FROM_MEMORY;
            }
        }
    } while (0);

    return result;
}

/**
 *  @brief Allocate a buffer for input to execute a model
 *  @details This function allocates an efficient buffer to execute a model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[out] buffers Array of EdenBuffers for input
 *  @param[out] numOfBuffers # of buffers
 *  @returns return code
 */
RtRet AllocateInputBuffers(uint32_t modelId,
                           EdenBuffer** buffers,
                           int32_t* numOfBuffers) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    RtRet result = RT_SUCCESS;

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    EdenBuffer* newBuffer = nullptr;
    uint64_t serverBuf = 0;
    EdenBuffer** tmpbuffers = buffers;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->allocateInputBuffer(stub_pid, modelId, [&](uint32_t rtRet, const auto &hd) {
            if (rtRet == RT_SUCCESS) {
                const native_handle_t* handle = hd.getNativeHandle();
                int num = *numOfBuffers = handle->numFds;
                newBuffer = new EdenBuffer[num];

                for (int idx = 0; idx < num; idx++) {
                    EdenBuffer* buffer = &(newBuffer[idx]);
                    uint32_t fd = handle->data[idx];
                    buffer->size = handle->data[num + idx];
                    buffer->addr = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                    LOGD(EDEN_RT_STUB, "alloc inputbuf fd: %d, size: %d\n", fd, buffer->size);
                }
                serverBuf = (uint64_t)((uint64_t)handle->data[num * 2] << 32 | CLRHIGH((uint64_t)handle->data[num * 2 + 1]));
                *buffers = newBuffer;
            } else {
                result = (RtRet)rtRet;
            }
        });

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->allocateInputBuffer success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->allocateInputBuffer: %d\n", result);
            }
        }
    }
    mutex_mMapClientbufServerbuf.lock();
    mMapClientbufServerbuf.insert(std::make_pair((uint64_t)newBuffer, std::make_pair(serverBuf, tmpbuffers)));
    mutex_mMapClientbufServerbuf.unlock();

    LOGI(EDEN_RT_STUB, "(-)\n");
    return result;
}

/**
 *  @brief Allocate a buffer for output to execute a model
 *  @details This function allocates an efficient buffer to execute a model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[out] buffers Array of EdenBuffers for input
 *  @param[out] numOfBuffers # of buffers
 *  @returns return code
 */
RtRet AllocateOutputBuffers(uint32_t modelId,
                            EdenBuffer** buffers,
                            int32_t* numOfBuffers) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    RtRet result = RT_SUCCESS;

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    EdenBuffer* newBuffer = nullptr;
    uint64_t serverBuf = 0;
    EdenBuffer** tmpbuffers = buffers;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->allocateOutputBuffer(stub_pid, modelId, [&](uint32_t rtRet, const auto &hd) {
            if (rtRet == RT_SUCCESS) {
                const native_handle_t* handle = hd.getNativeHandle();
                int num = *numOfBuffers = handle->numFds;
                newBuffer = new EdenBuffer[num];

                int32_t totalSize = 0;
                bool isBinding = handle->data[num] & (1 << 31) ? true : false;

                if (isBinding) {
                    static int32_t soc = 0;
                    if (!soc) getSocType(soc);
                    int32_t align = SIZE_4KB;
                    if (soc == 9925) {
                        align = 1;
                    }
                    for (int idx = 0; idx < num; idx++) {
                        int32_t size = (handle->data[num+idx]) & (~(1 << 31));
                        totalSize += size % align\
                                    ? (((size / align) + 1) * align)\
                                    : (size);
                    }
                    uint32_t fd = handle->data[0];
                    char* addr = reinterpret_cast<char*>(mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                                                                                                            fd, 0));

                    for (int idx = 0; idx < num; idx++) {
                        EdenBuffer* buffer = &(newBuffer[idx]);
                        buffer->addr = addr;
                        buffer->size = handle->data[num + idx] & (~(1 << 31));
                        addr += ((buffer->size) % align\
                                ? (((buffer->size / align) + 1) * align)\
                                : buffer->size);
                        LOGD(EDEN_RT_STUB, "alloc outbuf idx: %d, addr : %p, size: %d, fd : %d \n", idx, buffer->addr, buffer->size, fd);
                    }
                } else {
                    for (int idx = 0; idx < num; idx++) {
                        EdenBuffer* buffer = &(newBuffer[idx]);
                        uint32_t fd = handle->data[idx];
                        buffer->size = handle->data[num + idx];
                        buffer->addr = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                        LOGD(EDEN_RT_STUB, "alloc outbuf fd: %d, size: %d\n", fd, buffer->size);
                        LOGD(EDEN_RT_STUB, "out allc mmap addr: %p\n", buffer->addr);
                    }
                }

                serverBuf = (uint64_t)((uint64_t)handle->data[num * 2] << 32 | CLRHIGH((uint64_t)handle->data[num * 2 + 1]));
                *buffers = newBuffer;
                LOGD(EDEN_RT_STUB, "client buf: %p, gtest buf: %p\n", newBuffer, buffers);
            } else {
                result = (RtRet)rtRet;
            }
        });

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->allocateOutputBuffer success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->allocateOutputBuffer: %d\n", result);
            }
        }
    }
    mutex_mMapClientbufServerbuf.lock();
    mMapClientbufServerbuf.insert(std::make_pair((uint64_t)newBuffer, std::make_pair(serverBuf, tmpbuffers)));
    mutex_mMapClientbufServerbuf.unlock();

    LOGI(EDEN_RT_STUB, "(-)\n");
    return (RtRet)result;
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
RtRet LoadInputBuffers(uint32_t modelId, UserBuffer* userBuffers, int32_t numOfBuffers, EdenBuffer** edenBuffers) {
    LOGD(EDEN_RT, "(+)\n");
    LOGI(EDEN_RT, "Not supported yet\n");
    return RT_FAILED;
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
RtRet LoadOutputBuffers(uint32_t modelId, UserBuffer* userBuffers, int32_t numOfBuffers, EdenBuffer** edenBuffers) {
    LOGD(EDEN_RT, "(+)\n");
    LOGI(EDEN_RT, "Not supported yet\n");
    return RT_FAILED;
}

/**
 *  @brief Execute EDEN Req
 *  @details This API function executes EdenRequest with preference.
 *  @param[in] req It consists of EDEN Model ID, input/output buffers and callback.
 *  @param[in] evt Callback function defined by User.
 *  @param[in] preference it determines how to run EdenModel with preference.
 *  @returns return code
 */
RtRet ExecuteReq(EdenRequest* req, EdenEvent** evt, RequestPreference preference) {
    LOGD(EDEN_RT_STUB, "Deprecated function\n");
    EdenRequestOptions options;
    options.userPreference = preference.userPreference;
    options.requestMode = RequestMode::NONE;
    options.reserved[0] = {0, };
    return ExecuteReq(req, evt, options);
}

RtRet ExecuteReq(EdenRequest* req, EdenEvent** evt, const EdenRequestOptions& options) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    EdenRequestHidl erh;
    erh.modelId = req->modelId;

    mutex_mMapClientbufServerbuf.lock();
    auto it = mMapClientbufServerbuf.find((uint64_t)req->inputBuffers);
    if (it == mMapClientbufServerbuf.end()) {
        LOGE(EDEN_RT_STUB, "failed to find ServerInputBuffers according to buffers (-)");
        mutex_mMapClientbufServerbuf.unlock();
        return RT_FAILED;
    }
    erh.inputAddr = it->second.first;

    it = mMapClientbufServerbuf.find((uint64_t)req->outputBuffers);
    if (it == mMapClientbufServerbuf.end()) {
        LOGE(EDEN_RT_STUB, "failed to find ServerOutputBuffers according to buffers (-)");
        mutex_mMapClientbufServerbuf.unlock();
        return RT_FAILED;
    }
    erh.outputAddr = it->second.first;
    mutex_mMapClientbufServerbuf.unlock();

    erh.cValue.pollingAddr = (uint64_t)&req->callback->requestId;
    erh.cValue.value = (uint64_t)req;

    EdenRequestOptionsHidl ro;
    ro.userPreference.hw = (HwPreferenceHidl) options.userPreference.hw;
    ro.userPreference.mode = (ModePreferenceHidl) options.userPreference.mode;

    ro.userPreference.inputBufferMode.enable =
        static_cast<bool>(options.userPreference.inputBufferMode.enable);
    ro.userPreference.inputBufferMode.setInputAsFloat =
        static_cast<bool>(options.userPreference.inputBufferMode.setInputAsFloat);

    ro.requestMode = (RequestModeHidl) options.requestMode;

    LOGD(EDEN_RT_STUB, "ro.userPreference.hw : %d\n", ro.userPreference.hw);
    LOGD(EDEN_RT_STUB, "ro.userPreference.hw : %d\n", ro.userPreference.mode);
    LOGD(EDEN_RT_STUB, "ro.userPreference.inputBufferMode.enable : %d\n", ro.userPreference.inputBufferMode.enable);
    LOGD(EDEN_RT_STUB, "ro.userPreference.inputBufferMode.setInputAsFloat : %d\n", ro.userPreference.inputBufferMode.setInputAsFloat);
    LOGD(EDEN_RT_STUB, "ro.requestMode : %d\n", ro.requestMode);

    sp<Callback> callback = new Callback(req->callback->notify);
    // eCallback->setNotifyFunc(req->callback->notify);

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    uint32_t result;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGE(EDEN_RT_STUB, "service is nullptr (-)\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->executeReq(stub_pid, erh, callback, ro);

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            result = static_cast<uint32_t>(ret);
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->executeReq success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->executeReq: %d\n", result);
            }
        }
    }

    LOGD(EDEN_RT_STUB, "(-)\n");
    return (RtRet)result;
}

/**
 *  @brief Release a buffer allocated by Eden framework
 *  @details This function releases a buffer returned by AllocateXXXBuffers.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] buffers Buffer pointer allocated by AllocateXXXBuffers
 *  @returns return code
 */
RtRet FreeBuffers(uint32_t modelId, EdenBuffer* buffers) {
    LOGI(EDEN_RT_STUB, "(+) modelId : %d\n", modelId);

    if ((modelId == INVALID_MODEL_ID) || (buffers == nullptr)) {
        LOGE(EDEN_RT_STUB, "(-) Given modelId or buffers is invalid! modelId=[%d], buffers=[%p]\n", modelId, buffers);
        return RT_PARAM_INVALID;
    }

    mutex_mMapClientbufServerbuf.lock();
    auto it = mMapClientbufServerbuf.find((uint64_t)buffers);
    if (it == mMapClientbufServerbuf.end()) {
        LOGE(EDEN_RT_STUB, "failed to find handle according to buffers");
        mutex_mMapClientbufServerbuf.unlock();
        return RT_FAILED;
    }
    mutex_mMapClientbufServerbuf.unlock();

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    int32_t num = 0;
    RtRet result = RT_SUCCESS;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->freeBuffer(stub_pid, modelId, it->second.first,
                                [&result, &num](uint32_t rtRet, int32_t numOfBuffers) {
            if (rtRet == RT_SUCCESS) {
                num = numOfBuffers;
            } else {
                result = (RtRet)rtRet;
            }
        });

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->freeBuffer success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->freeBuffer: %d\n", result);
            }
        }
    }

    for (int idx = 0; idx < num; idx++) {
        EdenBuffer* buffer = &(buffers[idx]);
        LOGD(EDEN_RT_STUB, "free buffer unmap size: %d", buffer->size);
        munmap((void *)buffer->addr, buffer->size);
    }

    mutex_mMapClientbufServerbuf.lock();
    mMapClientbufServerbuf.erase((uint64_t)buffers);
    mutex_mMapClientbufServerbuf.unlock();
    delete[] buffers;

    LOGI(EDEN_RT_STUB, "(-)\n");
    return (RtRet)result;
}

/**
 *  @brief Close EDEN Model
 *  @details This API function releases resources related with the EDEN Model.
 *  @param[in] modelId It is a unique id for EDEN Model.
 *  @returns return code
 */
RtRet CloseModel(uint32_t modelId) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    uint32_t result;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null (-)\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->closeModel(stub_pid, modelId);

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            result = static_cast<uint32_t>(ret);
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->closeModel success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->closeModel: %d\n", result);
            }
        }
    }

    LOGI(EDEN_RT_STUB, "(-)\n");
    return (RtRet)result;
}

/**
 *  @brief Shutdown EDEN Runtime
 *  @details This API function close all EDEN Models with related resources for shutdown EDEN Framework.
 *  @param void
 *  @returns return code
 */
RtRet Shutdown(void) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    uint32_t result;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null\n");
            return RT_SERVICE_NOT_AVAILABLE;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->shutdown(stub_pid);
        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            result = RT_FAILED;
        } else {
            result = static_cast<uint32_t>(ret);
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->shutdown success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->shutdown: %d\n", result);
            }
        }

        // Close ION device
        if (DeinitHelper() != RT_SUCCESS) {
            LOGE(EDEN_RT_STUB, "Fail on DeinitHelper()!\n");
            result = RT_ERROR_ON_DEINIT_HELPER;
        }
    }

    LOGI(EDEN_RT_STUB, "(-)\n");
    return (RtRet)result;
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
RtRet GetInputBufferShape(uint32_t modelId, int32_t inputIndex, int32_t* width, int32_t* height, int32_t* channel, int32_t* number) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    RtRet result = RT_SUCCESS;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null (-)\n");
            return RT_FAILED;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->getInputBufferShape(stub_pid, modelId, inputIndex,
            [&](uint32_t rtRet, int32_t inputWidth, int32_t inputHeight,
            int32_t inputChannel, int32_t inputNumber) {
                if (rtRet == RT_SUCCESS) {
                    *width = inputWidth;
                    *height = inputHeight;
                    *channel = inputChannel;
                    *number = inputNumber;
                } else {
                    *width = 0;
                    *height = 0;
                    *channel = 0;
                    *number = 0;
                    result = (RtRet)rtRet;
                }
            }
        );

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            if (result == RT_SUCCESS) {
                LOGD(EDEN_RT_STUB, "service->getInputBufferShape success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->getInputBufferShape: %d\n", result);
            }
        }
    }

    LOGI(EDEN_RT_STUB, "(-)\n");
    return (RtRet)result;
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
RtRet GetOutputBufferShape(uint32_t modelId, int32_t outputIndex, int32_t* width, int32_t* height, int32_t* channel, int32_t* number) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    // Get current process id
    pid_t stub_pid = getpid();
    // Print out pid
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);

    RtRet result = RT_SUCCESS;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);

        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null\n");
            return RT_FAILED;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->getOutputBufferShape(stub_pid, modelId, outputIndex,
            [&](uint32_t rtRet, int32_t outputWidth, int32_t outputHeight,
            int32_t outputChannel, int32_t outputNumber) {
                if (rtRet == RT_SUCCESS) {
                    *width = outputWidth;
                    *height = outputHeight;
                    *channel = outputChannel;
                    *number = outputNumber;
                } else {
                    *width = 0;
                    *height = 0;
                    *channel = 0;
                    *number = 0;
                    result = (RtRet)rtRet;
                }
            }
        );

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            if (result == RT_SUCCESS) {
                LOGD(EDEN_RT_STUB, "service->getInputBufferShape success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->getInputBufferShape: %d\n", result);
            }
        }
    }

    LOGI(EDEN_RT_STUB, "(-)\n");
    return (RtRet)result;
}

RtRet GetEdenVersion(uint32_t modelId, int32_t* versions) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    pid_t stub_pid = getpid();  // Get current process id
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);  // Print out pid
    if (versions == nullptr) {
        return RT_PARAM_INVALID;
    }
    RtRet result = RT_SUCCESS;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);
        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null\n");
            return RT_FAILED;
        }

        // android::hardware::Return<uint32_t> ret;
        auto ret = service->getEdenVersion(stub_pid, modelId, [&](uint32_t rtRet, const auto hv) {
            if (rtRet == RT_SUCCESS) {
                std::vector<int32_t> vec = hv;
                for (uint32_t i = 0; i < vec.size(); i++) {
                    versions[i] = vec.at(i);
                }
            } else {
                result = (RtRet)rtRet;
            }
        });

        if (!ret.isOk()) {
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->getEdenVersion success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->getEdenVersion: %d\n", result);
            }
        }
    }
    LOGI(EDEN_RT_STUB, "(-)\n");
    return result;
}

RtRet GetCompileVersion(uint32_t modelId, EdenModelFile* modelFile, char versions[][256]) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    pid_t stub_pid = getpid();  // Get current process id
    LOGD(EDEN_RT_STUB, "stub_pid=[%d]\n", stub_pid);  // Print out pid
    if (versions == nullptr) {
        return RT_PARAM_INVALID;
    }
    if (modelId == INVALID_MODEL_ID && modelFile == nullptr) {
        return RT_PARAM_INVALID;
    }

    EdenModelHidl emh;
    if (modelFile != nullptr) {
        emh.modelFileFormat = (EdenModelFileHidl) modelFile->modelFileFormat;
        emh.pathToModelFile = hidl_string((char*)modelFile->pathToModelFile, modelFile->lengthOfPath);
        emh.lengthOfPath = modelFile->lengthOfPath;
    } else {
        emh.pathToModelFile = hidl_string("");
        emh.lengthOfPath = 0;
    }

    RtRet result = RT_SUCCESS;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_service);
        if (service == nullptr) {
            LOGD(EDEN_RT_STUB, "service is null\n");
            return RT_FAILED;
        }
        auto ret = service->getCompileVersion(stub_pid, modelId, emh, [&](uint32_t rtRet, const auto hv) {
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
            LOGE(EDEN_RT_STUB, "ret.isOk() is false (-)\n");
            return RT_FAILED;
        } else {
            if (!result) {
                LOGD(EDEN_RT_STUB, "service->getCompileVersion success\n");
            } else {
                LOGE(EDEN_RT_STUB, "service->getCompileVersion: %d\n", result);
            }
        }
    }
    LOGI(EDEN_RT_STUB, "(-)\n");
    return result;
}

RtRet GetCompileVersionFromMemory(ModelTypeInMemory typeInMemory, int8_t* addr, int32_t size, bool encrypted,
                                    char versions[][VERSION_LENGTH_MAX]) {
    LOGI(EDEN_RT_STUB, "(+)\n");

    RtRet result = RT_SUCCESS;
    void* hidlData;

    do {
        // Prepare HIDL data to be delivered
        RtRet rtRet = PrepareGetCompileVersionFromMemory(typeInMemory, addr, size, encrypted, &hidlData);
        if (rtRet != RT_SUCCESS) {
            LOGE(EDEN_RT_STUB, "Error on PrepareGetCompileVersionFromMemory! (rtRet=%d) (modelTypeInMemory=%d, addr=%p, size=%d, encrypted=%d\n",
                 rtRet, typeInMemory, addr, size, encrypted);
            result = RT_ERROR_ON_PREPARE_OPEN_MODEL_FROM_MEMORY;
            // Escape routine since there's no way to going further.
            break;
        }

        // Send HIDL data to server
        rtRet = CallGetCompileVersionFromMemory(hidlData, versions);
        if (rtRet != RT_SUCCESS) {
            LOGE(EDEN_RT_STUB, "Error on CallGetCompileVersionFromMemory! (rtRet=%d) (modelTypeInMemory=%d, addr=%p, size=%d, encrypted=%d\n",
                 rtRet, typeInMemory, addr, size, encrypted);
            result = RT_ERROR_ON_CALL_OPEN_MODEL_FROM_MEMORY;
            // Error occured, but it needs to go below to release allocated memory on hidlData.
            // And this release would be taken place at UnprepareGetCompileVersionFromMemory(..)
        }

        // Unprepare
        rtRet = UnprepareGetCompileVersionFromMemory(hidlData);
        if (rtRet != RT_SUCCESS) {
            LOGE(EDEN_RT_STUB, "Error on UnprepareGetCompileVersionFromMemory! (rtRet=%d) (modelTypeInMemory=%d, addr=%p, size=%d, encrypted=%d\n",
                 rtRet, typeInMemory, addr, size, encrypted);
            // No override result when it already keeps an error code.
            if (result == RT_SUCCESS) {
                result = RT_ERROR_ON_UNPREPARE_OPEN_MODEL_FROM_MEMORY;
            }
        }
    } while (0);

    LOGI(EDEN_RT_STUB, "(-)\n");
    return result;
}

#if 0
RtRet GetState(EdenState* state) {
    LOGD(EDEN_RT_STUB, "%s started\n", __func__);

    if (service == NULL) {
        LOGD(EDEN_RT_STUB, "service is null\n");
        return RT_FAILED;
    }
}
#endif

}  // rt
}  // eden

