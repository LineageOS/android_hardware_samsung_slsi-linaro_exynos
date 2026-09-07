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

#ifndef BOOT_CONTROL_H_
#define BOOT_CONTROL_H_

#include <stdint.h>
#include <stdbool.h>

#include <android/hardware/boot/1.2/IBootControl.h>
#include <exynos_bootctrl/exynos_bootctrl.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace samsung_slsi {
namespace boot_control {
namespace V1_2 {
namespace implementation {

using android::fs_mgr::Fstab;
using android::fs_mgr::ReadDefaultFstab;

using ::android::hardware::boot::V1_0::BoolResult;
using ::android::hardware::boot::V1_1::MergeStatus;
using ::android::hardware::boot::V1_2::IBootControl;

class BootControl : public IBootControl {
  public:
    bool Init();

    // Methods from ::android::hardware::boot::V1_0::IBootControl follow.
    Return<uint32_t> getNumberSlots() override;
    Return<uint32_t> getCurrentSlot() override;
    Return<void> markBootSuccessful(markBootSuccessful_cb _hidl_cb) override;
    Return<void> setActiveBootSlot(uint32_t slot, setActiveBootSlot_cb _hidl_cb) override;
    Return<void> setSlotAsUnbootable(uint32_t slot, setSlotAsUnbootable_cb _hidl_cb) override;
    Return<BoolResult> isSlotBootable(uint32_t slot) override;
    Return<BoolResult> isSlotMarkedSuccessful(uint32_t slot) override;
    Return<void> getSuffix(uint32_t slot, getSuffix_cb _hidl_cb) override;

    // Methods from ::android::hardware::boot::V1_1::IBootControl follow.
    Return<bool> setSnapshotMergeStatus(MergeStatus status) override;
    Return<MergeStatus> getSnapshotMergeStatus() override;

    // Methods from ::android::hardware::boot::V1_2::IBootControl follow.
    Return<uint32_t> getActiveBootSlot() override;

  private:
    android::hardware::samsung_slsi::boot_control::V1_2::Exynos_BootControl impl_;
};

extern "C" IBootControl* HIDL_FETCH_IBootControl(const char* name);

} // implementation
} // V1_2
} // namespace boot_control
} // namespace samsung_slsi
} // namespace hardware
} // namespace android


#endif  // BOOT_CONTROL_H
