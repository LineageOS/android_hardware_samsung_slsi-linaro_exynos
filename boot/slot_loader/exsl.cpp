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

#include <exynos_message/exynos_message.h>
#include <exsl/exsl.h>

#define OPEN_FAIL		"open fail"
#define LOAD_FAIL		"load fail"
#define SAVE_FAIL		"save fail"
#define INVALID_MAGIC	"Invalid magic"

namespace android {
namespace hardware {
namespace samsung_slsi {
namespace boot_control {
namespace V1_2 {
namespace slot_loader {

bool Slot_Loader::verify_slot() {
	if (!memcmp(si.magic, EXYNOS_SLOT_INFO_MAGIC, EXYNOS_SLOT_INFO_MAGIC_SIZE)) {
		return true;
	}
	return false;
}

void Slot_Loader::set_slot_data(struct slot_data data) {
	si = data;
	return;
}

struct slot_data Slot_Loader::get_slot_data() {
	return si;
}

bool Slot_Loader::read_slot_data() {
	if (!msg.open_message(SLOT_INFO_PARTITION)) {
		LOG(ERROR) << __func__ << " : " << OPEN_FAIL;
		return false;
	}
	if (!msg.read_message(&si, SLOTINFO_OFFSET, sizeof(struct slot_data))) {
		LOG(ERROR) << __func__ << " : " << LOAD_FAIL;
		return false;
	}
	msg.close_message();
	return true;
}

bool Slot_Loader::write_slot_data() {
	if (!msg.open_message(SLOT_INFO_PARTITION)) {
		LOG(ERROR) << __func__ << " : " << OPEN_FAIL;
		return false;
	}
	if (!msg.write_message(&si, SLOTINFO_OFFSET, sizeof(struct slot_data))) {
		LOG(ERROR) << __func__ << " : " << SAVE_FAIL;
		return false;
	}
	msg.close_message();
	return true;
}

} // namespace slot_loader
} // namespace V1_2
} // namespace boot_control
} // namespace samsung_slsi
} // namespace hardware
} // namespace android
