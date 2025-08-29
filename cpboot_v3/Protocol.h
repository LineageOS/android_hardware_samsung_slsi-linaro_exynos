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

#include <sys/ioctl.h>
#include <cutils/properties.h>

#include "Type.h"
#include "Log.h"

#define CP_WDT_RESET_STR	"CP WDOG Reset"
#define CP_CRASH_BY_STR		": CP Crash by"

/* cmd (4) + num_frames (4) + curr_frame (4) + len (4) */
#define STD_UDL_HDR_LEN		16
/* Maximum Transmission Unit = Header + Payload */
#define STD_UDL_MTU		(STD_UDL_HDR_LEN + STD_UDL_MSS)
#define STD_UDL_FIN_STAGE	0xF
#define STD_UDL_DUMP_STAGE	0xD
#define MAX_DLOAD_STAGE		STD_UDL_DUMP_STAGE

#define WAIT_POLL_TIME		30000	/* 30secs */

struct std_uload_info {
	u32 dump_size;
	u32 num_steps;
	u32 reason_len;
} __packed;

struct std_udl_crc_frame {
	u32 cmd;
	u32 crc;
} __packed;

struct std_toc_element {
	char name[MAX_IMG_NAME_LEN];	/* Binary name			*/
	u32 b_offset;			/* Binary offset in the file	*/
	u32 m_offset;			/* Memory Offset to be loaded	*/
	u32 size;			/* Binary size			*/
	u32 crc;			/* CRC value			*/
	u32 toc_count;			/* Reserved			*/
} __packed;

struct std_dload_control {
	/* Stage information */
	u32 stage;
	/* Handshaking control */
	int start;
	int download;
	int validate;
	int finish;
	/* Binary information */
	int b_fd;
	u32 b_offset;
	u32 m_offset;
	u32 b_size;
	u32 crc;
	/* Download once */
	int dl_once;
};

enum filedesc_t {
	/* BOOT & DUMP */
	FD_DEV = 0,
	FD_UPLOAD_DEV,

	/* BOOT */
	FD_BIN,
	FD_NV_DATA,
	FD_NV_NORM,
	FD_NV_PROT,

	/* DUMP */
	FD_LOG,
	FD_INFO,
	FD_DUMP,

	FD_MAX,
};

struct std_boot_args {
	int fds[FD_MAX];
	u32 num_stages;
	u32 start_stage;
	int toc_idx[TOC_MAX];
	struct std_toc_element toc[MAX_TOC_INDEX];
	struct std_dload_control dl_ctrl[MAX_DLOAD_STAGE];
};

struct std_dump_args {
	int fds[FD_MAX];
	struct std_uload_info info;
	char reason[CRASH_REASON_SIZE];
};

class Protocol {
protected:
	Protocol();
	virtual ~Protocol() {}

public:
#ifdef CONFIG_TYPE_MODAP
	virtual int std_boot_finish_handshake() = 0;
#endif
	virtual int std_dump_receive_cp_dump() = 0;
	virtual int std_reboot_system(const char *str) { return 0; }

#if !defined(CONFIG_PROTOCOL_SIT) || (defined(CONFIG_PROTOCOL_SIT) && defined(CONFIG_TYPE_EXT))
	virtual int std_udl_stage_start(enum operation oper, u32 stage) = 0;
	virtual int std_udl_stage_done(enum operation oper, u32 stage) = 0;
#endif
	virtual int std_ul_rx_frame(void *buff, u32 size);

#ifdef CONFIG_TYPE_EXT
	virtual int std_dl_send_bin(u32 stage, int b_fd, u8 *b_buffer, u32 size) = 0;
	virtual int std_dl_send_crc(u32 stage, u32 crc) = 0;
#endif

public:
	virtual unsigned char check_csc_sales_code() { return 0; }
	virtual int get_factory_prop(void) = 0;

protected:
	virtual int std_udl_poll(enum operation oper, short events, long timeout) = 0;
	virtual int std_udl_req_resp(enum operation oper, u32 req, u32 exp);

#ifdef CONFIG_TYPE_EXT
	virtual int std_dl_tx_frame(int b_fd, u8 *b_buffer, void *frame) = 0;
#endif

protected:
	virtual void make_cp_crash_reason(char *cp_reason);
};

