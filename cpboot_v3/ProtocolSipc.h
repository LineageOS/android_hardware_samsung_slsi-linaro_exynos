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
#define STD_UDL_MSS		(2 * 1024)

struct std_udl_frame {
	u32 cmd;
	u32 num_frames;
	u32 curr_frame;
	u32 len;
	u8 data[STD_UDL_MSS];
} __packed;

class ProtocolSipc : public Protocol {
public:
	ProtocolSipc();

private:
#ifdef CONFIG_TYPE_MODAP
	int std_boot_finish_handshake() override;
#endif
	int std_dump_receive_cp_dump() override;
	int std_reboot_system(const char *str) override;

	int std_udl_stage_start(enum operation oper, u32 stage) override;
	int std_udl_stage_done(enum operation oper, u32 stage) override;
	int std_udl_poll(enum operation oper, short events, long timeout) override;
#ifdef CONFIG_TYPE_EXT
	int std_dl_send_bin(u32 stage, int b_fd, u8 *b_buffer, u32 size) override;
	int std_dl_tx_frame(int b_fd, u8 *b_buffer, void *frame) override;
	int std_dl_send_crc(u32 stage, u32 crc) override;
#endif
	int std_ul_recv_info();
	int std_ul_recv_data(u32 step);

	unsigned char check_csc_sales_code() override;
	int get_factory_prop(void) override;
};

