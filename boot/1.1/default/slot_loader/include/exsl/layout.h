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

#ifndef _EXYNOS_BOOT_CONTROL_LAYOUT_H_
#define _EXYNOS_BOOT_CONTROL_LAYOUT_H_

#include <stdint.h>
#include <stdbool.h>

#include <android/hardware/boot/1.1/IBootControl.h>

#define EXYNOS_SLOT_INFO_MAGIC "EXBC"
#define EXYNOS_SLOT_INFO_MAGIC_SIZE 4
#define SLOT_INFO_PARTITION "/slotinfo"
#define MAX_SLOT_NUMBER 2

constexpr size_t SLOTINFO_OFFSET = 2 * 1024;

using ::android::hardware::boot::V1_1::MergeStatus;

// Structure for Exynos Slot Metadata for each slot
struct slot_metadata {
  uint8_t bootable;
  uint8_t is_active;
  uint8_t boot_successful;
  uint8_t tries_remaining;

  uint8_t reserved[4];
} __attribute__((packed));

#if (__STDC_VERSION__ >= 201112L) || defined(__cplusplus)
static_assert(sizeof(struct slot_metadata) == 8,
    "struct ExynosSlot_metadata size changes, which may break A/B devices");
#endif

// Structure for Exynos Slot Information
struct slot_data {
  uint8_t magic[EXYNOS_SLOT_INFO_MAGIC_SIZE];
  struct slot_metadata metadata[MAX_SLOT_NUMBER];
  MergeStatus merge_status : 8;
  uint8_t reserved[11];
} __attribute__((packed));

#if (__STDC_VERSION__ >= 201112L) || defined(__cplusplus)
static_assert(sizeof(struct slot_data) == 32,
    "struct ExynosSlotInfo size changes, which may break A/B devices");
#endif

#endif  // _EXYNOS_BOOT_CONTROL_H
