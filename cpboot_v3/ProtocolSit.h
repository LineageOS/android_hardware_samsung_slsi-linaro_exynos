/*
 * Copyright (C) 2020 Samsung Electronics
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

#pragma once

#include "Protocol.h"

/* Maximum Segment Size for payload */
#define STD_UDL_MSS		(16 * 1024)

/* Direction */
#define MSG_AP2CP			(0xA)
#define MSG_CP2AP			(0xC)

/* Command ID */
#define MSG_READY			(0x0)
#define MSG_DOWNLOAD			(0x1) // for BOOT
#define MSG_UPLOAD			(0x2) // for DUMP
#define MSG_SECURITY			(0x3) //security and CRC
#define MSG_FINALIZE			(0x4)
#define MSG_DEBUG			(0xD) // CP return current status with error code

#define MSG_NONE_TOC			(0x0)

#define MSG_BOOT			(0xB)
#define MSG_DUMP			(0xD)

/* Down/Up load commands */
#define MSG_START			(0x0)
#define MSG_DATA			(0xB)
#define MSG_DONE			(0xD)

/* Pass/Fail */
#define MSG_PASS			(0x0)
#define MSG_FAIL			(0xF)

/* Security */
#define MSG_CRC				(0x1)
#define MSG_SIGN			(0x2)

#define MSG_SHIFT_DIRECTION(_x)		(_x << 12)
#define MSG_SHIFT_CMD(_x)		(_x << 8)
#define MSG_SHIFT_INDEX(_x)		(_x << 4)
#define MSG_SHIFT_STAT(_x)		(_x << 0)

#define MSG(dir, cmd, index, stat)  ((u16)(MSG_SHIFT_DIRECTION(dir) | \
					MSG_SHIFT_CMD(cmd) | \
					MSG_SHIFT_INDEX(index) | \
					MSG_SHIFT_STAT(stat)))

struct sit_header {
	u16 cmd;
	u16 length;
} __packed;

struct sit_send_header {
	u16 cmd;
	u16 length;
	u32 total_size;
	u32 data_offset;
} __packed;

struct sit_recv_header {
	u16 cmd;
	u16 length;
	u32 total_size;
	u32 data_offset;
} __packed;

struct sit_dump_info {
	u32 dump_size;
	u32 reason_len;
	u8 reason[CRASH_REASON_SIZE];
} __packed;

struct exynos_data_info {
	u32 total_size;
	u32 data_offset;
} __packed;

struct exynos_payload {
	u16 cmd;
	u16 length;
	struct exynos_data_info dataInfo;
	u8 pdata[STD_UDL_MSS];
} __packed;

class ProtocolSit : public Protocol {
public:
	ProtocolSit();

private:
#ifdef CONFIG_TYPE_MODAP
	int std_boot_finish_handshake() override;
#endif
	int std_dump_receive_cp_dump() override;

	int std_udl_poll(enum operation oper, short events, long timeout) override;
	int std_ul_recv_raw_data(void *buffer, u32 size);
#ifdef CONFIG_TYPE_EXT
	int std_udl_stage_start(enum operation oper, u32 stage) override;
	int std_udl_stage_done(enum operation oper, u32 stage) override;
	int std_dl_send_bin(u32 stage, int b_fd, u8 *b_buffer, u32 size) override;
	int std_dl_tx_frame(int b_fd, u8 *b_buffer, void *frame) override;
	int std_dl_send_crc(u32 stage, u32 crc) override;
#endif

	int get_factory_prop(void) override;
};

