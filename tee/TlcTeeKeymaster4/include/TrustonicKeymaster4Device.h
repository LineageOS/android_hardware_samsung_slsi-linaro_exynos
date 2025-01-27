/*
 * Copyright (c) 2018-2020 TRUSTONIC LIMITED
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the TRUSTONIC LIMITED nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TRUSTONIC_KEYMASTER4_DEVICE_H
#define TRUSTONIC_KEYMASTER4_DEVICE_H

#include "TrustonicKeymaster4DeviceImpl.h"

#if KEYMASTER_WANTED_VERSION == 4
#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>
#else
#include <android/hardware/keymaster/3.0/IKeymasterDevice.h>
#endif
#include <hidl/Status.h>

#if KEYMASTER_WANTED_VERSION == 4
#define HIDL_VER V4_0
#else
#define HIDL_VER V3_0
#endif

#include <hidl/MQDescriptor.h>
namespace android {
namespace hardware {
namespace keymaster {
namespace HIDL_VER {
namespace implementation {

using ::android::hardware::keymaster::HIDL_VER::ErrorCode;
using ::android::hardware::keymaster::HIDL_VER::HardwareAuthToken;
#if KEYMASTER_WANTED_VERSION == 4
using ::android::hardware::keymaster::HIDL_VER::HmacSharingParameters;
#endif
using ::android::hardware::keymaster::HIDL_VER::IKeymasterDevice;
using ::android::hardware::keymaster::HIDL_VER::KeyCharacteristics;
using ::android::hardware::keymaster::HIDL_VER::KeyFormat;
using ::android::hardware::keymaster::HIDL_VER::KeyParameter;
using ::android::hardware::keymaster::HIDL_VER::KeyPurpose;
using ::android::hardware::keymaster::HIDL_VER::SecurityLevel;
#if KEYMASTER_WANTED_VERSION == 4
using ::android::hardware::keymaster::HIDL_VER::VerificationToken;
#endif
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;

class TrustonicKeymaster4Device : public IKeymasterDevice {
  public:
    TrustonicKeymaster4Device(TrustonicKeymaster4DeviceImpl *impl)
        : impl_(impl) {}
    virtual ~TrustonicKeymaster4Device();

    // Methods from ::android::hardware::keymaster::V4_0::IKeymasterDevice follow.
#if KEYMASTER_WANTED_VERSION == 4
    Return<void> getHardwareInfo(
        getHardwareInfo_cb _hidl_cb);
    Return<void> getHmacSharingParameters(
        getHmacSharingParameters_cb _hidl_cb);
    Return<void> computeSharedHmac(
        const hidl_vec<HmacSharingParameters>& params,
        computeSharedHmac_cb _hidl_cb);
    Return<void> verifyAuthorization(
        uint64_t operationHandle,
        const hidl_vec<KeyParameter>& parametersToVerify,
        const HardwareAuthToken& authToken,
        verifyAuthorization_cb _hidl_cb);
#else
    Return<void> getHardwareFeatures(
        getHardwareFeatures_cb _hidl_cb);
#endif
    Return<ErrorCode> addRngEntropy(
        const hidl_vec<uint8_t>& data);
    Return<void> generateKey(
        const hidl_vec<KeyParameter>& keyParams,
        generateKey_cb _hidl_cb);
    Return<void> importKey(
        const hidl_vec<KeyParameter>& keyParams,
        KeyFormat keyFormat,
        const hidl_vec<uint8_t>& keyData,
        importKey_cb _hidl_cb);
#if KEYMASTER_WANTED_VERSION == 4
    Return<void> importWrappedKey(
        const hidl_vec<uint8_t>& wrappedKeyData,
        const hidl_vec<uint8_t>& wrappingKeyBlob,
        const hidl_vec<uint8_t>& maskingKey,
        const hidl_vec<KeyParameter>& unwrappingParams,
        uint64_t passwordSid,
        uint64_t biometricSid,
        importWrappedKey_cb _hidl_cb);
#endif
    Return<void> getKeyCharacteristics(
        const hidl_vec<uint8_t>& keyBlob,
        const hidl_vec<uint8_t>& clientId,
        const hidl_vec<uint8_t>& appData,
        getKeyCharacteristics_cb _hidl_cb);
    Return<void> exportKey(
        KeyFormat keyFormat,
        const hidl_vec<uint8_t>& keyBlob,
        const hidl_vec<uint8_t>& clientId,
        const hidl_vec<uint8_t>& appData,
        exportKey_cb _hidl_cb);
    Return<void> attestKey(
        const hidl_vec<uint8_t>& keyToAttest,
        const hidl_vec<KeyParameter>& attestParams,
        attestKey_cb _hidl_cb);
    Return<void> upgradeKey(
        const hidl_vec<uint8_t>& keyBlobToUpgrade,
        const hidl_vec<KeyParameter>& upgradeParams,
        upgradeKey_cb _hidl_cb);
    Return<ErrorCode> deleteKey(
        const hidl_vec<uint8_t>& keyBlob);
    Return<ErrorCode> deleteAllKeys();
    Return<ErrorCode> destroyAttestationIds();
    Return<void> begin(
        KeyPurpose purpose,
        const hidl_vec<uint8_t>& keyBlob,
        const hidl_vec<KeyParameter>& inParams,
#if KEYMASTER_WANTED_VERSION == 4
        const HardwareAuthToken& authToken,
#endif
        begin_cb _hidl_cb);
    Return<void> update(
        uint64_t operationHandle,
        const hidl_vec<KeyParameter>& inParams,
        const hidl_vec<uint8_t>& input,
#if KEYMASTER_WANTED_VERSION == 4
        const HardwareAuthToken& authToken,
        const VerificationToken& verificationToken,
#endif
        update_cb _hidl_cb);
    Return<void> finish(
        uint64_t operationHandle,
        const hidl_vec<KeyParameter>& inParams,
        const hidl_vec<uint8_t>& input,
        const hidl_vec<uint8_t>& signature,
#if KEYMASTER_WANTED_VERSION == 4
        const HardwareAuthToken& authToken,
        const VerificationToken& verificationToken,
#endif
        finish_cb _hidl_cb);
    Return<ErrorCode> abort(
        uint64_t operationHandle);

  private:
    TrustonicKeymaster4DeviceImpl *impl_;
};

extern "C" IKeymasterDevice* HIDL_FETCH_IKeymasterDevice(const char* name);

}  // namespace implementation
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

#endif  // TRUSTONIC_KEYMASTER4_DEVICE_H
