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
 * @file    EdenServiceDelegatorService.cpp
 * @brief   This is EdenServiceDelegatorService class file.
 * @details This header defines EdenServiceDelegatorService class.
 *          This class is implementing proxy role for Eden Runtime service.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#include <iostream>

#include <sys/types.h>  // getpid
#include <unistd.h>     // getpid
#include <sys/mman.h>   // mmap, munmap

#include <map>
#include <mutex>

#include <vendor/samsung_slsi/hardware/eden_runtime/1.0/IEdenruntime.h>

#include "log.h"
#include "Common.h"
#include "EdenServiceDelegatorService.h"
#include "../include/EdenRuntimeType.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::EdenServiceDelegatorService"

#define CLRHIGH(u64)    ((u64) & 0x00000000FFFFFFFF)

// @todo Move inside EdenServiceDelegatorService class
std::mutex mutexService;
std::mutex mutex_mMapClientBufServerBuf;

std::map<uint64_t, std::pair<uint64_t, EdenBuffer**>> mMapClientBufServerBuf_;

class Callback : public IEdenruntimeCallback {
 public:
    Callback(NotifyFuncPtr nfp) {
        notifyFuncPtr_ = nfp;
    }
    uint32_t executeCallback(uint64_t cbAddr, uint64_t value, const hidl_handle& hd) {
        LOGD(EDEN_DRIVER, "executeCallback is called.\n");

        EdenRequest* req = reinterpret_cast<EdenRequest*>value;

        mutex_mMapClientBufServerBuf.lock();
        auto it = mMapClientBufServerBuf_.find(reinterpret_cast<uint64_t>(req->outputBuffers));
        if (it == mMapClientBufServerBuf_.end()) {
            LOGE(EDEN_DRIVER, "failed to find buffers!\n");
            return RT_FAILED;
        }
        EdenBuffer** buffers = it->second.second;
        mutex_mMapClientBufServerBuf.unlock();

        if (buffers == nullptr) {
            LOGE(EDEN_DRIVER, "buffers is nullptr!\n");
            return RT_FAILED;
        }
        const native_handle_t* handle = hd.getNativeHandle();
        int num = handle->numFds;

        for (int idx = 0; idx < num; idx++) {
            EdenBuffer* buffer = &((*buffers)[idx]);
            if (buffer == nullptr) {
                LOGE(EDEN_DRIVER, "buffer is nullptr!\n");
                return RT_FAILED;
            }
            uint32_t fd = handle->data[idx];
            munmap((void *)buffer->addr, buffer->size);
        }

        notifyFuncPtr_(reinterpret_cast<addr_t*>cbAddr, value);

        LOGD(EDEN_DRIVER, "executeCallback is finished.\n");
        return RT_SUCCESS;
    }

 private:
    NotifyFuncPtr notifyFuncPtr_;
};

namespace android {
namespace nn {
namespace eden_driver {

/**
 *  @brief Init EDEN Runtime
 *  @details This API function initializes the CPU/GPU/NPU handler.
 *  @param void
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::Init(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    Callback* cb = new Callback(nullptr);

    mutexService.lock();
    auto ret = service->init(pid, cb);
    mutexService.unlock();

    if (ret.isOk() == false) {
        LOGE(EDEN_DRIVER, "Init() is failed!\n");
    } else {
        LOGD(EDEN_DRIVER, "Init() is success!\n");
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Open EDEN Model
 *  @details This API function set a unique ID to EdenModel, preference, and compute the running path.
 *  @param[in] model it contains overall information.
 *  @param[out] modelId unique id for EDEN Model.
 *  @param[in] preference it determines how to run model with preference.
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::OpenModel(EdenModel* model, uint32_t* modelId, ModelPreference preference) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (model == nullptr || modelId == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "modelId : %u\n", *modelId);

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    // not used this function

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
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
int32_t EdenServiceDelegatorService::OpenModelFromMemory(ModelTypeInMemory modelTypeInMemory, int8_t* addr, int32_t size,
                                                         bool encrypted, uint32_t* modelId, ModelPreference preference) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (addr == nullptr || modelId == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, << "addr : %p, size : %d, modelId : %u\n", addr, size, *modelId);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    // BELOW CODE IS DUMMY!! THEY SHOULD BE REPLACED TO REAL CODE!
    *modelId = 11;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Execute EDEN Req
 *  @details This API function executes EdenRequest with preference.
 *  @param[in] req It consists of EDEN Model ID, input/output buffers and callback.
 *  @param[in] evt Callback function defined by User.
 *  @param[in] preference it determines how to run EdenModel with preference.
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::ExecuteReq(EdenRequest* req, EdenEvent** evt, RequestPreference preference) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (req == nullptr || evt == nullptr || *evt == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    mutex_mMapClientBufServerBuf.lock();
    auto it = mMapClientBufServerBuf_.find(reinterpret_cast<uint64_t>(req->outputBuffers));
    if (it == mMapClientBufServerBuf_.end()) {
        LOGE(EDEN_DRIVER, "failed to find buffers!\n");
        return FAIL_TO_FIND_KEY_IN_MAP;
    }
    mutex_mMapClientBufServerBuf.unlock();

    EdenRequestHidl edenRequestHidl;
    edenRequestHidl.modelId = req->modelId;
    edenRequestHidl.inputAddr = it->second.first;
    edenRequestHidl.cValue.value = reinterpret_cast<uint64_t>(req);
    edenRequestHidl.cValue.pollingAddr = reinterpret_cast<uint64_t>(&(req->callback->requestId));

    RequestPreferenceHidl requestPreferenceHidl;
    requestPreferenceHidl.userPreference.hw = (HwPreferenceHidl)preference.userPreference.hw;
    requestPreferenceHidl.userPreference.mode = (ModePreferenceHidl)preference.userPreference.mode;
    requestPreferenceHidl.userPreference.inputBufferMode.enable =
                                        (bool)preference.userPreference.inputBufferMode.enable;
    requestPreferenceHidl.userPreference.inputBufferMode.setInputAsFloat =
                                        (bool)preference.userPreference.inputBufferMode.setInputAsFloat;

    LOGD(EDEN_DRIVER, "inputBufferMode.enable : %d, inputBufferMode.setInputAsFloat : %d\n",
                       requestPreferenceHidl.userPreference.inputBufferMode.enable, requestPreferenceHidl.userPreference.inputBufferMode.setInputAsFloat);

    Callback* cb = new Callback(req->callback->notify);

    mutexService.lock();
    auto ret = service->executeReq(pid, edenRequestHidl, cb, requestPreferenceHidl);
    mutexService.unlock();

    if (ret.isOk() == false) {
        LOGE(EDEN_DRIVER, "ExecuteReq() is failed!\n");
    } else {
        LOGD(EDEN_DRIVER, "ExecuteReq() is success!\n");
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Execute EDEN Request with RequestOptions
 *  @details This API function executes EdenRequest with requestOptions.
 *  @param[in] request It consists of EDEN Model ID, input/output buffers and callback.
 *  @param[in] requestOptions
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::ExecuteReq(EdenRequest* /*request*/, RequestOptions /*requestOptions*/) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    LOGE(EDEN_DRIVER, "Not yet supported!");

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return INVALID_PARAMS;
}

/**
 *  @brief Close EDEN Model
 *  @details This API function releases resources related with the EDEN Model.
 *  @param[in] modelId It is a unique id for EDEN Model.
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::CloseModel(uint32_t modelId) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    mutexService.lock();
    auto ret = service->closeModel(pid, modelId);
    mutexService.unlock();

    if (ret.isOk() == false) {
        LOGE(EDEN_DRIVER, "CloseModel() is failed!\n");
    } else {
        LOGD(EDEN_DRIVER, "CloseModel() is success!\n");
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Shutdown EDEN Runtime
 *  @details This API function close all EDEN Models with related resources for shutdown EDEN Framework.
 *  @param void
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::Shutdown(void) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    mutexService.lock();
    auto ret = service->shutdown(pid);
    mutexService.unlock();

    if (ret.isOk() == false) {
        LOGE(EDEN_DRIVER, "Shutdown() is failed!\n");
    } else {
        LOGD(EDEN_DRIVER, "Shutdown() is success!\n");
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Allocate a buffer for input to execute a model
 *  @details This function allocates an efficient buffer to execute a model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[out] buffers Array of EdenBuffers for input
 *  @param[out] numOfBuffers # of buffers
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::AllocateInputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr || *buffers == nullptr || numOfBuffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "modelId : %u, numofBuffers : %d\n", modelId, *numOfBuffers);

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    RtRet result = RT_SUCCESS;
    native_handle_t* handle = nullptr;
    std::function<void(uint32_t, hidl_handle)> CBFunc = [&result, &handle](uint32_t rtRet, const auto &hd) {
        if (rtRet != RT_SUCCESS) {
            result = (RtRet)rtRet;
        } else {
            handle = hd.getNativeHandle();
        }
    };

    mutexService.lock();
    service->allocateInputBuffer(pid, modelId, CBFunc);
    mutexService.unlock();

    if (result != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "EdenServiceDelegatorService::AllocateInputBuffers() is failed : %d\n", result);
        return FAIL_TO_GET_INPUT_MEM;
    } else {
        EdenBuffer** tmpBuffer = buffers;
        int numOfEdenBuffers = *numOfBuffers = handle->numFds;
        EdenBuffer* newBuffer = new EdenBuffer[numOfEdenBuffers];
        *buffers = newBuffer;

        for (int idx = 0; idx < numOfEdenBuffers; idx++) {
            EdenBuffer* edenBuffer = &(newBuffer[idx]);
            uint32_t fd = handle->data[idx];
            edenBuffer->size = handle->data[numOfEdenBuffers + idx];
            edenBuffer->addr = mmap(NULL, edenBuffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            //LOGD(EDEN_RT_STUB, "alloc outbuf fd: %d, size: %d\n", fd, buffer->size);
            //LOGD(EDEN_RT_STUB, "out allc mmap addr: %p\n", buffer->addr);
        }
        uint64_t rightAddr = static_cast<uint64_t>(handle->data[numOfEdenBuffers * 2]) << 32;
        uint64_t leftAddr = CLRHIGH(static_cast<uint64_t>(handle->data[numOfEdenBuffers * 2 + 1]));
        uint64_t serverBuffer = static_cast<uint64_t>(rightAddr | leftAddr);

        mutex_mMapClientBufServerBuf.lock();
        mMapClientBufServerBuf_.insert(std::make_pair(reinterpret_cast<uint64_t>(newBuffer), std::make_pair(serverBuffer, tmpBuffer)));
        mutex_mMapClientBufServerBuf.unlock();
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Allocate a buffer for output to execute a model
 *  @details This function allocates an efficient buffer to execute a model.
 *  @param[in] modelId The model id to be applied by.
 *  @param[out] buffers Array of EdenBuffers for input
 *  @param[out] numOfBuffers # of buffers
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::AllocateOutputBuffers(uint32_t modelId, EdenBuffer** buffers, int32_t* numOfBuffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr || *buffers == nullptr || numOfBuffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, "modelId : %u, numofBuffers : %d\n", modelId, *numOfBuffers);

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    RtRet result = RT_SUCCESS;
    native_handle_t* handle = nullptr;
    std::function<void(uint32_t, hidl_handle)> CBFunc = [&result, &handle](uint32_t rtRet, const auto &hd) {
        if (rtRet != RT_SUCCESS) {
            result = (RtRet)rtRet;
        } else {
            handle = hd.getNativeHandle();
        }
    };

    mutexService.lock();
    service->allocateOutputBuffer(pid, modelId, CBFunc);
    mutexService.unlock();

    if (result != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "EdenServiceDelegatorService::AllocateOutputBuffers() is failed : %d\n", result);
        return FAIL_TO_GET_OUTPUT_MEM;
    } else {
        EdenBuffer** tmpBuffer = buffers;
        int numOfEdenBuffers = *numOfBuffers = handle->numFds;
        EdenBuffer* newBuffer = new EdenBuffer[numOfEdenBuffers];
        *buffers = newBuffer;

        for (int idx = 0; idx < numOfEdenBuffers; idx++) {
            EdenBuffer* edenBuffer = &(newBuffer[idx]);
            uint32_t fd = handle->data[idx];
            edenBuffer->size = handle->data[numOfEdenBuffers + idx];
            edenBuffer->addr = mmap(NULL, edenBuffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            //LOGD(EDEN_RT_STUB, "alloc outbuf fd: %d, size: %d\n", fd, buffer->size);
            //LOGD(EDEN_RT_STUB, "out allc mmap addr: %p\n", buffer->addr);
        }
        uint64_t rightAddr = static_cast<uint64_t>(handle->data[numOfEdenBuffers * 2]) << 32;
        uint64_t leftAddr = CLRHIGH(static_cast<uint64_t>(handle->data[numOfEdenBuffers * 2 + 1]));
        uint64_t serverBuffer = static_cast<uint64_t>(rightAddr | leftAddr);

        mutex_mMapClientBufServerBuf.lock();
        mMapClientBufServerBuf_.insert(std::make_pair(reinterpret_cast<uint64_t>(newBuffer), std::make_pair(serverBuffer, tmpBuffer)));
        mutex_mMapClientBufServerBuf.unlock();
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Release a buffer allocated by Eden framework
 *  @details This function releases a buffer returned by AllocateXXXBuffers.
 *  @param[in] modelId The model id to be applied by.
 *  @param[in] buffers Buffer pointer allocated by AllocateXXXBuffers
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::FreeBuffers(uint32_t modelId, EdenBuffer* buffers) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (buffers == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    mutex_mMapClientBufServerBuf.lock();
    auto it = mMapClientBufServerBuf_.find(reinterpret_cast<uint64_t>(buffers));
    if (it == mMapClientBufServerBuf_.end()) {
        LOGE(EDEN_DRIVER, "failed to find buffers!\n");
        return FAIL_TO_FIND_KEY_IN_MAP;
    }
    mutex_mMapClientBufServerBuf.unlock();
    auto serverBuf = it->second.first;

    RtRet result = RT_SUCCESS;
    int32_t numOfEdenBuffers = 0;
    std::function<void(uint32_t, int32_t)> CBFunc = [&result, &numOfEdenBuffers](uint32_t rtRet, int32_t numOfBuffers) {
        if (rtRet = RT_SUCCESS) {
            numOfEdenBuffers = numOfBuffers;
        } else {
            result = (RtRet)rtRet;
        }
    };

    mutexService.lock();
    service->freeBuffer(pid, modelId, serverBuf, CBFunc);
    mutexService.unlock();

    if (result != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "FreeBuffers() is failed!\n");
        return FAIL_TO_FREE_MEM;
    } else {
        for (int idx = 0; idx < numOfEdenBuffers; idx++) {
            EdenBuffer* edenBuffer = &(buffers[idx]);
            munmap(edenBuffer->addr, edenBuffer->size);
            LOGD(EDEN_DRIVER, "free buffer unmap size: %d\n", edenBuffer->size);
        }
    }

    mutex_mMapClientBufServerBuf.lock();
    mMapClientBufServerBuf_.erase(reinterpret_cast<uint64_t>(buffers));
    mutex_mMapClientBufServerBuf.unlock();
    delete[] buffers;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Get Processing Units state
 *  @details This API function returns state of CPU/GPU/NPU.
 *  @param[out] state
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::GetState(EdenState* state) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (state == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    mutexService.lock();
    service->getState(state);
    mutexService.unlock();

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
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
int32_t EdenServiceDelegatorService::GetInputBufferShape(uint32_t modelId, int32_t inputIndex, int32_t* width, int32_t* height,
        int32_t* channel, int32_t* number) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (width == nullptr || height == nullptr || channel == nullptr || number == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, << "modelId : %u, inputIndex : %d, width : %d, height : %d, channel : %d, number : %d\n", modelId, inputIndex, *width, *height, *channel, *number);

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    RtRet result = RT_SUCCESS;
    std::function<void(uint32_t, int32_t, int32_t, int32_t, int32_t)> CBFunc = [&width, &height, &channel, &number, &result](
        uint32_t rtRet, int32_t inputWidth, int32_t inputHeight, int32_t inputChannel, int32_t inputNumber) {
            if (rtRet == 0) {
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
    };

    mutexService.lock();
    service->getInputBufferShape(pid, modelId, inputIndex, CBFunc);
    mutexService.unlock();

    if (result != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "EdenServiceDelegatorService::GetInputBufferShape() is failed : %d\n", result);
        return FAIL_TO_GET_INPUT_MEM_SHAPE;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
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
int32_t EdenServiceDelegatorService::GetOutputBufferShape(uint32_t modelId, int32_t outputIndex, int32_t* width, int32_t* height,
        int32_t* channel, int32_t* number) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (width == nullptr || height == nullptr || channel == nullptr || number == nullptr) {
        LOGE(EDEN_DRIVER, "Invalied Params.\n");
        return INVALID_PARAMS;
    }
    LOGD(EDEN_DRIVER, << "modelId : %u, outputIndex : %d, width : %d, height : %d, channel : %d, number : %d\n", modelId, outputIndex, *width, *height, *channel, *number);

    pid_t pid = getpid();
    LOGD(EDEN_DRIVER, "NNHAL pid : %d\n", pid);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();
    if (service == nullptr) {
        LOGE(EDEN_DRIVER, "getService() is failed!\n");
        return FAIL_TO_GET_SERVICE;
    }

    RtRet result = RT_SUCCESS;
    std::function<void(uint32_t, int32_t, int32_t, int32_t, int32_t)> CBFunc = [&width, &height, &channel, &number, &result](
        uint32_t rtRet, int32_t outputWidth, int32_t outputHeight, int32_t outputChannel, int32_t outputNumber) {
            if (rtRet == 0) {
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
    };

    mutexService.lock();
    service->getOutputBufferShape(pid, modelId, outputIndex, CBFunc);
    mutexService.unlock();

    if (result != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "EdenServiceDelegatorService::GetOutputBufferShape() is failed : %d\n", result);
        return FAIL_TO_GET_OUTPUT_MEM_SHAPE;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 *  @brief Get ncp buffer from EdenManager
 *  @details This function gets ncp buffer to release ncp buffer
 *  @param[in] modelId
 *  @param[out] ncp buffer
 *  @returns return code
 */
int32_t EdenServiceDelegatorService::GetNcpBuffer(uint32_t modelId, void*& ncpBuffer) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    IEdenruntime* service = nullptr;
    service = IEdenruntime::getService();

    /* TODO : This function is not used now. GetNcpBuffer should be implemented at IEdenruntime when Service is enabled.
    RtRet ret = service->GetNcpBuffer(modelId, edenModel);
    if (ret != RT_SUCCESS) {
        LOGE(EDEN_DRIVER, "eden::rt::GetEdenModel() fails(%d).\n", ret);
        return ret;
    }
    */
    return RET_OK;
}


}  // namespace eden_driver
}  // namespace nn
}  // namespace android

