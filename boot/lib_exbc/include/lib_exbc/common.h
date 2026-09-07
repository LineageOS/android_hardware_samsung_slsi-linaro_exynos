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

#ifndef _EXYNOS_BOOT_CONTROL_COMMON_H_
#define _EXYNOS_BOOT_CONTROL_COMMON_H_

#include <stdint.h>
#include <stdbool.h>

#include <android/hardware/boot/1.1/IBootControl.h>

using ::android::hardware::boot::V1_1::MergeStatus;

// Structure for Exynos Slot Metadata for each slot
struct ExynosSlot_metadata {
  uint8_t bootable;
  uint8_t is_active;
  uint8_t boot_successful;
  uint8_t tries_remaining;
} __attribute__((packed));

// Structure for Exynos Slot Metadata for each slot
struct ExynosSlot_common {
  MergeStatus merge_status;
  uint8_t ota_flag;
} __attribute__((packed));

// Structure for Exynos Slot Information
struct ExynosSlot_info {
  struct ExynosSlot_metadata metadata[2];
  struct ExynosSlot_common common;
} __attribute__((packed));

#endif  // _EXYNOS_BOOT_CONTROL_COMMON_H
