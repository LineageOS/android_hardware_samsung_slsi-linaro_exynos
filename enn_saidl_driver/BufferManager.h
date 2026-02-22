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
 * @file    BufferManager.h
 * @brief   This is BufferManager class file.
 * @details This header defines ResourceManager class.
 *          This class is implementing functions for device managed buffer.
 */

#ifndef DRIVER_BUFFERMANAGER_H_
#define DRIVER_BUFFERMANAGER_H_

#include <map>
#include <cstdint>  // int32_t
#include <stack>
#include <set>
#include <memory>

#include "nnapi/TypeUtils.h"
#include "nnapi/Validation.h"
#include "../enn/include/enn_api-type.h"

namespace android {
namespace nn {
namespace enn_driver {

class ManagedBuffer {
 public:
    static std::shared_ptr<ManagedBuffer> create(EnnBuffer* buffer,
                                                 uint32_t size,
                                                 std::set<PreparedModelRole> roles,
                                                 const Operand& operand);
    ManagedBuffer(EnnBuffer* buffer, uint32_t size,
                    std::set<PreparedModelRole> roles, const Operand& operand);

    int32_t validateRequest(uint32_t poolIndex,
                            const Request& request,
                            const IPreparedModel* preparedModel) const;

    // "size" is the byte size of the hidl_memory provided to the copyFrom or copyTo method.
    ErrorStatus validateCopyFrom(const std::vector<uint32_t>& dimensions, uint32_t size) const;
    ErrorStatus validateCopyTo(uint32_t size) const;

    EnnBuffer* getBuffer() const;
    bool updateDimensions(const std::vector<uint32_t>& dimensions);
    void setInitialized(bool initialized);

 private:
    mutable std::mutex mMutex;
    EnnBuffer* mBuffer;
    const uint32_t kSize;
    const std::set<PreparedModelRole> kRoles;
    const OperandType kOperandType;
    const std::vector<uint32_t> kInitialDimensions;
    std::vector<uint32_t> mUpdatedDimensions;
    bool mInitialized = false;
};

/*Keep track of all ManagedBuffers and assign each with a unique token.*/
class BufferTracker : public std::enable_shared_from_this<BufferTracker>  {
 public:
    // A RAII class to help manage the lifetime of the token.
    // It is only supposed to be constructed in BufferTracker::add.
    class Token {
     public:
        Token(Request::MemoryDomainToken token, std::shared_ptr<BufferTracker> tracker)
            : kToken(token), kBufferTracker(std::move(tracker)) {}
        ~Token() { kBufferTracker->free(kToken); }
	Request::MemoryDomainToken get() const { return kToken; }

     private:
        const Request::MemoryDomainToken kToken;
        const std::shared_ptr<BufferTracker> kBufferTracker;
    };

    // The factory of BufferTracker. This ensures that the BufferTracker is always managed by a
    // shared_ptr.
    static std::shared_ptr<BufferTracker> create() { return std::make_shared<BufferTracker>(); }

    // Prefer BufferTracker::create.
    BufferTracker();

    std::unique_ptr<Token> add(std::shared_ptr<ManagedBuffer> buffer);
    std::shared_ptr<ManagedBuffer> get(Request::MemoryDomainToken token) const;

 private:
    void free(Request::MemoryDomainToken token);

    mutable std::mutex mMutex;
    std::stack<Request::MemoryDomainToken, std::vector<Request::MemoryDomainToken>> mFreeTokens;

    // Since the tokens are allocated in a non-sparse way, we use a vector to represent the mapping.
    // The index of the vector is the token. When the token gets freed, the corresponding entry is
    // set to nullptr. mTokenToBuffers[0] is always set to nullptr because 0 is an invalid token.
    std::vector<std::shared_ptr<ManagedBuffer>> mTokenToBuffers;
};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_BUFFERMANAGER_H_


