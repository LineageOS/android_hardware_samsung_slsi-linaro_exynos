/*
 * Copyright (c) 2022, Samsung Electronics Co. Ltd All Rights Reserved
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
#define LOG_TAG "exynos_bootctrl"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <log/log.h>
#include <memory>

#include <fstab/fstab.h>
#include <hardware/hardware.h>
#include <hardware/boot_control.h>
#include <android-base/properties.h>
#include <android-base/logging.h>
#include "BootControl.h"

namespace android {
namespace hardware {
namespace samsung_slsi {
namespace boot_control {
namespace V1_2 {
namespace implementation {

using ::android::hardware::boot::V1_0::CommandResult;

bool BootControl::Init() {
    return impl_.Init();
}

// Methods from ::android::hardware::boot::V1_0::IBootControl follow.
Return<uint32_t> BootControl::getNumberSlots() {
    return impl_.GetNumberSlots();
}

Return<uint32_t> BootControl::getCurrentSlot() {
    return impl_.GetCurrentSlot();
}

Return<void> BootControl::markBootSuccessful(markBootSuccessful_cb _hidl_cb) {
    struct CommandResult cr;
	uint32_t slot = impl_.GetCurrentSlot();
    if (impl_.MarkBootSuccessful(slot)) {
        cr.success = true;
        cr.errMsg = "Success";
    } else {
        cr.success = false;
        cr.errMsg = "Operation failed";
    }
    _hidl_cb(cr);
    return Void();
}

Return<void> BootControl::setActiveBootSlot(uint32_t slot, setActiveBootSlot_cb _hidl_cb) {
    struct CommandResult cr;
    if (impl_.SetActiveBootSlot(slot)) {
        cr.success = true;
        cr.errMsg = "Success";
    } else {
        cr.success = false;
        cr.errMsg = "Operation failed";
    }
    _hidl_cb(cr);
    return Void();
}

Return<void> BootControl::setSlotAsUnbootable(uint32_t slot, setSlotAsUnbootable_cb _hidl_cb) {
    struct CommandResult cr;
    if (impl_.SetSlotAsUnbootable(slot)) {
        cr.success = true;
        cr.errMsg = "Success";
    } else {
        cr.success = false;
        cr.errMsg = "Operation failed";
    }
    _hidl_cb(cr);
    return Void();
}

Return<BoolResult> BootControl::isSlotBootable(uint32_t slot) {
    if (!impl_.IsValidSlot(slot)) {
        return BoolResult::INVALID_SLOT;
    }
    return impl_.IsSlotBootable(slot) ? BoolResult::TRUE : BoolResult::FALSE;
}

Return<BoolResult> BootControl::isSlotMarkedSuccessful(uint32_t slot) {
    if (!impl_.IsValidSlot(slot)) {
        return BoolResult::INVALID_SLOT;
    }
    return impl_.IsSlotMarkedSuccessful(slot) ? BoolResult::TRUE : BoolResult::FALSE;
}

Return<void> BootControl::getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) {
    hidl_string ans;
    const char* suffix = impl_.GetSuffix(slot);
    if (suffix) {
        ans = suffix;
    }
    _hidl_cb(ans);
    return Void();
}

Return<bool> BootControl::setSnapshotMergeStatus(MergeStatus status) {
    return impl_.SetSnapshotMergeStatus(status);
}

Return<MergeStatus> BootControl::getSnapshotMergeStatus() {
    return impl_.GetSnapshotMergeStatus();
}

Return<uint32_t> BootControl::getActiveBootSlot() {
    return impl_.GetActiveBootSlot();
}

IBootControl* HIDL_FETCH_IBootControl(const char* /* hal */) {
    auto module = std::make_unique<BootControl>();
    if (!module->Init()) {
        ALOGE("Could not initialize BootControl module");
        return nullptr;
    }
    return module.release();
}

} // namespace implement
} // namespace V1_2
} // namespace boot_control
} // namespace samsung_slsi
} // namespace bootable
} // namespace android
