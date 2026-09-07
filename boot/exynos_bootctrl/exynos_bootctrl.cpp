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

#include "exynos_bootctrl/exynos_bootctrl.h"

#define LOAD_FAIL	"load fail"
#define SAVE_FAIL	"save fail"

namespace android {
namespace hardware {
namespace samsung_slsi {
namespace boot_control {
namespace V1_2 {

using ::android::hardware::boot::V1_1::MergeStatus;

void Exynos_BootControl::Encode(struct ExynosSlot_info *si, struct slot_data *data) {
	int i;
	for (i=0; i<MAX_SLOT_NUMBER; i++) {
		data->metadata[i].bootable = si->metadata[i].bootable;
		data->metadata[i].is_active = si->metadata[i].is_active;
		data->metadata[i].boot_successful = si->metadata[i].boot_successful;
		data->metadata[i].tries_remaining = si->metadata[i].tries_remaining;
	}
	data->merge_status = si->common.merge_status;
	data->ota_flag = si->common.ota_flag;
	memcpy(data->magic, EXYNOS_SLOT_INFO_MAGIC, EXYNOS_SLOT_INFO_MAGIC_SIZE);
	return;
}

void Exynos_BootControl::Decode(struct ExynosSlot_info *si, struct slot_data *data) {
	int i;
	for (i=0; i<MAX_SLOT_NUMBER; i++) {
		si->metadata[i].bootable = data->metadata[i].bootable;
		si->metadata[i].is_active = data->metadata[i].is_active;
		si->metadata[i].boot_successful = data->metadata[i].boot_successful;
		si->metadata[i].tries_remaining = data->metadata[i].tries_remaining;
	}
	si->common.merge_status = data->merge_status;
	si->common.ota_flag = data->ota_flag;
	memcpy(data->magic, EXYNOS_SLOT_INFO_MAGIC, EXYNOS_SLOT_INFO_MAGIC_SIZE);
	return;
}

// * Note for direction * //
// Downloading	: from loader to lib_exbc
// Uploading	: from lib_exbc to loader
void Exynos_BootControl::Downloading_data() {
	struct slot_data data;
	struct ExynosSlot_info info;
	data = loader.get_slot_data();
	// From data translate to info
	Decode(&info, &data);
	exbc.set_slotinfo(info);
	return;
}

void Exynos_BootControl::Uploading_data() {
	struct slot_data data;
	struct ExynosSlot_info info;
	info = exbc.get_slotinfo();
	// From info translate to data
	Encode(&info, &data);
	loader.set_slot_data(data);
}

bool Exynos_BootControl::Load_Slot() {
	if (!loader.read_slot_data()) {
		LOG(ERROR) << __func__ << " : " << LOAD_FAIL;
		return false;
	}
	if (!loader.verify_slot()) {
		// Reset Slot information because of data damaged
		exbc.reset_slot_info();

		// Transport New slot information to loader and save it
		Uploading_data();
		if (!loader.write_slot_data()) {
			LOG(ERROR) << __func__ << " : " << SAVE_FAIL;
			return false;
		}
	} else {
		// If data is valid, download data from loader
		Downloading_data();
	}
	return true;
}

bool Exynos_BootControl::Save_Slot() {
	Uploading_data();
	if (!loader.write_slot_data()) {
		LOG(ERROR) << __func__ << " : " << SAVE_FAIL;
		return false;
	}
	return true;
}

bool Exynos_BootControl::Init() {
	if (exbc.GetInitFlag())
		return true;

	android::base::InitLogging(nullptr, &android::base::KernelLogger);
	android::base::SetDefaultTag(std::string("ExynosBootControl"));

	// Try to load slot data
	if (!Load_Slot()) {
		return false;
	}
	exbc.Init();
	return true;
}

uint32_t Exynos_BootControl::GetNumberSlots() {
	return exbc.GetNumberSlots();
}

uint32_t Exynos_BootControl::GetCurrentSlot() {
	return exbc.GetCurrentSlot();
}

const char * Exynos_BootControl::GetSuffix(uint32_t slot) {
	if (!IsValidSlot(slot))
		return nullptr;

	return exbc.GetSuffix(slot);
}

bool Exynos_BootControl::MarkBootSuccessful(uint32_t slot) {
	if (!IsValidSlot(slot))
		return false;

	if (!Load_Slot()) {
		LOG(ERROR) << __func__ << LOAD_FAIL;
		return false;
	}
	exbc.MarkBootSuccessful(slot);
	if (!Save_Slot()) {
		LOG(ERROR) << __func__ << SAVE_FAIL;
		return false;
	}
	return true;
}

bool Exynos_BootControl::SetActiveBootSlot(uint32_t slot) {
	if (!IsValidSlot(slot))
		return false;

 	if (!Load_Slot()) {
		LOG(ERROR) << __func__ << LOAD_FAIL;
		return false;
	}
	exbc.SetActiveBootSlot(slot);
	if (!Save_Slot()) {
		LOG(ERROR) << __func__ << SAVE_FAIL;
		return false;
	}
	return true;
}

bool Exynos_BootControl::SetSlotAsUnbootable(uint32_t slot) {
	if (!IsValidSlot(slot))
		return false;

  	if (!Load_Slot()) {
		LOG(ERROR) << __func__ << LOAD_FAIL;
		return false;
	}
	exbc.SetSlotAsUnbootable(slot);
	if (!Save_Slot()) {
		LOG(ERROR) << __func__ << SAVE_FAIL;
		return false;
	}
	return true;
}

bool Exynos_BootControl::IsSlotBootable(uint32_t slot) {
	if (!IsValidSlot(slot))
		return false;

	if (!Load_Slot()) {
		LOG(ERROR) << __func__ << LOAD_FAIL;
		return false;
	}
	return exbc.IsSlotBootable(slot);
}

bool Exynos_BootControl::IsSlotMarkedSuccessful(uint32_t slot) {
	if (!IsValidSlot(slot))
		return false;

 	if (!Load_Slot()) {
		LOG(ERROR) << __func__ << LOAD_FAIL;
		return false;
	}
	return exbc.IsSlotMarkedSuccessful(slot);
}

bool Exynos_BootControl::SetSnapshotMergeStatus(MergeStatus status) {
	if (!Load_Slot()) {
		LOG(ERROR) << __func__ << LOAD_FAIL;
		return false;
	}
	exbc.SetSnapshotMergeStatus(status);
	if (!Save_Slot()) {
		LOG(ERROR) << __func__ << SAVE_FAIL;
		return false;
	}
	return true;

}

MergeStatus Exynos_BootControl::GetSnapshotMergeStatus() {
  	if (!Load_Slot()) {
		LOG(ERROR) << __func__ << LOAD_FAIL;
		return MergeStatus::UNKNOWN;
	}
	return exbc.GetSnapshotMergeStatus();
}

bool Exynos_BootControl::IsValidSlot(uint32_t slot) {
	return exbc.IsValidSlot(slot);
}

uint32_t Exynos_BootControl::GetActiveBootSlot() {
	return exbc.GetActiveBootSlot();
}

} // V1_2
} // namespace boot_control
} // namespace samsung_slsi
} // namespace hardware
} // namespace android
