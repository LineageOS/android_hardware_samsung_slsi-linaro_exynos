/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <thread>
#include <hardware/exynos/ion.h>
#include <sys/mman.h>             // mmap
#include <map>                    // map
#include <mutex>
#include <shared_mutex>
#include <algorithm>              // remove
#include <system_error>
#include <sys/types.h>            // pid_t
#include <errno.h>                // errno
#include <unistd.h>               // access
#include <signal.h>               // kill

#include <sys/syscall.h>
#include "cutils/properties.h"

#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>

#include "Edenruntime.h"

// eden framework
#include "eden_memory.h"
#include "EdenRuntime.h"

#include "log.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SERVICE"

#define CLRHIGH(u64)    ((u64) & 0x00000000FFFFFFFF)
#define LOWU32(u64)     ((uint32_t)((u64) & 0x00000000FFFFFFFF))
#define HIGHU32(u64)    ((uint32_t)((u64) >> 32))

#define ALIVE_MONITOR_TIMEOUT   (5)
#define MAX_PATH_LENGTH (512)

#define CHECK_PATH_LENGTH(len, cb)                                       \
    do {                                                                 \
        if (len > MAX_PATH_LENGTH) {                                     \
            LOGE(EDEN_HIDL, "path length is over %d", MAX_PATH_LENGTH);  \
            cb(RT_FAILED, 0);                                            \
            return Void();                                               \
        }                                                                \
    } while (0)

DEVICE_STATE EdenruntimeState;

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace eden_runtime {
namespace V1_0 {
namespace implementation {

using ::vendor::samsung_slsi::hardware::eden_runtime::V1_0::IEdenruntime;

using ::android::hardware::hidl_memory;
using ::android::hidl::memory::V1_0::IMemory;

typedef enum {
    PID_STATE_INIT_START,
    PID_STATE_INIT_END,
    PID_STATE_OPEN_MODEL_START,
    PID_STATE_OPEN_MODEL_END,
    PID_STATE_EXECUTE_REQ_START,
    PID_STATE_EXECUTE_REQ_END,
    PID_STATE_CLOSE_MODEL_START,
    PID_STATE_CLOSE_MODEL_END,
    PID_STATE_SHUTDOWN_START,
    PID_STATE_SHUTDOWN_END,
    PID_STATE_ALLOCATE_INPUT_BUFFER_START,
    PID_STATE_ALLOCATE_INPUT_BUFFER_END,
    PID_STATE_ALLOCATE_OUTPUT_BUFFER_START,
    PID_STATE_ALLOCATE_OUTPUT_BUFFER_END,
    PID_STATE_FREE_BUFFER_START,
    PID_STATE_FREE_BUFFER_END,
    PID_STATE_GET_INPUT_BUFFER_SHAPE_START,
    PID_STATE_GET_INPUT_BUFFER_SHAPE_END,
    PID_STATE_GET_OUTPUT_BUFFER_SHAPE_START,
    PID_STATE_GET_OUTPUT_BUFFER_SHAPE_END,
} PID_STATE;

typedef struct __PidData {
    std::atomic<PID_STATE> pidState;
    std::vector<uint32_t> modelIds;
    int32_t count;
} PidData;

std::shared_timed_mutex mutex_mapReqCvalue;
std::map<uint64_t, std::pair<CallbackValue, sp<IEdenruntimeCallback>>> mapReqCvalue;
std::map<int64_t, int64_t> mapReqPid;                        // (EdenRequest*, pid)

std::shared_timed_mutex mutex_mapBufferHandle;
std::map<uint64_t, std::pair<native_handle_t *, int32_t>> mapBufferHandle;

std::condition_variable cond_monitorThread;
std::mutex mutex_waitMonitorThread;
std::mutex mutex_mapPidData;
std::mutex mutex_init;
std::map<int64_t, std::shared_ptr<PidData>> mapPidData;      // (pid, PidData*)
std::map<int64_t, std::shared_ptr<PidData>> newMapPidData;   // (pid, PidData*)

std::mutex mutex_mapPidAlive;
std::map<int64_t, bool> mapPidAlive;                         // (pid, alive)

std::mutex mutex_modelVectorBoostOpt;
std::vector<uint32_t> modelVectorBoostOpt; // model_id

std::mutex mutex_mapPidCallback;
std::map<int64_t, sp<IEdenruntimeCallback>> mapPidCallback;  // (pid, IEdenruntimeCallback*)

// Check if caller is alive
static inline bool CheckCallerIsAlive(const sp<IEdenruntimeCallback>& cb, int32_t pid) {
    LOGD(EDEN_HIDL, "CheckCallerIsAlive!\n");
    android::hardware::Return<uint32_t> ret = cb->isAlive();
    if (!ret.isOk()) {
        LOGD(EDEN_HIDL, "Oops! Caller[pid=%d] is died!\n", pid);
        {
            std::lock_guard<std::mutex> lock(mutex_mapPidAlive);
            mapPidAlive[pid] = false;
        }
        return false;
    } else {
        LOGD(EDEN_HIDL, "Ok! Caller[pid=%d] is alive!\n", pid);
        return true;
    }
}

static bool IsValidPID(pid_t pid) {
    LOGD(EDEN_HIDL, "Check pid=(%d) is valid...\n", pid);

    std::lock_guard<std::mutex> lock(mutex_mapPidAlive);
    if (mapPidAlive[pid] == false) {
        LOGD(EDEN_HIDL, "Check pid=(%d) is NOT valid...Dead!\n", pid);
        return false;
    }
    LOGD(EDEN_HIDL, "Check pid=(%d) is valid...Alive!\n", pid);
    return true;
}

static bool releaseResource(PID_STATE pidState, uint32_t modelId, int32_t count) {
    LOGI(EDEN_HIDL, "%s: started", __func__);

    LOGI(EDEN_HIDL, "Try to release resources, pidState=(%d), modelId=(%d)...\n", pidState, modelId);

    bool handled = false;

    switch (pidState) {
        case PID_STATE_INIT_START:
        case PID_STATE_CLOSE_MODEL_START:
        case PID_STATE_OPEN_MODEL_START:
        case PID_STATE_EXECUTE_REQ_START:
        case PID_STATE_SHUTDOWN_START:
        case PID_STATE_ALLOCATE_INPUT_BUFFER_START:
        case PID_STATE_ALLOCATE_OUTPUT_BUFFER_START:
        case PID_STATE_FREE_BUFFER_START:
        case PID_STATE_GET_INPUT_BUFFER_SHAPE_START:
        case PID_STATE_GET_OUTPUT_BUFFER_SHAPE_START:
            // Not yet ready to be handled
            LOGI(EDEN_HIDL, "Not yet ready to be handled\n");
            handled = false;
            break;

        case PID_STATE_INIT_END:
        case PID_STATE_SHUTDOWN_END:
            // Nothing to be released
            LOGD(EDEN_HIDL, "Nothing to be released!\n");
            handled = true;
            break;

        case PID_STATE_OPEN_MODEL_END:
        case PID_STATE_FREE_BUFFER_END:
        case PID_STATE_ALLOCATE_INPUT_BUFFER_END:
        case PID_STATE_ALLOCATE_OUTPUT_BUFFER_END:
        case PID_STATE_GET_INPUT_BUFFER_SHAPE_END:
        case PID_STATE_GET_OUTPUT_BUFFER_SHAPE_END:
        case PID_STATE_EXECUTE_REQ_END:
        case PID_STATE_CLOSE_MODEL_END:
            if (modelId == INVALID_MODEL_ID) {
                handled = true;
                LOGE(EDEN_HIDL, "modelId is not valid!\n");
                break;
            }

            if (count > 0) {
                handled = true;
                // CloseModel
                LOGI(EDEN_HIDL, "Call CloseModel for modelId=(%d)...\n", modelId);
                eden::rt::CloseModel(modelId);
                LOGI(EDEN_HIDL, "Call CloseModel for modelId=(%d)...Done!\n", modelId);
            } else {
                handled = false;
                LOGI(EDEN_HIDL, "Skip first time to catch up callback is done\n");
            }
            break;

        default:
            // Nothing to be released
            LOGI(EDEN_HIDL, "Nothing to be released!\n");
            break;
    }

    return handled;
}

void processAliveMointorMain() {
    try {
        while (1) {
            // Sleep
            LOGD(EDEN_HIDL, "Sleeping...\n");
            std::unique_lock<std::mutex> lock(mutex_waitMonitorThread);
            cond_monitorThread.wait_for(lock, std::chrono::seconds(ALIVE_MONITOR_TIMEOUT));
            lock.unlock();
            LOGD(EDEN_HIDL, "Wake up and check dead processes on mapPidData!\n");
            // Check whether pid is alive or not!
            {
                {
                    // Update for all pid alive status
                    for (auto iter = mapPidCallback.begin(); iter != mapPidCallback.end(); ++iter) {
                        int64_t pid = iter->first;     // pid
                        auto callback = iter->second;  // sp<IEdenruntimeCallback>
                        CheckCallerIsAlive(callback, pid);
                    }
                }

                {
                    // Release resources for all died pid
                    std::lock_guard<std::mutex> lock(mutex_mapPidData);
                    newMapPidData.clear();
                    for (auto iter = mapPidData.begin(); iter != mapPidData.end(); ++iter) {
                        // Check process alive
                        int64_t pid = iter->first;
                        auto pidData = iter->second;

                        LOGI(EDEN_HIDL, "Try to check pid=(%ld)...\n", pid);
                        //if (CheckCallerIsAlive(pid) {
                        if (IsValidPID(pid) == true) {
                            // Keep alive pid data
                            newMapPidData[pid] = pidData;
                        } else {
                            bool handled = true;
                            if (pidData->pidState == PID_STATE_OPEN_MODEL_START) {
                                // If the client dies right after requesets open model, it could be happened that model is opened after pidData is deleted.
                                // Then, there is no way to released the model that opened. (function 'releaseResource()' can't be called)

                                // If the other models already exists with same client (same pid), pidState can be changed by other models.
                                // In this case, it seems that this code block won't works as expect, however
                                // since releaseResource() skips to close model first time(check the param : pidData->count++),
                                // we can expect the model open is done until the next time that this thread for release wakes up (5s).
                                LOGI(EDEN_HIDL, "Skip to release client resources until openModel is done\n");
                                handled = false;
                                newMapPidData[pid] = pidData;
                            }
                            // Release all resources based on model IDs allocated for this pid
                            for (auto modelId : pidData->modelIds) {
                                // Release resources allocated for pid
                                handled = releaseResource(pidData->pidState, modelId, pidData->count++);
                                if (handled == false) {
                                    // Keep alive pid data
                                    newMapPidData[pid] = pidData;
                                    break;
                                }
                            }

                            // Delete died pid from mapPidAlive
                            if (handled == true) {
                                // Delete
                                LOGD(EDEN_HIDL, "Delete died pid from mapPidAlive...\n");
                                mapPidAlive.erase(pid);
                                mapPidCallback.erase(pid);
                            }
                        }
                    }
                    // Swap to new data
                    mapPidData.swap(newMapPidData);
                }
            }
        }
    } catch (const std::exception& e) {
        LOGE(EDEN_HIDL, "Caught exception with meaning=[%s]\n", e.what());
    }
}

void edenNotify(addr_t* addr, addr_t value) {
    LOGD(EDEN_HIDL, "%s, addr: %p, value: %p\n", __func__, addr, (addr_t*)(value));

    std::map<uint64_t,
        std::pair<CallbackValue, sp<IEdenruntimeCallback>>>::iterator it, it_end;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_mapReqCvalue);
        it = mapReqCvalue.find(value);
        it_end = mapReqCvalue.end();
    }
    if (it == it_end) {
        LOGE(EDEN_HIDL, "failed to find callback value according to request addr");
        return;
    }

    CallbackValue cValue = it->second.first;
    sp<IEdenruntimeCallback> cb = it->second.second;

    if (value == 0) {
        LOGE(EDEN_HIDL, "request is null");
        return;
    }

    EdenRequest* request = (EdenRequest*)value;

    eden_memory_t* newEmaBuffers;
    int32_t isBinding = 0;
    RtRet rtRet = eden::rt::GetMatchedEMABuffers(request->modelId, request->outputBuffers,
                                                                (void**)&newEmaBuffers, &isBinding);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        return;
    }

    std::map<uint64_t, std::pair<native_handle_t *, int32_t>>::iterator it2, it2_end;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_mapBufferHandle);
        it2 = mapBufferHandle.find(cValue.outbufAddr);
        it2_end = mapBufferHandle.end();
    }
    if (it2 == it2_end) {
        LOGE(EDEN_HIDL, "failed to find callback value according to request addr");
        return;
    }

    native_handle_delete(it2->second.first);

    int32_t num = it2->second.second;

    EdenBuffer* buffers;
    buffers = request->outputBuffers;

    native_handle_t* nh = native_handle_create(num, num + 2);
    if (isBinding) {
        for (int idx = 0; idx < num; idx++) {
            eden_memory_t* emaBuffer = &(newEmaBuffers[0]);
            nh->data[idx] = emaBuffer->ref.ion.fd;
            nh->data[num + idx] = buffers[idx].size;
            LOGD(EDEN_HIDL, "fd: %d, size: %d\n", nh->data[idx], nh->data[num + idx]);
        }
        nh->data[num] |= (1 << 31);
    } else {
    for (int idx = 0; idx < num; idx++) {
        eden_memory_t* emaBuffer = &(newEmaBuffers[idx]);
        nh->data[idx] = emaBuffer->ref.ion.fd;
        nh->data[num + idx] = emaBuffer->size;
        LOGD(EDEN_HIDL, "fd: %d, size: %d\n", nh->data[idx], nh->data[num + idx]);
    }
    }
    nh->data[num * 2] = HIGHU32((uint64_t)buffers);
    nh->data[num * 2 + 1] = LOWU32((uint64_t)buffers);

    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapBufferHandle);
        mapBufferHandle.erase(cValue.outbufAddr);
        mapBufferHandle[(uint64_t)buffers] = std::make_pair(nh, num);
    }

    // Check before call executeCallback whether caller is still alive...
    pid_t pid = -1;
    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapReqCvalue);
        pid = mapReqPid[value];
    }

    bool skip_notify_rpc = false;
    {
        std::lock_guard<std::mutex> lock(mutex_modelVectorBoostOpt);
        if (!modelVectorBoostOpt.empty()) {
            std::vector<uint32_t>::iterator it =
                    find(modelVectorBoostOpt.begin(), modelVectorBoostOpt.end(), request->modelId);
            if (it != modelVectorBoostOpt.end()) {
                skip_notify_rpc = true;
            }
        }
    }

    if (skip_notify_rpc == false) {
        if (IsValidPID(pid) == true) {
            LOGD(EDEN_HIDL, "pid=%d is alive, call executeCallback!\n", pid);
            android::hardware::Return<uint32_t> ret = cb->executeCallback(cValue.pollingAddr, cValue.value, nh);
            if (!ret.isOk()) {
                LOGD(EDEN_HIDL, "Error!! ret.isOk() is false\n");
            } else {
                LOGD(EDEN_HIDL, "No Error!!\n");
            }
        } else {
            LOGI(EDEN_HIDL, "Oops, caller(App) requested was DEAD!! Skip executeCallback!\n");
        }
    }

    delete request->callback;
    delete request;

    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapReqCvalue);
        mapReqCvalue.erase(value);
        mapReqPid.erase(value);
    }
}

void print_EdenruntimeState(void)
{
    switch (EdenruntimeState) {
        case INITIALIZED:
            LOGD(EDEN_HIDL, "Edenruntime service state is 'INITIALIZED'");
            break;
        case SHUTDOWN:
            LOGD(EDEN_HIDL, "Edenruntime service state is 'SHUTDOWN'");
            break;
        default:
            LOGD(EDEN_HIDL, "Edenruntime service state is 'UNKNOWN'");
            break;
    }
}

static void setCoreAffinity() {
    char buffer[PROPERTY_VALUE_MAX];
    int ret = property_get("ro.vendor.eden.core_mask", buffer, NULL);
    if (ret == 0) {
        LOGW(EDEN_HIDL, "no core_mask value found.");
        return;
    }

    int coreMask = std::atoi(buffer);
    pid_t tid = gettid();
    ret = syscall(__NR_sched_setaffinity, tid, sizeof(coreMask), &coreMask);
    LOGI(EDEN_HIDL, "tid=%d, mask=%x, ret=%d, err=%d", tid, coreMask, ret, errno);
}

class EdenruntimeDeathRecipient : public android::hardware::hidl_death_recipient {
  public:
    EdenruntimeDeathRecipient(const sp<IEdenruntime> rt) : mRt_(rt), has_died_(false) {}

    virtual void serviceDied(uint64_t cookie, const android::wp<::android::hidl::base::V1_0::IBase>& /*who*/) {
        LOGI(EDEN_HIDL, "(Not service)User Process of cookie[pid=%lu] DIED \n", cookie);
        has_died_ = true;
        // Clean up resources
        int64_t pid = (int64_t)cookie;
        LOGD(EDEN_HIDL, "Set mapPidAlive[%ld] to false\n", pid);
        mapPidAlive[pid] = false;
        cond_monitorThread.notify_all();
    }

    bool getHasDied() const { return has_died_; }
    void setHasDied(bool has_died) { has_died_ = has_died; }

  private:
    sp<IEdenruntime> mRt_;
    bool has_died_;
};

Edenruntime::Edenruntime() {
    LOGD(EDEN_HIDL, "%s: started", __func__);

    setCoreAffinity();

    RtRet rtRet = eden::rt::Init();
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        EdenruntimeState = UNKNOWN;

        return;
    }

    death_recipient_ = new EdenruntimeDeathRecipient(this);

    try {
        // Create thread for process alive monitor
        aliveMonitor_ = std::thread(processAliveMointorMain);
    } catch (const std::exception& e) {
        LOGE(EDEN_HIDL, "Caught exception with meaning=[%s]\n", e.what());
        return;
    }

    EdenruntimeState = INITIALIZED;
}

Edenruntime::~Edenruntime() {
    LOGD(EDEN_HIDL, "%s: started", __func__);
    try {
        // remove callback link from stub
        unlink_cb_(death_recipient_);

        if (aliveMonitor_.joinable()) {
            aliveMonitor_.join();
        }

        RtRet rtRet = eden::rt::Shutdown();
        if (rtRet != RT_SUCCESS) {
            LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        }
    } catch (const std::exception& e) {
        LOGE(EDEN_HIDL, "Caught exception with meaning=[%s]\n", e.what());
    }

    EdenruntimeState = SHUTDOWN;
}

// Methods from ::vendor::samsung_slsi::hardware::eden_runtime::V1_0::IEdenruntime follow.
Return<uint32_t> Edenruntime::init(int32_t pid, const sp<IEdenruntimeCallback>& cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);
    std::lock_guard<std::mutex> lock(mutex_init);
    {
        std::lock_guard<std::mutex> lock(mutex_mapPidData);
        // Generate PidData for pid
        mapPidData[pid] = std::make_shared<PidData>();
        mapPidData[pid]->pidState = PID_STATE_INIT_START;
        mapPidData[pid]->count = 0;
        mapPidAlive[pid] = true;
    }

    if (cb == nullptr) {
        LOGE(EDEN_HIDL, "Error, cb is null!\n");
        return RT_PARAM_INVALID;
    }

    death_recipient_->setHasDied(false);
    cb->linkToDeath(death_recipient_, pid);

    unlink_cb_ = [&](sp<EdenruntimeDeathRecipient>& death_recipient) {
        if (death_recipient->getHasDied()) {
            LOGE(EDEN_HIDL, "Skipping unlink call, service died!\n");
        }
        else {
            cb->unlinkToDeath(death_recipient_);
        }
    };

    print_EdenruntimeState();

    // atomic store
    if (mapPidData.count(pid) > 0) {
        mapPidData[pid]->pidState = PID_STATE_INIT_END;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_mapPidCallback);
        if (mapPidCallback.count(pid) > 0) {
            mapPidCallback.erase(pid);
        }
        mapPidCallback[pid] = cb;
    }
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return RT_SUCCESS;
}

Return<void> Edenruntime::openModel(int32_t pid, const EdenModelHidl& eModelHidl,
                                    const EdenModelOptionsHidl& mOptionsHidl,
                                    openModel_cb _hidl_cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);
    {
        std::lock_guard<std::mutex> lock(mutex_mapPidData);
        if (mapPidData.count(pid) > 0) {
            mapPidData[pid]->pidState = PID_STATE_OPEN_MODEL_START;
        }
    }

    EdenModelFile modelFile;
    uint32_t modelId;

    modelFile.modelFileFormat = (EdenFileFormat)eModelHidl.modelFileFormat;
    if (eModelHidl.lengthOfPath > 0) {
        CHECK_PATH_LENGTH(eModelHidl.pathToModelFile.size(), _hidl_cb);
        modelFile.lengthOfPath = eModelHidl.pathToModelFile.size() + 1;
        modelFile.pathToModelFile = new int8_t[modelFile.lengthOfPath];
        strncpy(reinterpret_cast<char*>(modelFile.pathToModelFile), eModelHidl.pathToModelFile.c_str(),
                modelFile.lengthOfPath);
        LOGD(EDEN_HIDL, "pathToModelFile: %s", modelFile.pathToModelFile);
    } else {
        modelFile.pathToModelFile = nullptr;
    }

    if (eModelHidl.lengthOfWeightBias > 0) {
        CHECK_PATH_LENGTH(eModelHidl.pathToWeightBiasFile.size(), _hidl_cb);
        modelFile.lengthOfWeightBias = eModelHidl.pathToWeightBiasFile.size() + 1;
        modelFile.pathToWeightBiasFile = new int8_t[modelFile.lengthOfWeightBias];
        strncpy(reinterpret_cast<char*>(modelFile.pathToWeightBiasFile),
                eModelHidl.pathToWeightBiasFile.c_str(), modelFile.lengthOfWeightBias);
        LOGD(EDEN_HIDL, "pathToWeightBiasFile: %s", modelFile.pathToWeightBiasFile);
    } else {
        modelFile.pathToWeightBiasFile = nullptr;
    }

    EdenModelOptions options;
    options.modelPreference.nnApiType           = (NnApiType) mOptionsHidl.modelPreference.nnApiType;
    options.priority                            = (ModelPriority) mOptionsHidl.priority;

    ModelPreferenceHidl modelPrefHidl = mOptionsHidl.modelPreference;
    options.modelPreference.userPreference.hw   = (HwPreference) modelPrefHidl.userPreference.hw;
    options.modelPreference.userPreference.mode = (ModePreference) modelPrefHidl.userPreference.mode;

    options.modelPreference.userPreference.inputBufferMode.enable = static_cast<bool>(modelPrefHidl.userPreference.inputBufferMode.enable);
    options.modelPreference.userPreference.inputBufferMode.setInputAsFloat = static_cast<bool>(modelPrefHidl.userPreference.inputBufferMode.setInputAsFloat);

    options.latency = static_cast<uint32_t>(mOptionsHidl.latency);
    options.boundCore = static_cast<uint32_t>(mOptionsHidl.boundCore);
    options.presetScenarioId = static_cast<int32_t>(mOptionsHidl.presetScenarioId);

    RtRet rtRet = eden::rt::OpenModelFromFile(&modelFile, &modelId, options);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d]), (pid=%d), (modelId=[%d])!\n", rtRet, pid, modelId);
        _hidl_cb((uint32_t)rtRet, 0);
    } else {
        LOGD(EDEN_HIDL, "Success! Call hidl_cb(RT_SUCCESS, modelId)!, (pid=%d), (modelId=%d)\n", pid, modelId);
        _hidl_cb(RT_SUCCESS, modelId);
    }

    // Below check should be done regardless of IsValidPID(pid)
    // When OpenModel is failed, it is already destroyed in OpenModel itself.
    // When OpenModel is success, but IsValidPID(pid) is failed,
    // it should be kept on mapPidData so that it will be released properly.
    // When OpenModel is success, but IsValidPid(pid_ is success,
    // it should be kept on mapPidData and it might have change to be released
    // by CloseModel from user process(pid)
    {
        std::lock_guard<std::mutex> lock(mutex_mapPidData);
        // Keep modelId when OpenModelFromFile is success!
        if (rtRet == RT_SUCCESS) {
            if (mapPidData.count(pid) > 0) {
                mapPidData[pid]->modelIds.push_back(modelId);
            }
        }
        if (mapPidData.count(pid) > 0) {
            mapPidData[pid]->pidState = PID_STATE_OPEN_MODEL_END;
        }
    }

    if (rtRet == RT_SUCCESS) {
        if ((ModePreference)mOptionsHidl.modelPreference.userPreference.mode == BOOST_MODE_BLOCK_OPTIMIZED) {
            std::lock_guard<std::mutex> lock(mutex_modelVectorBoostOpt);
            modelVectorBoostOpt.push_back(modelId);
        }
    }

    delete[] modelFile.pathToModelFile;
    delete[] modelFile.pathToWeightBiasFile;
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::openModelFromMemory(int32_t pid, const EdenModelFromMemoryHidl& eModelfromMemoryHidl,
                                              const EdenModelOptionsHidl& mOptionsHidl,
                                              openModelFromMemory_cb _hidl_cb) {
    LOGD(EDEN_HIDL, "%s: started, (caller pid=%d)", __func__, pid);
    {
        std::lock_guard<std::mutex> lock(mutex_mapPidData);
        if (mapPidData.count(pid) > 0) {
            mapPidData[pid]->pidState = PID_STATE_OPEN_MODEL_START;
        }
    }

    ModelTypeInMemory modelTypeInMemory = (ModelTypeInMemory)eModelfromMemoryHidl.modelTypeInMemory;

    hidl_memory hidlMemory = eModelfromMemoryHidl.mem;
    sp<IMemory> memory = mapMemory(hidlMemory);
    memory->update();
    void* data = memory->getPointer();
    int8_t* addr = (int8_t*)data;
    int32_t size = memory->getSize();

    LOGD(EDEN_HIDL, "hidl_memory's addr=[%p], size=[%d]\n", addr, size);

    bool encrypted = eModelfromMemoryHidl.encrypted;
    uint32_t modelId;

    EdenModelOptions options;
    options.modelPreference.nnApiType           = (NnApiType) mOptionsHidl.modelPreference.nnApiType;
    options.priority                            = (ModelPriority) mOptionsHidl.priority;

    ModelPreferenceHidl modelPrefHidl = mOptionsHidl.modelPreference;
    options.modelPreference.userPreference.hw   = (HwPreference) modelPrefHidl.userPreference.hw;
    options.modelPreference.userPreference.mode = (ModePreference) modelPrefHidl.userPreference.mode;

    options.modelPreference.userPreference.inputBufferMode.enable = static_cast<bool>(modelPrefHidl.userPreference.inputBufferMode.enable);
    options.modelPreference.userPreference.inputBufferMode.setInputAsFloat = static_cast<bool>(modelPrefHidl.userPreference.inputBufferMode.setInputAsFloat);

    options.latency = static_cast<uint32_t>(mOptionsHidl.latency);
    options.boundCore = static_cast<uint32_t>(mOptionsHidl.boundCore);
    options.presetScenarioId = static_cast<int32_t>(mOptionsHidl.presetScenarioId);

    RtRet rtRet = eden::rt::OpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, &modelId, options);

    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d]), (pid=%d), (modelId=[%d])!\n", rtRet, pid, modelId);
        _hidl_cb((uint32_t)rtRet, 0);
    } else {
        LOGD(EDEN_HIDL, "Success! Call hidl_cb(RT_SUCCESS, modelId)!, (pid=%d), (modelId=%d)\n", pid, modelId);
        _hidl_cb(RT_SUCCESS, modelId);
    }

    // Below check should be done regardless of IsValidPID(pid)
    // When OpenModel is failed, it is already destroyed in OpenModel itself.
    // When OpenModel is success, but IsValidPID(pid) is failed,
    // it should be kept on mapPidData so that it will be released properly.
    // When OpenModel is success, but IsValidPid(pid_ is success,
    // it should be kept on mapPidData and it might have change to be released
    // by CloseModel from user process(pid)
    {
        std::lock_guard<std::mutex> lock(mutex_mapPidData);
        // Keep modelId when OpenModelFromFile is success!
        if (rtRet == RT_SUCCESS) {
            if (mapPidData.count(pid) > 0) {
                mapPidData[pid]->modelIds.push_back(modelId);
            }
        }
        if (mapPidData.count(pid) > 0) {
            mapPidData[pid]->pidState = PID_STATE_OPEN_MODEL_END;
        }
    }

    if (rtRet == RT_SUCCESS) {
        if ((ModePreference)mOptionsHidl.modelPreference.userPreference.mode == BOOST_MODE_BLOCK_OPTIMIZED) {
            std::lock_guard<std::mutex> lock(mutex_modelVectorBoostOpt);
            modelVectorBoostOpt.push_back(modelId);
        }
    }

    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::openModelFromFd(int32_t pid, const EdenModelFromFdHidl& eModelfromFdHidl,
                                          const EdenModelOptionsHidl& mOptionsHidl,
                                          openModelFromFd_cb _hidl_cb) {
    LOGD(EDEN_HIDL, "%s: started, (caller pid=%d)", __func__, pid);
    {
        std::lock_guard<std::mutex> lock(mutex_mapPidData);
        if (mapPidData.count(pid) > 0) {
            mapPidData[pid]->pidState = PID_STATE_OPEN_MODEL_START;
        }
    }

    uint32_t modelId;
    RtRet rtRet = RT_FAILED;

    ModelTypeInMemory modelTypeInMemory = (ModelTypeInMemory)eModelfromFdHidl.modelTypeInMemory;

    const native_handle_t* handle = eModelfromFdHidl.hd.getNativeHandle();
    bool encrypted = eModelfromFdHidl.encrypted;

    int num = handle->numFds;
    if (num != 1) {
        LOGE(EDEN_HIDL, "Error, numFds != 1 is not supported!");
        _hidl_cb((uint32_t)RT_UNSUPPORTED_FEATURES, 0);
    } else {
        int32_t idx = 0;
        int32_t fd = handle->data[idx];
        int32_t size = handle->data[num + idx];
        LOGD(EDEN_HIDL, "fd: %d, size: %d\n", fd, size);

        // mmap
        int8_t* addr = (int8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        LOGD(EDEN_HIDL, "mmap addr: %p\n", addr);

        EdenModelOptions options;
        options.modelPreference.nnApiType           = (NnApiType) mOptionsHidl.modelPreference.nnApiType;
        options.priority                            = (ModelPriority) mOptionsHidl.priority;

        ModelPreferenceHidl modelPrefHidl = mOptionsHidl.modelPreference;
        options.modelPreference.userPreference.hw   = (HwPreference) modelPrefHidl.userPreference.hw;
        options.modelPreference.userPreference.mode = (ModePreference) modelPrefHidl.userPreference.mode;

        options.modelPreference.userPreference.inputBufferMode.enable = static_cast<bool>(modelPrefHidl.userPreference.inputBufferMode.enable);
        options.modelPreference.userPreference.inputBufferMode.setInputAsFloat = static_cast<bool>(modelPrefHidl.userPreference.inputBufferMode.setInputAsFloat);

        options.latency = static_cast<uint32_t>(mOptionsHidl.latency);
        options.boundCore = static_cast<uint32_t>(mOptionsHidl.boundCore);
        options.presetScenarioId = static_cast<int32_t>(mOptionsHidl.presetScenarioId);

        rtRet = eden::rt::OpenModelFromMemory(modelTypeInMemory, addr, size, encrypted, &modelId, options);

        // munmap
        LOGD(EDEN_HIDL, "munmap addr: %p, size: %d\n", addr, size);
        munmap(addr, size);

        if (rtRet != RT_SUCCESS) {
            LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d]), (pid=%d), (modelId=[%d])!\n", rtRet, pid, modelId);
            _hidl_cb((uint32_t)rtRet, 0);
        } else {
            LOGD(EDEN_HIDL, "Success! Call hidl_cb(RT_SUCCESS, modelId)!, (pid=%d), (modelId=%d)\n", pid, modelId);
            _hidl_cb(RT_SUCCESS, modelId);
        }
    }

    // Below check should be done regardless of IsValidPID(pid)
    // When OpenModel is failed, it is already destroyed in OpenModel itself.
    // When OpenModel is success, but IsValidPID(pid) is failed,
    // it should be kept on mapPidData so that it will be released properly.
    // When OpenModel is success, but IsValidPid(pid_ is success,
    // it should be kept on mapPidData and it might have change to be released
    // by CloseModel from user process(pid)
    {
        std::lock_guard<std::mutex> lock(mutex_mapPidData);
        // Keep modelId when OpenModelFromFile is success!
        if (rtRet == RT_SUCCESS) {
            mapPidData[pid]->modelIds.push_back(modelId);
        }
        if (mapPidData.count(pid) > 0) {
            mapPidData[pid]->pidState = PID_STATE_OPEN_MODEL_END;
        }
    }

    if (rtRet == RT_SUCCESS) {
        if ((ModePreference)mOptionsHidl.modelPreference.userPreference.mode == BOOST_MODE_BLOCK_OPTIMIZED) {
            std::lock_guard<std::mutex> lock(mutex_modelVectorBoostOpt);
            modelVectorBoostOpt.push_back(modelId);
        }
    }

    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<uint32_t> Edenruntime::executeReq(int32_t pid, const EdenRequestHidl& eReqHidl,
                                     const sp<IEdenruntimeCallback>& eCallback,
                                     const EdenRequestOptionsHidl& rOptionsHidl) {
    LOGD(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);
    if (mapPidData.count(pid) > 0) {
        mapPidData[pid]->pidState = PID_STATE_EXECUTE_REQ_START;
    }

    EdenRequest* request = new EdenRequest;
    EdenEvent* edenEvent;

    request->modelId = eReqHidl.modelId;
    request->callback = new EdenCallback;
    request->callback->notify = edenNotify;

    request->inputBuffers = reinterpret_cast<EdenBuffer *>(eReqHidl.inputAddr);
    LOGD(EDEN_HIDL, "Inputb: %p", request->inputBuffers);

    request->outputBuffers = reinterpret_cast<EdenBuffer *>(eReqHidl.outputAddr);
    LOGD(EDEN_HIDL, "Outputb: %p", request->outputBuffers);

    CallbackValue cv;
    cv.pollingAddr = eReqHidl.cValue.pollingAddr;
    cv.value = eReqHidl.cValue.value;
    cv.outbufAddr = (uint64_t)request->outputBuffers;

    EdenRequestOptions options;
    options.requestMode = (RequestMode) rOptionsHidl.requestMode;
    options.userPreference.hw   = (HwPreference) rOptionsHidl.userPreference.hw;
    options.userPreference.mode = (ModePreference) rOptionsHidl.userPreference.mode;

    options.userPreference.inputBufferMode.enable = static_cast<bool>(rOptionsHidl.userPreference.inputBufferMode.enable);
    options.userPreference.inputBufferMode.setInputAsFloat = static_cast<bool>(rOptionsHidl.userPreference.inputBufferMode.setInputAsFloat);

    LOGD(EDEN_HIDL, "polling addr: %p", (addr_t*)(eReqHidl.cValue.pollingAddr));
    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapReqCvalue);
        mapReqCvalue[(uint64_t)request] = std::make_pair(cv, eCallback);
        mapReqPid[(uint64_t)request] = pid;
    }

    LOGD(EDEN_HIDL, "options.userPreference.hw : %d \n", options.userPreference.hw);
    LOGD(EDEN_HIDL, "options.userPreference.inputBufferMode.enable : %d \n", options.userPreference.inputBufferMode.enable);
    LOGD(EDEN_HIDL, "options.userPreference.inputBufferMode.setInputAsFloat : %d \n", options.userPreference.inputBufferMode.setInputAsFloat);
    LOGD(EDEN_HIDL, "options.userPreference.mode : %d \n", options.userPreference.mode);
    LOGD(EDEN_HIDL, "options.requestMode : %d \n", options.requestMode);

    RtRet rtRet = eden::rt::ExecuteReq(request, &edenEvent, options);

    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        delete request->callback;
        delete request;
    }

    if (mapPidData.count(pid) > 0) {
        mapPidData[pid]->pidState = PID_STATE_EXECUTE_REQ_END;
    }
    LOGD(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return rtRet;
}

Return<uint32_t> Edenruntime::closeModel(int32_t pid, uint32_t modelId) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);
    if (mapPidData.count(pid) > 0) {
        mapPidData[pid]->pidState = PID_STATE_CLOSE_MODEL_START;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_modelVectorBoostOpt);
        if (!modelVectorBoostOpt.empty()) {
            std::vector<uint32_t>::iterator it;
            it = find(modelVectorBoostOpt.begin(), modelVectorBoostOpt.end(), modelId);
            if (it != modelVectorBoostOpt.end())
                modelVectorBoostOpt.erase(it);
        }
    }

    RtRet rtRet = eden::rt::CloseModel(modelId);

    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
    } else {
		LOGD(EDEN_HIDL, "RT_SUCCESS (rtRet=[%d])!\n", rtRet);
	}

    {
        std::lock_guard<std::mutex> lock(mutex_mapPidData);
        // Keep modelId when OpenModelFromFile is success!
        if (rtRet == RT_SUCCESS) {
            // erase-remove idiom
            if (mapPidData.count(pid) > 0) {
                std::vector<uint32_t> &vec = mapPidData[pid]->modelIds;
                vec.erase(std::remove(vec.begin(), vec.end(), modelId), vec.end());
            }
        }
        if (mapPidData.count(pid) > 0) {
            mapPidData[pid]->pidState = PID_STATE_CLOSE_MODEL_END;
        }
    }

    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return rtRet;
}

Return<uint32_t> Edenruntime::shutdown(int32_t pid) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);
    if (mapPidData.count(pid) > 0) {
        mapPidData[pid]->pidState = PID_STATE_SHUTDOWN_END;
    }
    mapPidData[pid]->pidState = PID_STATE_SHUTDOWN_START;

    print_EdenruntimeState();

    if (mapPidData.count(pid) > 0) {
        mapPidData[pid]->pidState = PID_STATE_SHUTDOWN_END;
    }

    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return RT_SUCCESS;
}

Return<void> Edenruntime::allocateInputBuffer(int32_t pid, uint32_t modelId,
                                              allocateInputBuffer_cb _hidl_cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);
    mapPidData[pid]->pidState = PID_STATE_ALLOCATE_INPUT_BUFFER_START;

    EdenBuffer* buffers;
    int32_t numOfBuffers;

    RtRet rtRet = eden::rt::AllocateInputBuffers(modelId, &buffers, &numOfBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    eden_memory_t* newEmaBuffers;

    rtRet = eden::rt::GetMatchedEMABuffers(modelId, buffers, (void**)&newEmaBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    int32_t num = numOfBuffers;
    // +2 is for EdenBuffer 64bit address. It will be save half address with 32 bit address.
    native_handle_t* nh = native_handle_create(num, num + 2);
    for (int idx = 0; idx < num; idx++) {
        eden_memory_t* emaBuffer = &(newEmaBuffers[idx]);
        nh->data[idx] = emaBuffer->ref.ion.fd;
        nh->data[num + idx] = emaBuffer->size;
        LOGD(EDEN_HIDL, "fd: %d, size: %d\n", nh->data[idx], nh->data[num + idx]);
    }
    nh->data[num * 2] = HIGHU32((uint64_t)buffers);
    nh->data[num * 2 + 1] = LOWU32((uint64_t)buffers);

    LOGD(EDEN_HIDL, "buffers: %p", buffers);
    LOGD(EDEN_HIDL, "front: %x", nh->data[num * 2]);
    LOGD(EDEN_HIDL, "end: %x", nh->data[num * 2 + 1]);

    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapBufferHandle);
        mapBufferHandle[(uint64_t)buffers] = std::make_pair(nh, num);
    }

    _hidl_cb(RT_SUCCESS, nh);
    mapPidData[pid]->pidState = PID_STATE_ALLOCATE_INPUT_BUFFER_END;
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::allocateOutputBuffer(int32_t pid, uint32_t modelId,
                                               allocateOutputBuffer_cb _hidl_cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);
    mapPidData[pid]->pidState = PID_STATE_ALLOCATE_OUTPUT_BUFFER_START;

    EdenBuffer* buffers;
    int32_t numOfBuffers;
    int32_t isBinding;

    RtRet rtRet = eden::rt::AllocateOutputBuffers(modelId, &buffers, &numOfBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    eden_memory_t* newEmaBuffers;

    rtRet = eden::rt::GetMatchedEMABuffers(modelId, buffers, (void**)&newEmaBuffers, &isBinding);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    int32_t num = numOfBuffers;
    native_handle_t* nh = native_handle_create(num, num + 2);
    LOGD(EDEN_HIDL, "numOfBuffers: %d\n", numOfBuffers);
    if (isBinding) {
        LOGD(EDEN_HIDL, "num: %d\n", num);
        for (int idx = 0; idx < num; idx++) {
            eden_memory_t* emaBuffer = &(newEmaBuffers[0]);
            nh->data[idx] = emaBuffer->ref.ion.fd;
            nh->data[num + idx] = buffers[idx].size;
            LOGD(EDEN_HIDL, "idx : %d , fd: %d, size: %d\n", idx, nh->data[idx], nh->data[num + idx]);
        }
        /* Set First Size value's MSB to 1, to distinguish Binding Buffer */
        nh->data[num] |= (1 << 31);
    } else {
    for (int idx = 0; idx < num; idx++) {
        eden_memory_t* emaBuffer = &(newEmaBuffers[idx]);
        nh->data[idx] = emaBuffer->ref.ion.fd;
        nh->data[num + idx] = emaBuffer->size;
        LOGD(EDEN_HIDL, "fd: %d, size: %d\n", nh->data[idx], nh->data[num + idx]);
    }
    }
    nh->data[num * 2] = HIGHU32((uint64_t)buffers);
    nh->data[num * 2 + 1] = LOWU32((uint64_t)buffers);

    LOGD(EDEN_HIDL, "buffers: %p", buffers);
    LOGD(EDEN_HIDL, "front: %x", nh->data[num * 2]);
    LOGD(EDEN_HIDL, "end: %x", nh->data[num * 2 + 1]);

    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapBufferHandle);
        mapBufferHandle[(uint64_t)buffers] = std::make_pair(nh, num);
    }

    _hidl_cb(RT_SUCCESS, nh);
    mapPidData[pid]->pidState = PID_STATE_ALLOCATE_OUTPUT_BUFFER_END;
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::loadInputBuffers(int32_t pid, uint32_t modelId, uint32_t numOfBuffers, const hidl_vec<UserBufferHidl>& userBuffersHidl, loadInputBuffers_cb _hidl_cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);

    EdenBuffer* buffers;
    UserBuffer userBuffers[numOfBuffers];

    for (int idx = 0; idx < numOfBuffers; idx++) {
        userBuffers[idx].size = userBuffersHidl[idx].size;
        if ((UserBufferType)userBuffersHidl[idx].type == FD) {
            userBuffers[idx].type = FD;
            userBuffers[idx].elem.fd = dup(userBuffersHidl[idx].hd->data[0]);
        } else {
            LOGE(EDEN_HIDL, "VADDR UserBufferType Not Supported \n");
            _hidl_cb(RT_UNSUPPORTED_FEATURES, 0);
            return Void();
        }
    }
    RtRet rtRet = eden::rt::LoadInputBuffers(modelId, userBuffers, numOfBuffers, &buffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    eden_memory_t* newEmaBuffers;

    rtRet = eden::rt::GetMatchedEMABuffers(modelId, buffers, (void**)&newEmaBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    int32_t num = numOfBuffers;
    // +2 is for EdenBuffer 64bit address. It will be save half address with 32 bit address.
    native_handle_t* nh = native_handle_create(num, num + 2);
    for (int idx = 0; idx < num; idx++) {
        eden_memory_t* emaBuffer = &(newEmaBuffers[idx]);
        nh->data[idx] = emaBuffer->ref.ion.fd;
        nh->data[num + idx] = emaBuffer->size;
        LOGD(EDEN_HIDL, "fd: %d, size: %d\n", nh->data[idx], nh->data[num + idx]);
    }
    nh->data[num * 2] = HIGHU32((uint64_t)buffers);
    nh->data[num * 2 + 1] = LOWU32((uint64_t)buffers);

    LOGD(EDEN_HIDL, "buffers: %p", buffers);
    LOGD(EDEN_HIDL, "front: %x", nh->data[num * 2]);
    LOGD(EDEN_HIDL, "end: %x", nh->data[num * 2 + 1]);

    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapBufferHandle);
        mapBufferHandle[(uint64_t)buffers] = std::make_pair(nh, num);
    }

    _hidl_cb(RT_SUCCESS, nh);
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::loadOutputBuffers(int32_t pid, uint32_t modelId, uint32_t numOfBuffers, const hidl_vec<UserBufferHidl>& userBuffersHidl, loadOutputBuffers_cb _hidl_cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);

    EdenBuffer* buffers;
    UserBuffer userBuffers[numOfBuffers];

    for (int idx = 0; idx < numOfBuffers; idx++) {
        userBuffers[idx].size = userBuffersHidl[idx].size;
        if ((UserBufferType)userBuffersHidl[idx].type == FD) {
            userBuffers[idx].type = FD;
            userBuffers[idx].elem.fd = dup(userBuffersHidl[idx].hd->data[0]);
        } else {
            LOGE(EDEN_HIDL, "VADDR UserBufferType Not Supported \n");
            _hidl_cb(RT_UNSUPPORTED_FEATURES, 0);
            return Void();
        }
    }
    RtRet rtRet = eden::rt::LoadOutputBuffers(modelId, userBuffers, numOfBuffers, &buffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    eden_memory_t* newEmaBuffers;

    rtRet = eden::rt::GetMatchedEMABuffers(modelId, buffers, (void**)&newEmaBuffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    int32_t num = numOfBuffers;
    // +2 is for EdenBuffer 64bit address. It will be save half address with 32 bit address.
    native_handle_t* nh = native_handle_create(num, num + 2);
    for (int idx = 0; idx < num; idx++) {
        eden_memory_t* emaBuffer = &(newEmaBuffers[idx]);
        nh->data[idx] = emaBuffer->ref.ion.fd;
        nh->data[num + idx] = emaBuffer->size;
        LOGD(EDEN_HIDL, "fd: %d, size: %d\n", nh->data[idx], nh->data[num + idx]);
    }
    nh->data[num * 2] = HIGHU32((uint64_t)buffers);
    nh->data[num * 2 + 1] = LOWU32((uint64_t)buffers);

    LOGD(EDEN_HIDL, "buffers: %p", buffers);
    LOGD(EDEN_HIDL, "front: %x", nh->data[num * 2]);
    LOGD(EDEN_HIDL, "end: %x", nh->data[num * 2 + 1]);

    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapBufferHandle);
        mapBufferHandle[(uint64_t)buffers] = std::make_pair(nh, num);
    }

    _hidl_cb(RT_SUCCESS, nh);
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::freeBuffer(int32_t pid, uint32_t modelId, uint64_t bufferAddr,
                                                    freeBuffer_cb _hidl_cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);
    if (mapPidData.count(pid) > 0) {
        mapPidData[pid]->pidState = PID_STATE_FREE_BUFFER_START;
    }

    EdenBuffer* buffers;

    if (bufferAddr == 0) {
        LOGE(EDEN_HIDL, "%s: bufferAddr is null", __func__);
        _hidl_cb(RT_PARAM_INVALID, 0);
        return Void();
    }

    buffers = (EdenBuffer *)bufferAddr;

    std::map<uint64_t, std::pair<native_handle_t *, int32_t>>::iterator it, it_end;
    {
        std::shared_lock<std::shared_timed_mutex> rlock(mutex_mapBufferHandle);
        it = mapBufferHandle.find(bufferAddr);
        it_end = mapBufferHandle.end();
    }
    if (it == it_end) {
        LOGE(EDEN_HIDL, "failed to find map value according to saved server buffer addr");
        _hidl_cb(RT_FAILED, 0);
        return Void();
    }

    native_handle_delete(it->second.first);

    RtRet rtRet = eden::rt::FreeBuffers(modelId, buffers);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb(rtRet , 0);
        return Void();
    }

    _hidl_cb(RT_SUCCESS, it->second.second);
    {
        std::lock_guard<std::shared_timed_mutex> wlock(mutex_mapBufferHandle);
        mapBufferHandle.erase(bufferAddr);
    }

    if (mapPidData.count(pid) > 0) {
        mapPidData[pid]->pidState = PID_STATE_FREE_BUFFER_END;
    }
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::getInputBufferShape(int32_t pid, uint32_t modelId, int32_t inputIndex,
                                              getInputBufferShape_cb _hidl_cb) {
    LOGD(EDEN_HIDL, "%s: started, (caller pid=%d)", __func__, pid);
    mapPidData[pid]->pidState = PID_STATE_GET_INPUT_BUFFER_SHAPE_START;

    int32_t width, height, channel, number;
    RtRet rtRet = eden::rt::GetInputBufferShape(modelId, inputIndex, &width, &height, &channel, &number);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb(rtRet, 0, 0, 0, 0);
        return Void();
    }

    _hidl_cb(RT_SUCCESS, width, height, channel, number);
    mapPidData[pid]->pidState = PID_STATE_GET_INPUT_BUFFER_SHAPE_END;

    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::getOutputBufferShape(int32_t pid, uint32_t modelId, int32_t outputIndex,
                                               getOutputBufferShape_cb _hidl_cb) {
    LOGD(EDEN_HIDL, "%s: started, (caller pid=%d)", __func__, pid);
    mapPidData[pid]->pidState = PID_STATE_GET_OUTPUT_BUFFER_SHAPE_START;

    int32_t width, height, channel, number;
    RtRet rtRet = eden::rt::GetOutputBufferShape(modelId, outputIndex, &width, &height, &channel, &number);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb(rtRet, 0, 0, 0, 0);
        return Void();
    }

    _hidl_cb(RT_SUCCESS, width, height, channel, number);
    mapPidData[pid]->pidState = PID_STATE_GET_OUTPUT_BUFFER_SHAPE_END;

    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<uint32_t> Edenruntime::notifyDead(int32_t pid, int32_t signo) {
    LOGI(EDEN_HIDL, "notifyDead is called with pid=(%d), signo=(%d)\n", pid, signo);
    {
        std::lock_guard<std::mutex> lock(mutex_mapPidAlive);
        mapPidAlive[pid] = false;
    }
    LOGD(EDEN_HIDL, "Now mapPidAlive[%d]=[%d]\n", pid, mapPidAlive[pid]);

    return 0;
}

Return<void> Edenruntime::getEdenVersion(int32_t pid, uint32_t modelId, getEdenVersion_cb _hidl_cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);

    int32_t versions[VERSION_MAX] = {0, };
    RtRet rtRet = eden::rt::GetEdenVersion(modelId, versions);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    android::hardware::hidl_vec<int32_t> hv = std::vector<int32_t>(versions, versions + VERSION_MAX);
    _hidl_cb(RT_SUCCESS, hv);
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::getCompileVersion(int32_t pid, uint32_t modelId, const EdenModelHidl& eModelHidl, getCompileVersion_cb _hidl_cb) {
    LOGI(EDEN_HIDL, "(+) %s: started, (caller pid=%d)", __func__, pid);

    EdenModelFile modelFile;
    if (eModelHidl.modelFileFormat == EdenModelFileHidl::TFLITE && eModelHidl.lengthOfPath != 0) {
        CHECK_PATH_LENGTH(eModelHidl.pathToModelFile.size(), _hidl_cb);
        modelFile.modelFileFormat = (EdenFileFormat)eModelHidl.modelFileFormat;
        modelFile.lengthOfPath = eModelHidl.pathToModelFile.size() + 1;
        modelFile.pathToModelFile = new int8_t[modelFile.lengthOfPath];
        strncpy((char*)modelFile.pathToModelFile, eModelHidl.pathToModelFile.c_str(), modelFile.lengthOfPath);
    }

    RtRet rtRet = RT_SUCCESS;
    char compileVersion[COMPILE_VERSION_MAX][256] = {{0}, };
    if (modelId != INVALID_MODEL_ID) {
        rtRet = eden::rt::GetCompileVersion(modelId, nullptr, compileVersion);
    } else {
        rtRet = eden::rt::GetCompileVersion(INVALID_MODEL_ID, &modelFile, compileVersion);
    }
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    android::hardware::hidl_vec<hidl_string> hv(COMPILE_VERSION_MAX);
    for (int i = 0; i < COMPILE_VERSION_MAX; i++) {
        std::string data(compileVersion[i]);
        hv[i] = data;
    }

    _hidl_cb(RT_SUCCESS, hv);
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::getCompileVersionFromFd(int32_t pid, const EdenModelFromFdHidl& eModelfromFdHidl,
                                                    getCompileVersionFromFd_cb _hidl_cb) {
    LOGD(EDEN_HIDL, "%s: started, (caller pid=%d)", __func__, pid);

    RtRet rtRet = RT_SUCCESS;
    char compileVersion[COMPILE_VERSION_MAX][256] = {{0}, };

    ModelTypeInMemory typeInMemory = (ModelTypeInMemory)eModelfromFdHidl.modelTypeInMemory;
    const native_handle_t* handle = eModelfromFdHidl.hd.getNativeHandle();
    bool encrypted = eModelfromFdHidl.encrypted;

    int num = handle->numFds;
    if (num != 1) {
        LOGE(EDEN_HIDL, "Error, numFds != 1 is not supported!");
        _hidl_cb((uint32_t)RT_UNSUPPORTED_FEATURES, 0);
    } else {
        int32_t idx = 0;
        int32_t fd = handle->data[idx];
        int32_t size = handle->data[num + idx];
        LOGD(EDEN_HIDL, "fd: %d, size: %d\n", fd, size);

        // mmap
        int8_t* addr = (int8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        LOGD(EDEN_HIDL, "mmap addr: %p\n", addr);

        rtRet = eden::rt::GetCompileVersionFromMemory(typeInMemory, addr, size, encrypted, compileVersion);
        // munmap
        LOGD(EDEN_HIDL, "munmap addr: %p, size: %d\n", addr, size);
        munmap(addr, size);
    }

    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    android::hardware::hidl_vec<hidl_string> hv(COMPILE_VERSION_MAX);
    for (int i = 0; i < COMPILE_VERSION_MAX; i++) {
        std::string data(compileVersion[i]);
        hv[i] = data;
    }

    _hidl_cb(RT_SUCCESS, hv);
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

Return<void> Edenruntime::getCompileVersionFromMemory(int32_t pid, const EdenModelFromMemoryHidl& eModelfromMemoryHidl,
                                                        getCompileVersionFromMemory_cb _hidl_cb) {
    LOGD(EDEN_HIDL, "%s: started, (caller pid=%d)", __func__, pid);

    RtRet rtRet = RT_SUCCESS;
    char compileVersion[COMPILE_VERSION_MAX][256] = {{0}, };

    ModelTypeInMemory typeInMemory = (ModelTypeInMemory)eModelfromMemoryHidl.modelTypeInMemory;
    hidl_memory hidlMemory = eModelfromMemoryHidl.mem;
    sp<IMemory> memory = mapMemory(hidlMemory);
    memory->update();
    void* data = memory->getPointer();
    int8_t* addr = (int8_t*)data;
    int32_t size = memory->getSize();

    LOGD(EDEN_HIDL, "hidl_memory's addr=[%p], size=[%d]\n", addr, size);
    bool encrypted = eModelfromMemoryHidl.encrypted;

    rtRet = eden::rt::GetCompileVersionFromMemory(typeInMemory, addr, size, encrypted, compileVersion);
    if (rtRet != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from Runtime, (rtRet=[%d])!\n", rtRet);
        _hidl_cb((uint32_t)rtRet, 0);
        return Void();
    }

    android::hardware::hidl_vec<hidl_string> hv(COMPILE_VERSION_MAX);
    for (int i = 0; i < COMPILE_VERSION_MAX; i++) {
        std::string data(compileVersion[i]);
        hv[i] = data;
    }

    _hidl_cb(RT_SUCCESS, hv);
    LOGI(EDEN_HIDL, "(-) %s: completed, (caller pid=%d)", __func__, pid);
    return Void();
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

Return<void> Edenruntime::debug(const hidl_handle& fd, const hidl_vec<hidl_string>& options) {
    LOGD(EDEN_HIDL, "");
    const native_handle_t* handle = fd.getNativeHandle();
    // vts_ibase_test passes hidl_handle() as its mHandle is nullptr.
    if (handle == nullptr || handle->numFds < 1) {
        return Void();
    }
    int fdh = handle->data[0];
    RtRet ret = eden::rt::dumpAll(fdh);
    if(ret != RT_SUCCESS) {
        LOGE(EDEN_HIDL, "Error from eden::rt::dumpAll \n");
    }
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace eden_runtime
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
