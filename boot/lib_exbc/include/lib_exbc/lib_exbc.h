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

#ifndef _LIB_EXBC_H_
#define _LIB_EXBC_H_

#include <stdint.h>
#include <stdbool.h>

#include <android/hardware/boot/1.1/IBootControl.h>
#include "common.h"

namespace android {
namespace hardware {
namespace samsung_slsi {
namespace boot_control {
namespace V1 {

using ::android::hardware::boot::V1_1::MergeStatus;

  // Initial Status of Slot information.
  // If magic code is currupted, SlotInfo will be reset like below.
  // 
  // Slot Metadata
  // +======+==========+===========+=================+=================+
  // | Slot | bootable | is_active | boot_successful | tries_ramaining |
  // +======+==========+===========+=================+=================+
  // | A    | True     | True      | False           | 7               |
  // +------+----------+-----------+-----------------+-----------------+
  // | B    | True     | False     | False           | 7               |
  // +======+==========+===========+=================+=================+
  //
  // Virtual Info
  // +==============+
  // | Merge Status |
  // +==============+
  // |     None     |
  // +==============+

class BootControl {

 private:
  struct ExynosSlot_info si;

  const char * slot_suffix[2] = {"_a", "_b"};
  int exynos_slot_initialized = false;

 public:
  // Function for control Exynos Slot Information
  void reset_slot_info();
  void print_slot_info();
  bool GetInitFlag();
  void set_slotinfo(struct ExynosSlot_info data);
  struct ExynosSlot_info get_slotinfo();

  // API for Boot Control HAL 1.0
  void Init();
  bool IsValidSlot(uint32_t slot);
  uint32_t GetNumberSlots();
  uint32_t GetCurrentSlot();
  const char *GetSuffix(uint32_t slot);
  void MarkBootSuccessful(uint32_t slot);
  void SetActiveBootSlot(uint32_t slot);
  void SetSlotAsUnbootable(uint32_t slot);
  bool IsSlotBootable(uint32_t slot);
  bool IsSlotMarkedSuccessful(uint32_t slot);

  // API for Boot Control HAL 1.1
  void SetSnapshotMergeStatus(MergeStatus status);
  MergeStatus GetSnapshotMergeStatus(void);

  // API for Boot Control HAL 1.2
  uint32_t GetActiveBootSlot();
};

} // V1
} // namespace boot_control
} // namespace samsung_slsi
} // namespace hardware
} // namespace android

#endif  // _EXYNOS_BOOT_CONTROL_H
