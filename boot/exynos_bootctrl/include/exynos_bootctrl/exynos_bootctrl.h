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

#ifndef _EXYNOS_BOOT_CONTROL_H_
#define _EXYNOS_BOOT_CONTROL_H_

#include <stdint.h>
#include <stdbool.h>

#include <android/hardware/boot/1.2/IBootControl.h>
#include <lib_exbc/common.h>
#include <lib_exbc/lib_exbc.h>
#include <exsl/exsl.h>

namespace android {
namespace hardware {
namespace samsung_slsi {
namespace boot_control {
namespace V1_2 {

using ::android::hardware::boot::V1_1::MergeStatus;

class Exynos_BootControl {

	private:
		android::hardware::samsung_slsi::boot_control::V1::BootControl exbc;
		android::hardware::samsung_slsi::boot_control::V1_2::slot_loader::Slot_Loader loader;
		void Encode(struct ExynosSlot_info *si, struct slot_data *data);
		void Decode(struct ExynosSlot_info *si, struct slot_data *data);
		void Downloading_data();
		void Uploading_data();
		bool Load_Slot();
		bool Save_Slot();

	public:
		// API for Boot Control HAL 1.0
		bool Init();
		bool IsValidSlot(uint32_t slot);
		uint32_t GetNumberSlots();
		uint32_t GetCurrentSlot();
		const char *GetSuffix(uint32_t slot);
		bool MarkBootSuccessful(uint32_t slot);
		bool SetActiveBootSlot(uint32_t slot);
		bool SetSlotAsUnbootable(uint32_t slot);
		bool IsSlotBootable(uint32_t slot);
		bool IsSlotMarkedSuccessful(uint32_t slot);

		// API for Boot Control HAL 1.1
		bool SetSnapshotMergeStatus(MergeStatus status);
		MergeStatus GetSnapshotMergeStatus(void);

		// API for Boot Control HAL 1.2
		uint32_t GetActiveBootSlot();
};

} // V1_2
} // namespace boot_control
} // namespace samsung_slsi
} // namespace hardware
} // namespace android

#endif  // _EXYNOS_BOOT_CONTROL_H
