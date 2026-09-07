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

#ifndef _EXYNOS_SLOT_LOADER_H_
#define _EXYNOS_SLOT_LOADER_H_

#include <stdint.h>
#include <stdbool.h>
#include <exynos_message/exynos_message.h>
#include "layout.h"

namespace android {
namespace hardware {
namespace samsung_slsi {
namespace boot_control {
namespace V1_1 {
namespace slot_loader {

class Slot_Loader {

	private:
		struct slot_data si;
		android::hardware::samsung_slsi::exynos_bootloader_message::Exynos_Bootloader_Message msg;

	public:
		bool verify_slot();
		void set_slot_data(struct slot_data data);
		struct slot_data get_slot_data();
		bool read_slot_data();
		bool write_slot_data();
};

} // namespace slot_loader
} // V1_1
} // namespace boot_control
} // namespace samsung_slsi
} // namespace hardware
} // namespace android

#endif  // _EXYNOS_BOOT_CONTROL_H
