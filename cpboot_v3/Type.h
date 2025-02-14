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

#ifndef s8
typedef char		s8;
#endif

#ifndef s16
typedef short		s16;
#endif

#ifndef s32
typedef int		s32;
#endif

#ifndef u8
typedef unsigned char	u8;
#endif

#ifndef u16
typedef unsigned short	u16;
#endif

#ifndef u32
typedef unsigned int	u32;
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/*
 * IOCTL commands
 */
#define IOCTL_MAGIC	'o'

#define IOCTL_POWER_ON			_IO(IOCTL_MAGIC, 0x19)
#define IOCTL_POWER_OFF			_IO(IOCTL_MAGIC, 0x20)

enum cp_boot_mode {
	CP_BOOT_MODE_NORMAL,
	CP_BOOT_MODE_DUMP,
	CP_BOOT_RE_INIT,
	CP_BOOT_REQ_CP_RAM_LOGGING = 5,
	CP_BOOT_MODE_MANUAL = 7,
	CP_BOOT_MODE_VSS = 12,
	MAX_CP_BOOT_MODE
};
struct boot_mode {
	enum cp_boot_mode idx;
} __attribute__((__packed__));
#define IOCTL_POWER_RESET		_IOW(IOCTL_MAGIC, 0x21, struct boot_mode)
#define IOCTL_START_CP_BOOTLOADER	_IOW(IOCTL_MAGIC, 0x22, struct boot_mode)
#define IOCTL_MODEM_BOOT_ON		_IO('o', 0x22)
#define IOCTL_COMPLETE_NORMAL_BOOTUP	_IO(IOCTL_MAGIC, 0x23)
#define IOCTL_GET_CP_STATUS		_IO(IOCTL_MAGIC, 0x27)
#define IOCTL_MODEM_DL_START            _IO('o', 0x28)
#define IOCTL_TRIGGER_CP_CRASH		_IO(IOCTL_MAGIC, 0x34)
#define IOCTL_TRIGGER_KERNEL_PANIC	_IO(IOCTL_MAGIC, 0x35)

struct cp_image {
#ifdef LEGACY_IOCTL
	u32 stage;
	u32 m_offset;
	u8 *binary;
	u32 size;
	u32 b_offset;
	u32 len;
#else
	u8 *binary;
	u32 size;
	u32 m_offset;
	u32 b_offset;
	u32 mode;
	u32 len;
#endif
} __attribute__((__packed__));
#define IOCTL_LOAD_CP_IMAGE		_IOW(IOCTL_MAGIC, 0x40, struct cp_image)

#define IOCTL_GET_SRINFO		_IO(IOCTL_MAGIC, 0x45)
#define IOCTL_SET_SRINFO		_IO(IOCTL_MAGIC, 0x46)
#define IOCTL_GET_CP_BOOTLOG		_IO(IOCTL_MAGIC, 0x47)
#define IOCTL_CLR_CP_BOOTLOG		_IO(IOCTL_MAGIC, 0x48)

/* Log dump */
#define IOCTL_MIF_LOG_DUMP		_IO(IOCTL_MAGIC, 0x51)

enum cp_log_dump_index {
	LOG_IDX_SHMEM,
	LOG_IDX_VSS,
	LOG_IDX_ACPM,
	LOG_IDX_CP_BTL,
	LOG_IDX_DATABUF,
	LOG_IDX_L2B,
	LOG_IDX_DDM,
	LOG_IDX_DATABUF_DL,
	LOG_IDX_DATABUF_UL,
	LOG_IDX_MAX
};
struct cp_log_dump {
	char name[32];
	enum cp_log_dump_index idx;
	u32 size;
} __attribute__((__packed__));
#define IOCTL_GET_LOG_DUMP		_IOWR(IOCTL_MAGIC, 0x52, struct cp_log_dump)

struct modem_sec_req {
	u32 mode;
	u32 param2;
	u32 param3;
	u32 param4;
} __attribute__((__packed__));
#define IOCTL_REQ_SECURITY		_IOW('o', 0x53, struct modem_sec_req)

/* Crash reason */
#define CRASH_REASON_SIZE	512
enum crash_type {
	CRASH_REASON_CP_ACT_CRASH = 0,
	CRASH_REASON_RIL_MNR,
	CRASH_REASON_RIL_REQ_FULL,
	CRASH_REASON_RIL_PHONE_DIE,
	CRASH_REASON_RIL_RSV_MAX,
	CRASH_REASON_USER = 5,
	CRASH_REASON_MIF_TX_ERR = 6,
	CRASH_REASON_MIF_RIL_BAD_CH,
	CRASH_REASON_MIF_RX_BAD_DATA,
	CRASH_REASON_RIL_TRIGGER_CP_CRASH,
	CRASH_REASON_MIF_FORCED,
	CRASH_REASON_CP_WDOG_CRASH,
	CRASH_REASON_MIF_RSV_MAX = 12,
	CRASH_REASON_CP_SRST,
	CRASH_REASON_CP_RSV_0,
	CRASH_REASON_CP_RSV_MAX,
	CRASH_REASON_CLD = 16,
	CRASH_REASON_NONE = 0xFFFF,
};
struct crash_reason {
	u32 owner;
	char string[CRASH_REASON_SIZE];
} __attribute__((__packed__));
#define IOCTL_GET_CP_CRASH_REASON	_IOR('o', 0x55, struct crash_reason)

#define CPIF_VERSION_SIZE	20
struct cpif_version {
	char string[CPIF_VERSION_SIZE];
} __attribute__((__packed__));
#define IOCTL_GET_CPIF_VERSION		_IOR('o', 0x56, struct cpif_version)

struct sec_info {
    int bmode;
    u32 boot_size;
    u32 main_size;
};
#define IOCTL_CHECK_SECURITY            _IO('o', 0x62)
#define IOCTL_XMIT_BIN                  _IO('o', 0x63)

#define CPDUMP_PATH		"/data/vendor/log/cbd"

/* property for vendor */
#define VPROP_CPBOOT		"vendor.cbd.cpboot"
#define VPROP_CPBOOT_DONE	"vendor.cbd.boot_done"
#define VPROP_CPRESET_DONE	"vendor.cbd.reset_done"
#define VPROP_FIRST_XMIT_DONE	"vendor.cbd.first_xmit_done"
#define VPROP_DT_REVISION	"vendor.cbd.dt_revision"
#define VPROP_DEBUG_CP_OPT	"persist.vendor.cbd.debug_cp_opt"
#define VPROP_BTL_SIZE		"persist.vendor.cbd.btl_size"

#ifdef CONFIG_DUMP_LIMIT
#define VPROP_CDUMP_INDEX	"persist.vendor.cbd.crash_dump_index"
#define VPROP_CDUMP_LIMIT	"persist.vendor.cbd.crash_dump_limit"
#endif

#ifdef CONFIG_PROTOCOL_SIT
#define VPROP_RFS_CHECKDONE	"vendor.ril.cbd.rfs_check_done"
#define VPROP_RILD_RESET	"vendor.sys.rild_reset"
#define VPROP_CRASH_MODE	"persist.vendor.ril.crash_handling_mode"
#else
#define VPROP_RFS_CHECKDONE	"vendor.cbd.rfs_check_done"
#define VPROP_DEV_OFFRES	"vendor.cbd.deviceOffRes"
#endif

/* property for system */
#define PROP_DEBUG_LEVEL	"ro.boot.debug_level"
#define PROP_FORCE_UPLOAD	"ro.boot.force_upload"

#ifdef CONFIG_PROTOCOL_SIT
#define PROP_RADIO_MULTISIM_CONFIG	"persist.radio.multisim.config"
#else
#define PROP_SERIAL_NO			"ro.serialno"
#define PROP_SALES_CODE			"ro.csc.sales_code"
#define PROP_DEV_OFFREQ			"sys.shutdown.requested"
#define PROP_SYS_POWERCTL		"sys.powerctl"
#endif

#define STAGE_VSS		2

#define MAX_CMD_LINE_LEN	1024

#define MAX_NAME_LEN		32
#define MAX_PREFIX_LEN		64
#define MAX_SUFFIX_LEN		64
#define MAX_PATH_LEN		512
#define MAX_PROP_STRING_LEN	128

#define MAX_TOC_INDEX		16
#define MAX_TOC_ELEMENT_SIZE	32
#define MAX_TOC_SIZE		(MAX_TOC_INDEX * MAX_TOC_ELEMENT_SIZE)	/* 512 */
#define MAX_IMG_NAME_LEN	12

#define MAX_ERROR_INFO_BUF_SIZE 512

#define SZ_1M	0x00100000

#define PATH_BTL_NODE		"/dev/ramdump_memshare"

#define IOCTL_GET_BTL_SIZE	_IO(IOCTL_MAGIC, 0x59)
#define IOCTL_SET_BTL_SIZE	_IOW(IOCTL_MAGIC, 0x60, u32)

enum modem_state {
	STATE_OFFLINE,
	STATE_CRASH_RESET,	/* silent reset */
	STATE_CRASH_EXIT,	/* cp ramdump */
	STATE_BOOTING,
	STATE_ONLINE,
	STATE_NV_REBUILDING,	/* NV rebuilding start */
	STATE_LOADER_DONE,
	STATE_SIM_ATTACH,
	STATE_SIM_DETACH,
	STATE_CRASH_WATCHDOG,	/* cp watchdog crash */
};

enum modem_t {
	MODEM_INVALID = 0,
	SEC_SS310,
	SEC_S5100,
	SEC_MODAP_SIT,
	SEC_S5100_SIT,
	MAX_MODEM_TYPE
};

enum modem_link {
	LINKDEV_UNDEFINED,
	LINKDEV_SPI,
	LINKDEV_SHMEM,
	LINKDEV_PCIE,
	LINKDEV_MAX,
};

enum sec_debug_level {
	DBG_LOW,
	DBG_MID,
	DBG_HIGH,
	DBG_AUTO,
};

enum sec_cp_debug {
	DBG_CP_NORMAL,
	DBG_CP_NOCRASH,		/* hang cbd on cp crash situation */
	DBG_CP_NORESET,		/* on 2nd boot trying, skip CP boot */
	DBG_CP_NOBOOT,		/* skip CP boot */
	DBG_CP_AUTORESET,	/* reset CP repeatedly */
	DBG_CP_FORCEPANIC,	/* force a kernel panic on cp crash*/
};

enum cp_image_toc_type {
	TOC_TOC,
	TOC_BOOT,
	TOC_MAIN,
	TOC_VSS,
	TOC_NV_NORM,
	TOC_NV_PROT,
	TOC_NV,
	TOC_OFFSET,
	TOC_MAX
};

struct modem_comp {
	char name[MAX_NAME_LEN];
	const char *rat;

	char node_boot[MAX_NAME_LEN];
	char *node_status;
	char node_dump[MAX_NAME_LEN];
	char path_bin[MAX_PATH_LEN];

	char path_nv_data[MAX_PATH_LEN];
	char path_nv_norm[MAX_PATH_LEN];
	char path_nv_prot[MAX_PATH_LEN];
	u32 nv_size;

	int num_stages;
	enum cp_image_toc_type first_stage_toc_type;
};

struct std_cbd_args {
	enum modem_t type;
	enum modem_link lnk_boot;
	enum modem_link lnk_main;
	unsigned daemon;
	struct modem_comp cpn;	/*component*/
	unsigned options;	/*wildcard?*/
	char reason[CRASH_REASON_SIZE];
};

enum Status : int {
	OK = 0,
	ERROR,
};

enum operation {
	OPER_BOOT = 0,
	OPER_DUMP,
	OPER_MAX,
};

