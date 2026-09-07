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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <android-base/properties.h>
#include <android-base/logging.h>

#include <lib_exbc/lib_exbc.h>

namespace android {
namespace hardware {
namespace samsung_slsi {
namespace boot_control {
namespace V1 {

using ::android::hardware::boot::V1_1::MergeStatus;

void BootControl::set_slotinfo(struct ExynosSlot_info data) {
	si = data;
}

struct ExynosSlot_info BootControl::get_slotinfo() {
	return si;
}

void BootControl::reset_slot_info() {
	LOG(ERROR) << "Warning: Reset Slot info";
	si.common.merge_status = MergeStatus::NONE;
	si.common.ota_flag = 0;
	int i;

	for (i=0; i<2; i++) {
		si.metadata[i].bootable = true;
		si.metadata[i].is_active = false;
		si.metadata[i].boot_successful = false;
		si.metadata[i].tries_remaining = 7;
	}

	si.metadata[0].is_active = true;
}

void BootControl::print_slot_info() {
	uint32_t i;
	LOG(INFO) << "=============METADATA=============";
	for (i=0; i<2; i++) {
		LOG(INFO) << "Slot[" << i << "].bootable        = " << si.metadata[i].bootable;
		LOG(INFO) << "Slot[" << i << "].is_active       = " << si.metadata[i].is_active;
		LOG(INFO) << "Slot[" << i << "].boot_successful = " << si.metadata[i].boot_successful;
		LOG(INFO) << "Slot[" << i << "].tries_remaining = " << si.metadata[i].tries_remaining;
		LOG(INFO) << "==================================";
  }
}

void BootControl::Init() {
	if (exynos_slot_initialized)
		return;

	android::base::InitLogging(nullptr, &android::base::KernelLogger);
	android::base::SetDefaultTag(std::string("EXBC_MODULE"));

	exynos_slot_initialized = true;
	return;
}

bool BootControl::GetInitFlag() {
	return exynos_slot_initialized;
}

uint32_t BootControl::GetNumberSlots() {
	return 2;
}

uint32_t BootControl::GetCurrentSlot() {
	std::string suffix_prop = android::base::GetProperty("ro.boot.slot_suffix", "");
	for (int i = 0; i < 2; i++) {
		if (strncmp(suffix_prop.c_str(), slot_suffix[i], 2) == 0) {
			return i;
		}
	}
	LOG(ERROR) << "WARNING: androidboot.slot_suffix is invalid";
	return -1;
}

const char * BootControl::GetSuffix(uint32_t slot) {
	if (IsValidSlot(slot))
		return slot_suffix[slot];

	return 0;
}

void BootControl::MarkBootSuccessful(uint32_t slot) {
	si.metadata[slot].boot_successful = true;
	return;
}

void BootControl::SetActiveBootSlot(uint32_t slot) {
	si.metadata[slot].bootable = true;
	si.metadata[slot].is_active = true;
	si.metadata[slot].boot_successful = false;
	si.metadata[slot].tries_remaining = 7;

	if (slot != GetCurrentSlot())
		si.common.ota_flag = 1;
	else
		si.common.ota_flag = 0;

	si.metadata[1-slot].is_active = false;
	return;
}

void BootControl::SetSlotAsUnbootable(uint32_t slot) {
	si.metadata[slot].bootable = false;
	si.metadata[slot].is_active = false;
	si.metadata[slot].boot_successful = false;
	si.metadata[slot].tries_remaining = 0;

	if (si.metadata[1-slot].bootable)
		si.metadata[1-slot].is_active = true;
	return;
}

bool BootControl::IsSlotBootable(uint32_t slot) {
	return si.metadata[slot].bootable;
}

bool BootControl::IsSlotMarkedSuccessful(uint32_t slot) {
	return si.metadata[slot].boot_successful;
}

void BootControl::SetSnapshotMergeStatus(MergeStatus status) {
	si.common.merge_status = status;
	return;
}

MergeStatus BootControl::GetSnapshotMergeStatus() {
	return si.common.merge_status;
}

bool BootControl::IsValidSlot(uint32_t slot) {
	if (slot >= 2) {
		LOG(ERROR) << "ERROR: Slot number Invalid 0 or 1: input - " << slot;
		return false;
	}
	return true;
}

uint32_t BootControl::GetActiveBootSlot() {
	uint32_t i;
	uint32_t max_slot;

	max_slot = GetNumberSlots();

	for (i = 0; i < max_slot; i++) {
		if (si.metadata[i].is_active)
			return i;
	}
	return 0;
}


} // V1
} // namespace boot_control
} // namespace samsung_slsi
} // namespace hardware
} // namespace android
