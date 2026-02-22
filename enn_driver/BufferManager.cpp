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

#include "../include/log.h"
#include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "BufferManager.h"
#include "Common.h"  // RET_OK

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::BufferManager"

namespace android {
namespace nn {
namespace eden_driver {

std::shared_ptr<ManagedBuffer> ManagedBuffer::create(EdenBuffer* buffer,
                                                     uint32_t size,
                                                     std::set<HalPreparedModelRole> roles,
                                                     const V1_3::Operand& operand) {
    return std::make_shared<ManagedBuffer>(buffer, size, roles, operand);
}

/**
 * @brief ManagedBuffer constructor
 * @details This class manages allocated device buffer
 * @param[in] buffer allocated EdenBuffer
 * @param[in] size size of allocated buffer
 * @param[in] roles inputroles/outputroles given in allocate() API
 * @param[in] operand corresponding operand
 * @returns return code
 */
ManagedBuffer::ManagedBuffer(EdenBuffer* buffer,
                              uint32_t size,
                              std::set<HalPreparedModelRole> roles,
                              const V1_3::Operand& operand):
    mBuffer(buffer),
    kSize(size),
    kRoles(std::move(roles)),
    kOperandType(operand.type),
    kInitialDimensions(operand.dimensions),
    mUpdatedDimensions(operand.dimensions) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
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
                                       const V1_3::Request& request,
                                       const V1_3::IPreparedModel* preparedModel) const {
    std::lock_guard<std::mutex> guard(mMutex);

    bool usedAsInput = false, usedAsOutput = false;
    for (uint32_t i = 0; i < request.inputs.size(); i++) {
        if (request.inputs[i].hasNoValue) continue;
        if (request.inputs[i].location.poolIndex != poolIndex) continue;

        /* Validate if the input role is specified during allocation. */
        if (kRoles.count({preparedModel, IOType::INPUT, i}) == 0) {
            LOGE(EDEN_DRIVER, "validateRequest: invalid buffer role.\n");
            return INVALID_PARAMS;
        }
        if (!mInitialized) {
            LOGE(EDEN_DRIVER, "validateRequest: using uninitialized buffer as input request.\n");
            return GENERAL_FAILURE;
        }
        auto combined = combineDimensions(mUpdatedDimensions, request.inputs[i].dimensions);
        if (!combined.has_value()) {
            LOGE(EDEN_DRIVER, "validateRequest: incompatible input dimensions.\n");
               return INVALID_PARAMS;
        }
        usedAsInput = true;
    }

    for (uint32_t i = 0; i < request.outputs.size(); i++) {
        if (request.outputs[i].hasNoValue) continue;
        if (request.outputs[i].location.poolIndex != poolIndex) continue;
        if (usedAsInput || usedAsOutput) {
            LOGE(EDEN_DRIVER, "validateRequest: using the same device memory for input/output or multiple outputs.\n");
            return INVALID_PARAMS;
        }
        /* Validate if the output role is specified during allocation.*/
        if (kRoles.count({preparedModel, IOType::OUTPUT, i}) == 0) {
            LOGE(EDEN_DRIVER, "validateRequest: invalid buffer role.\n");
            return INVALID_PARAMS;
        }

        auto combined = combineDimensions(kInitialDimensions, request.outputs[i].dimensions);
        if (!combined.has_value()) {
            LOGE(EDEN_DRIVER, "validateRequest: incompatible output dimensions.\n");
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
V1_3::ErrorStatus ManagedBuffer::validateCopyTo(uint32_t size) const {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (size != kSize) {
        LOGE(EDEN_DRIVER, "validateCopyTo: Invalid memory size :  [%d] vs [%d]\n", kSize, size);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> guard(mMutex);
    if (!mInitialized) {
        LOGE(EDEN_DRIVER, "validateCopyTo: using uninitialized buffer as source.\n");
        return V1_3::ErrorStatus::GENERAL_FAILURE;
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_3::ErrorStatus::NONE;
}

/**
 * @brief validates input argumnets for copyFrom
 * @details This functions validates input argumnets of copyFrom() API
 * @param[in] dimensions dimensions provided in input
 * @param[in] size size provided in input
 * @returns ErrorStatus
 */
V1_3::ErrorStatus ManagedBuffer::validateCopyFrom(const std::vector<uint32_t>& dimensions,
                                                       uint32_t size) const {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (size != kSize) {
        LOGE(EDEN_DRIVER, "validateCopyFrom: Invalid memory size: [%d] vs [%d]\n", kSize, size);
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }
    if (nonExtensionOperandTypeIsScalar(static_cast<int>(kOperandType))) {
        if (!dimensions.empty()) {
            LOGE(EDEN_DRIVER, "validatecopyFrom: invalid dimensions for scaler operand.\n");
            return V1_3::ErrorStatus::INVALID_ARGUMENT;
        }
    }

    if (dimensions.empty()) {
        if (tensorHasUnspecifiedDimensions(kOperandType, kInitialDimensions)) {
            LOGE(EDEN_DRIVER, "validateCopyFrom: No dimension update is provided.\n");
            return V1_3::ErrorStatus::INVALID_ARGUMENT;
        }
    } else {
        if (tensorHasUnspecifiedDimensions(kOperandType, dimensions)) {
            LOGE(EDEN_DRIVER, "validateCopyFrom: the updated dimensions are not fully specified.\n");
            return V1_3::ErrorStatus::INVALID_ARGUMENT;
         }
    }

    const auto combined = combineDimensions(kInitialDimensions, dimensions);
    if (!combined.has_value()) {
        LOGE(EDEN_DRIVER, "validateCopyFrom:incompatible dimensions\n");
        return V1_3::ErrorStatus::INVALID_ARGUMENT;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return V1_3::ErrorStatus::NONE;
}

bool ManagedBuffer::updateDimensions(const std::vector<uint32_t>& dimensions) {
    auto combined = combineDimensions(kInitialDimensions, dimensions);
    if (!combined.has_value()) {
        LOGE(EDEN_DRIVER, "updateDimensions: incompatible dimensions.\n");
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

EdenBuffer* ManagedBuffer::getBuffer() const {
    std::lock_guard<std::mutex> guard(mMutex);
    return mBuffer;
}

/**
 * @brief add a token for the buffer
 * @details This functions associates an unique token for the allocated device buffer
 * @param[in] buffer allocated device buffer
 * @returns integer token
 */
std::unique_ptr<BufferTracker::Token> BufferTracker::add(std::shared_ptr<ManagedBuffer> buffer) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (buffer == nullptr) {
        return nullptr;
    }
    std::lock_guard<std::mutex> guard(mMutex);
    uint32_t token = 0;

    if (mFreeTokens.empty()) {
        token = mTokenToBuffers.size();
        mTokenToBuffers.push_back(std::move(buffer));
    } else {
        token = mFreeTokens.top();
        mFreeTokens.pop();
        mTokenToBuffers[token] = std::move(buffer);
    }
    LOGD(EDEN_DRIVER, "BufferTracker::add -- new token =%d \n", token);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return std::make_unique<Token>(token, shared_from_this());
}

/**
 * @brief gets a buffer for the token
 * @details This functions get the corrosponding buffer associated with the given token
 * @param[in] token token received in argument
 * @returns ManagedBuffer
 */
std::shared_ptr<ManagedBuffer> BufferTracker::get(uint32_t token) const {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    std::lock_guard<std::mutex> guard(mMutex);
    if (mTokenToBuffers.size() <= token || mTokenToBuffers[token] == nullptr) {
        LOGE(EDEN_DRIVER, "BufferTracker::get -- unknown token %d\n", token);
        return nullptr;
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return mTokenToBuffers[token];
}

/**
 * @brief free a token for the buffer
 * @details This functions disassociates a token for the allocated device buffer
 * @param[in] token
 * @returns None
 */
void BufferTracker::free(uint32_t token) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    std::lock_guard<std::mutex> guard(mMutex);
    if (token >= mTokenToBuffers.size()) {
        LOGE(EDEN_DRIVER, "BufferTracker will not be free\n");
        LOGE(EDEN_DRIVER, "There is no token(%d) in mTokenToBuffer.size(%lu)\n", token, mTokenToBuffers.size());
        return;
    }
    if (mTokenToBuffers[token] == nullptr) {
        LOGE(EDEN_DRIVER, "BufferTracker will not be free\n");
        LOGE(EDEN_DRIVER, "mTokenToBuffer[token(%d)] is null", token);
        return;
    }
    LOGD(EDEN_DRIVER, "BufferTracker::free -- release token = %d\n", token);
    mTokenToBuffers[token] = nullptr;
    mFreeTokens.push(token);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

