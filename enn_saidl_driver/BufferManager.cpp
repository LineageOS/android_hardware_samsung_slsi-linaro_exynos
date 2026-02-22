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
 * @file    BufferManager.cpp
 * @brief   This is ManagedBuffer class file.
 * @details This header defines ResourceManager class.
 *          This class is implementing resource managing such as caching etc.
 * @author
 */

#include <iostream>
#include <memory>
#include <vector>
#include <mutex>

#include "log.h"
#include "LegacyUtils.h"
#include "BufferManager.h"
#include "Common.h"  // RET_OK

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EnnDriver::BufferManager"

namespace android {
namespace nn {
namespace enn_driver {

std::shared_ptr<ManagedBuffer> ManagedBuffer::create(EnnBuffer* buffer,
                                                     uint32_t size,
                                                     std::set<PreparedModelRole> roles,
                                                     const Operand& operand) {
    return std::make_shared<ManagedBuffer>(buffer, size, roles, operand);
}

/**
 * @brief ManagedBuffer constructor
 * @details This class manages allocated device buffer
 * @param[in] buffer allocated EnnBuffer
 * @param[in] size size of allocated buffer
 * @param[in] roles inputroles/outputroles given in allocate() API
 * @param[in] operand corresponding operand
 * @returns return code
 */
ManagedBuffer::ManagedBuffer(EnnBuffer* buffer,
                              uint32_t size,
                              std::set<PreparedModelRole> roles,
                              const Operand& operand):
    mBuffer(buffer),
    kSize(size),
    kRoles(std::move(roles)),
    kOperandType(operand.type),
    kInitialDimensions(operand.dimensions),
    mUpdatedDimensions(operand.dimensions) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief validates input argumnets of execution request
 * @details This functions validates input argumnets of execution request
 * @param[in] poolIndex index of the device memory pool
 * @param[in] request execution request
 * @param[in] preparedModel corresponding prepared model
 * @returns Error code
 */
int32_t ManagedBuffer::validateRequest(uint32_t poolIndex,
                                       const Request& request,
                                       const IPreparedModel* preparedModel) const {
    std::lock_guard<std::mutex> guard(mMutex);

    bool usedAsInput = false, usedAsOutput = false;
    for (uint32_t i = 0; i < request.inputs.size(); i++) {
        if (request.inputs[i].lifetime != Request::Argument::LifeTime::POOL) continue;
        if (request.inputs[i].location.poolIndex != poolIndex) continue;

        /* Validate if the input role is specified during allocation. */
        if (kRoles.count({preparedModel, IOType::INPUT, i}) == 0) {
            LOGE(ENN_DRIVER, "validateRequest: invalid buffer role.\n");
            return INVALID_PARAMS;
        }
        if (!mInitialized) {
            LOGE(ENN_DRIVER, "validateRequest: using uninitialized buffer as input request.\n");
            return GENERAL_FAILURE;
        }
        auto combined = combineDimensions(mUpdatedDimensions, request.inputs[i].dimensions);
        if (!combined.has_value()) {
            LOGE(ENN_DRIVER, "validateRequest: incompatible input dimensions.\n");
               return INVALID_PARAMS;
        }
        usedAsInput = true;
    }

    for (uint32_t i = 0; i < request.outputs.size(); i++) {
        if (request.outputs[i].lifetime != Request::Argument::LifeTime::POOL) continue;
        if (request.outputs[i].location.poolIndex != poolIndex) continue;
        if (usedAsInput || usedAsOutput) {
            LOGE(ENN_DRIVER, "validateRequest: using the same device memory for input/output or multiple outputs.\n");
            return INVALID_PARAMS;
        }
        /* Validate if the output role is specified during allocation.*/
        if (kRoles.count({preparedModel, IOType::OUTPUT, i}) == 0) {
            LOGE(ENN_DRIVER, "validateRequest: invalid buffer role.\n");
            return INVALID_PARAMS;
        }

        auto combined = combineDimensions(kInitialDimensions, request.outputs[i].dimensions);
        if (!combined.has_value()) {
            LOGE(ENN_DRIVER, "validateRequest: incompatible output dimensions.\n");
            return INVALID_PARAMS;
        }
        usedAsOutput = true;
    }
    return RET_OK;
}

/**
 * @brief validates input argumnets for copyTo
 * @details This functions validates input argumnets of copyTo() API
 * @param[in] size size provided in input
 * @returns ErrorStatus
 */
ErrorStatus ManagedBuffer::validateCopyTo(uint32_t size) const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    if (size != kSize) {
        LOGE(ENN_DRIVER, "validateCopyTo: Invalid memory size :  [%d] vs [%d]\n", kSize, size);
        return ErrorStatus::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> guard(mMutex);
    if (!mInitialized) {
        LOGE(ENN_DRIVER, "validateCopyTo: using uninitialized buffer as source.\n");
        return ErrorStatus::GENERAL_FAILURE;
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return ErrorStatus::NONE;
}

/**
 * @brief validates input argumnets for copyFrom
 * @details This functions validates input argumnets of copyFrom() API
 * @param[in] dimensions dimensions provided in input
 * @param[in] size size provided in input
 * @returns ErrorStatus
 */
ErrorStatus ManagedBuffer::validateCopyFrom(const std::vector<uint32_t>& dimensions,
                                                       uint32_t size) const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    if (size != kSize) {
        LOGE(ENN_DRIVER, "validateCopyFrom: Invalid memory size: [%d] vs [%d]\n", kSize, size);
        return ErrorStatus::INVALID_ARGUMENT;
    }
    if (nonExtensionOperandTypeIsScalar(static_cast<int>(kOperandType))) {
        if (!dimensions.empty()) {
            LOGE(ENN_DRIVER, "validatecopyFrom: invalid dimensions for scaler operand.\n");
            return ErrorStatus::INVALID_ARGUMENT;
        }
    }

    if (dimensions.empty()) {
        if (tensorHasUnspecifiedDimensions(kOperandType, kInitialDimensions)) {
            LOGE(ENN_DRIVER, "validateCopyFrom: No dimension update is provided.\n");
            return ErrorStatus::INVALID_ARGUMENT;
        }
    } else {
        if (tensorHasUnspecifiedDimensions(kOperandType, dimensions)) {
            LOGE(ENN_DRIVER, "validateCopyFrom: the updated dimensions are not fully specified.\n");
            return ErrorStatus::INVALID_ARGUMENT;
         }
    }

    const auto combined = combineDimensions(kInitialDimensions, dimensions);
    if (!combined.has_value()) {
        LOGE(ENN_DRIVER, "validateCopyFrom:incompatible dimensions\n");
        return ErrorStatus::INVALID_ARGUMENT;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return ErrorStatus::NONE;
}

bool ManagedBuffer::updateDimensions(const std::vector<uint32_t>& dimensions) {
    auto combined = combineDimensions(kInitialDimensions, dimensions);
    if (!combined.has_value()) {
        LOGE(ENN_DRIVER, "updateDimensions: incompatible dimensions.\n");
        return false;
    }
    std::lock_guard<std::mutex> guard(mMutex);
    mUpdatedDimensions = std::move(combined.value());
    return true;
}


void ManagedBuffer::setInitialized(bool initialized) {
    std::lock_guard<std::mutex> guard(mMutex);
    mInitialized = initialized;
}

EnnBuffer* ManagedBuffer::getBuffer() const {
    std::lock_guard<std::mutex> guard(mMutex);
    return mBuffer;
}

BufferTracker::BufferTracker() {
    constexpr size_t kPreallocatedElements = 1024;
    using StackSpace = std::vector<Request::MemoryDomainToken>;
    using Stack = std::stack<Request::MemoryDomainToken, StackSpace>;
    StackSpace stackSpace;
    stackSpace.reserve(kPreallocatedElements);
    mFreeTokens = Stack(std::move(stackSpace));
    mTokenToBuffers.reserve(kPreallocatedElements);
    mTokenToBuffers.emplace_back();
}
/**
 * @brief add a token for the buffer
 * @details This functions associates an unique token for the allocated device buffer
 * @param[in] buffer allocated device buffer
 * @returns integer token
 */
std::unique_ptr<BufferTracker::Token> BufferTracker::add(std::shared_ptr<ManagedBuffer> buffer) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    if (buffer == nullptr) {
        return nullptr;
    }
    std::lock_guard<std::mutex> guard(mMutex);
    auto  token = Request::MemoryDomainToken{0};

    if (mFreeTokens.empty()) {
        token = static_cast<Request::MemoryDomainToken>( mTokenToBuffers.size());
        mTokenToBuffers.push_back(std::move(buffer));
    } else {
        token = mFreeTokens.top();
        mFreeTokens.pop();
        const auto index = static_cast<uint32_t>(token);
	mTokenToBuffers[index] = std::move(buffer);
    }
    LOGD(ENN_DRIVER, "BufferTracker::add -- new token =%d \n", static_cast<uint32_t>(token));
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return std::make_unique<Token>(token, shared_from_this());
}

/**
 * @brief gets a buffer for the token
 * @details This functions get the corrosponding buffer associated with the given token
 * @param[in] token token received in argument
 * @returns ManagedBuffer
 */
std::shared_ptr<ManagedBuffer> BufferTracker::get(Request::MemoryDomainToken token) const {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    std::lock_guard<std::mutex> guard(mMutex);
    const auto index = static_cast<uint32_t>(token);
    if (mTokenToBuffers.size() <= index || mTokenToBuffers[index] == nullptr) {
        LOGE(ENN_DRIVER, "BufferTracker::get -- unknown token %d\n", token);
        return nullptr;
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return mTokenToBuffers[index];
}

/**
 * @brief free a token for the buffer
 * @details This functions disassociates a token for the allocated device buffer
 * @param[in] token
 * @returns None
 */
void BufferTracker::free(Request::MemoryDomainToken token) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);
    std::lock_guard<std::mutex> guard(mMutex);
    const auto index = static_cast<uint32_t>(token);
    if (index >= mTokenToBuffers.size()) {
        LOGE(ENN_DRIVER, "BufferTracker will not be free\n");
        LOGE(ENN_DRIVER, "There is no token(%d) in mTokenToBuffer.size(%lu)\n", token, mTokenToBuffers.size());
        return;
    }
    if (mTokenToBuffers[index] == nullptr) {
        LOGE(ENN_DRIVER, "BufferTracker will not be free\n");
        LOGE(ENN_DRIVER, "mTokenToBuffer[token(%d)] is null", token);
        return;
    }
    LOGD(ENN_DRIVER, "BufferTracker::free -- release token = %d\n", token);
    mTokenToBuffers[index] = nullptr;
    mFreeTokens.push(token);
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

