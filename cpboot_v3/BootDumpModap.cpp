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

#include "BootDumpModap.h"
#include "Srinfo.h"
#include "Container.h"

#define EXYNOS_PAYLOAD_LEN	(62*1024)
#define CP_MEMORY_MASK		(0x40000000 - 1)

#ifdef CONFIG_PROTOCOL_SIT
#define SHANNON_LEGACY_MAX_DL_STAGE 4 /* BOOT, MAIN, NV, NV_PROT */
#else
#define SHANNON_LEGACY_MAX_DL_STAGE 3 /* BOOT, MAIN, NV */
#endif

BootDumpModap::BootDumpModap()
{
	memset(&std_boot, 0, sizeof(struct std_boot_args));
	memset(&std_dump, 0, sizeof(struct std_dump_args));
}

int BootDumpModap::boot()
{
	int ret = 0;
	int spin = 50;
	int boot_once = 0;
	char prop_buf[PROPERTY_VALUE_MAX] = {0, };
#ifdef LEGACY_IOCTL
	struct sec_info info;
#endif

	cbd_info("CP boot device = %s\n", Container::getCbdArgs()->cpn.node_boot);
	cbd_info("CP binary file = %s\n", Container::getCbdArgs()->cpn.path_bin);
#ifdef CONFIG_PROTOCOL_SIT
	cbd_info("CP NV normal file = %s\n", Container::getCbdArgs()->cpn.path_nv_norm);
	cbd_info("CP NV prot file = %s\n", Container::getCbdArgs()->cpn.path_nv_prot);
#else
	cbd_info("CP NV data file = %s\n", Container::getCbdArgs()->cpn.path_nv_data);
#endif

	switch (Container::getCbdArgs()->lnk_boot) {
	case LINKDEV_SHMEM:
		cbd_info("BOOT link SHMEM\n");
		break;
	default:
		cbd_info("ERR! BOOT link# %d not supported\n", Container::getCbdArgs()->lnk_boot);
		ret = -ENODEV;
		goto exit;
	}

	switch (Container::getCbdArgs()->lnk_main) {
	case LINKDEV_SHMEM:
		cbd_info("MAIN link SHMEM\n");
		break;
	default:
		cbd_info("ERR! MAIN link# %d not supported\n", Container::getCbdArgs()->lnk_main);
		ret = -ENODEV;
		goto exit;
	}

	/* Wait for completion of RILD's NV validity check */
	while (spin--) {
		property_get(VPROP_RFS_CHECKDONE, prop_buf, "0");
		if (prop_buf[0] == '1')
			break;
		usleep(100000);
	}
	cbd_info("NV validation %s\n", spin < 0 ? "TIMEOUT" : "done");

	/*
	** Start CP bootup
	*/
	boot_wake_lock(1);

	cbd_info("Prepare arguments\n");
	if (!prepare_boot_args(CP_BOOT_MODE_NORMAL)) {
		cbd_info("ERR! prepare_boot_args fail\n");
		ret = -EFAULT;
		goto exit;
	}

	if (Container::getSrinfo())
		Container::getSrinfo()->store_srinfo(std_boot.fds[FD_DEV]);

	if (get_modem_state(std_boot.fds[FD_DEV]) != STATE_OFFLINE) {
		cbd_info("Power reset CP_BOOT_MODE_NORMAL\n");
		ret = std_boot_power_reset(CP_BOOT_MODE_NORMAL);
		if (ret < 0) {
			cbd_info("ERR! std_boot_power_reset fail\n");
			goto exit;
		}
	}

#ifdef LEGACY_IOCTL
	cbd_info("Send CP image\n");
	ret = load_cp_images(CP_BOOT_MODE_NORMAL);
	if (ret < 0) {
		cbd_info("ERR! BOOT_STAGE fail\n");
		goto exit;
	}
	boot_once = ret;


	info.bmode = CP_BOOT_MODE_NORMAL;
	info.boot_size = std_boot.dl_ctrl[TOC_BOOT].b_size;
	info.main_size = std_boot.dl_ctrl[TOC_MAIN].b_size;

	cbd_info("IOCTL_CHECK_SECURITY bmode=%d, boot_size=%d, main_size=%d",
			info.bmode, info.boot_size, info.main_size);
	ret = ioctl(getStdBoot()->fds[FD_DEV], IOCTL_CHECK_SECURITY, &info);
	if (ret < 0) {
		cbd_err("modem_request_security failed!!!\n");
		goto exit;
	}
#endif

	cbd_info("Power on CP\n");
	ret = std_boot_power_on();
	if (ret < 0) {
		cbd_info("ERR! std_boot_power_on fail\n");
		goto exit;
	}

#ifndef LEGACY_IOCTL
	cbd_info("Request security : non-secure mode\n");
	ret = std_security_req(CP_BOOT_RE_INIT, 0, 0);
	if (ret < 0) {
		cbd_info("ERR! security check fail\n");
		goto exit;
	}

	cbd_info("Send CP image\n");
	ret = load_cp_images(CP_BOOT_MODE_NORMAL);
	if (ret < 0) {
		cbd_info("ERR! BOOT_STAGE fail\n");
		goto exit;
	}
	boot_once = ret;

#ifdef CONFIG_SEC_CP_VERIFYING_ALL
	/* Request Security : secure mode
	   - TOC/VSS verifying have to be first, before BOOT/MAIN part.
		 because of MODE_NORMAL command set the memory access limitation. */
	cbd_info("Request Security : secure mode\n");
	ret = std_security_req(CP_BOOT_MODE_MANUAL,
				   std_boot.dl_ctrl[TOC_TOC].m_offset,
				   std_boot.dl_ctrl[TOC_TOC].b_size);
	if (ret < 0) {
		cbd_info("ERR! security check fail for TOC\n");
		goto exit;
	}

	if (std_boot.toc_idx[TOC_VSS] >= 0) {
		u32 vss_mode = (Container::getCbdArgs()->options & BOPT_VERIFY_VSS) ?
			CP_BOOT_MODE_VSS : CP_BOOT_MODE_MANUAL;

		if (boot_once && (vss_mode == CP_BOOT_MODE_MANUAL)) {
			/* if xmit_boot was already done once, skip vss verifying */
			cbd_info("skip VSS check\n");
		} else {
			cbd_info("Request Security : VSS secure mode\n");
			ret = std_security_req(vss_mode,
					std_boot.dl_ctrl[TOC_VSS].m_offset,
					std_boot.dl_ctrl[TOC_VSS].b_size);
			if (ret < 0) {
				cbd_info("ERR! security check fail for VSS\n");
				goto exit;
			}
		}
	}
#endif

	cbd_info("Request security : secure mode\n");
	ret = std_security_req(CP_BOOT_MODE_NORMAL,
				   std_boot.dl_ctrl[TOC_BOOT].b_size,
				   std_boot.dl_ctrl[TOC_MAIN].b_size);
	if (ret < 0) {
		cbd_info("ERR! security check fail for BOOT/MAIN\n");
		goto exit;
	}
#endif

	/* set SIM configuration using /efs/factory.prop */
	set_sim_configuration();

	cbd_info("Start CP bootloader\n");
	ret = std_boot_start_cp_bootloader(CP_BOOT_MODE_NORMAL);
	if (ret < 0) {
		cbd_info("ERR! std_boot_start_cp_bootloader fail\n");
		goto exit;
	}

#ifdef LEGACY_IOCTL
	ret = ioctl(getStdBoot()->fds[FD_DEV], IOCTL_MODEM_DL_START, NULL);
	if (ret < 0) {
		cbd_err("modem_request_security failed!!!\n");
		goto exit;
	}
#endif

	cbd_info("Handshake\n");
	ret = Container::getProtocol()->std_boot_finish_handshake();
	if (ret < 0) {
		cbd_info("ERR! std_boot_finish_handshake fail\n");
		goto exit;
	}

	cbd_info("Complete normal bootup\n");
	ret = std_boot_complete_normal_bootup();
	if (ret < 0) {
		cbd_info("ERR! std_boot_complete_normal_bootup fail\n");
		goto exit;
	}

	if (Container::getSrinfo())
		Container::getSrinfo()->restore_srinfo(std_boot.fds[FD_DEV]);

exit:
	std_boot_close_args();

	boot_wake_lock(0);

	return ret;
}

int BootDumpModap::dump()
{
	int ret;
	char reason[MAX_PREFIX_LEN];

	boot_wake_lock(1);

	/*
	** Save kernel log
	*/
	snprintf(reason, MAX_PREFIX_LEN, "cpcrash_%s_klog_before", Container::getCbdArgs()->cpn.rat);
	Log::save_logs(LOGB_DMESG, reason);

	cbd_info("Prepare CP crash dump\n");
	if (!std_dump_prepare_args()) {
		cbd_info("ERR! std_dump_prepare_args fail\n");
		ret = -EFAULT;
		goto exit;
	}

	if (!prepare_boot_args(CP_BOOT_MODE_DUMP)) {
		cbd_info("ERR! prepare_boot_args fail\n");
		ret = -EFAULT;
		goto exit;
	}

	/* Save srinfo from shmem to file */
	if (Container::getSrinfo())
		Container::getSrinfo()->store_srinfo(std_boot.fds[FD_DEV]);

	cbd_info("Get log dump\n");
	get_log_dump("shmem", LOG_IDX_SHMEM);
	if (Container::getCbdArgs()->options & BOPT_VERIFY_VSS)
		cbd_info("BOPT_VERIFY_VSS is enabled. VSS region is not accessible by cbd\n");
	else
		get_log_dump("vss", LOG_IDX_VSS);
	get_log_dump("databuf", LOG_IDX_DATABUF);
	get_log_dump("l2b", LOG_IDX_L2B);
	get_log_dump("acpm", LOG_IDX_ACPM);
	get_log_dump("cp_btl", LOG_IDX_CP_BTL);
	get_log_dump("ddm", LOG_IDX_DDM);

	cbd_info("Power reset CP_BOOT_MODE_DUMP\n");
	ret = std_boot_power_reset(CP_BOOT_MODE_DUMP);
	if (ret < 0) {
		cbd_info("ERR! std_boot_power_reset fail\n");
		goto exit;
	}

	cbd_info("Load CP bootloader\n");
	ret = load_cp_images(CP_BOOT_MODE_DUMP);
	if (ret < 0) {
		cbd_info("ERR! load_cp_images fail\n");
		goto exit;
	}

#ifndef LEGACY_IOCTL
	cbd_info("Request security : dump mode\n");
	ret = std_security_req(CP_BOOT_MODE_DUMP,
				   std_boot.dl_ctrl[TOC_BOOT].b_size,
				   std_boot.dl_ctrl[TOC_MAIN].b_size);
	if (ret < 0) {
		cbd_info("ERR! security check fail\n");
		goto exit;
	}
#endif

	cbd_info("Start CP bootloader for crash dump\n");
	ret = std_boot_start_cp_bootloader(CP_BOOT_MODE_DUMP);
	if (ret < 0) {
		cbd_info("ERR! std_boot_start_cp_bootloader fail\n");
		goto exit;
	}

	cbd_info("Receive CP crash dump\n");
	ret = std_dump_receive_cp_dump();
	if (ret < 0) {
		cbd_dump_info("ERR! std_dump_receive_cp_dump fail\n");
		goto exit;
	}

	snprintf(reason, MAX_PREFIX_LEN, "cpcrash_%s_klog_after", Container::getCbdArgs()->cpn.rat);
	Log::save_logs(LOGB_DMESG, reason);

	sync();

#ifdef CONFIG_PROTOCOL_SIT
	if (Log::debug_level == DBG_AUTO) {
		cbd_info("No trigger dump upload\n");
		ret = 0;
		goto exit;
	}
#endif

	cbd_info("Trigger dump upload\n");
	ret = std_dump_upload();
	if (ret < 0) {
		cbd_dump_info("ERR! std_dump_upload fail\n");
	}

exit:
	if (ret < 0) {
		snprintf(reason, MAX_PREFIX_LEN, "cpcrash_%s_dump_fail", Container::getCbdArgs()->cpn.rat);
		Log::save_logs(LOGB_DMESG, reason);
	}

	std_dump_close_args();
	std_boot_close_args();

	boot_wake_lock(0);

	return ret;
}

struct std_boot_args *BootDumpModap::getStdBoot()
{
	return &std_boot;
}

struct std_dump_args *BootDumpModap::getStdDump()
{
	return &std_dump;
}

void BootDumpModap::setCbdArgs(char *name)
{
	struct std_cbd_args *cbd_args = Container::getCbdArgs();
	char prop_buf[PROPERTY_VALUE_MAX] = {0, };

#ifdef CONFIG_PROTOCOL_SIT
	cbd_info("MODAP SIT modem\n");
	cbd_args->type = SEC_MODAP_SIT;
	sprintf(cbd_args->cpn.node_dump, "/dev/umts_boot0");
	sprintf(cbd_args->cpn.path_nv_norm, "/mnt/vendor/efs/nv_normal.bin");
	sprintf(cbd_args->cpn.path_nv_prot, "/mnt/vendor/efs/nv_protected.bin");
	cbd_args->cpn.num_stages = 6; /* toc, boot, main, vss, nv, nv_prot */
#else
	cbd_info("SS310 modem\n");
	cbd_args->type = SEC_SS310;
	sprintf(cbd_args->cpn.node_dump, "/dev/umts_ramdump0");
	cbd_args->cpn.num_stages = 5; /* toc, boot, main, vss, nv */
#endif

	property_get("ro.boot.slot_suffix", prop_buf, "");

	cbd_args->lnk_boot = LINKDEV_SHMEM;
	cbd_args->lnk_main = LINKDEV_SHMEM;
	sprintf(cbd_args->cpn.name, "%s", name);
	cbd_args->cpn.rat = "umts";
	sprintf(cbd_args->cpn.node_boot, "/dev/umts_boot0");
	cbd_args->cpn.node_status = cbd_args->cpn.node_boot;
	sprintf(cbd_args->cpn.path_bin, "/dev/block/by-name/modem%s", prop_buf);
	sprintf(cbd_args->cpn.path_nv_data, "/mnt/vendor/efs/nv_data.bin");
	cbd_args->cpn.nv_size = (512 << 10);
	cbd_args->cpn.first_stage_toc_type = TOC_TOC;
}

void BootDumpModap::boot_wake_lock(int lock)
{
	const char *path = lock ? "/sys/power/wake_lock" : "/sys/power/wake_unlock";
	const char *name = "ss310";
	int fd, ret;

	fd = open(path,  O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	if (fd < 0) {
		cbd_err("user wake_%s open fail\n", (lock ? "lock" : "unlock"));
		return;
	}
	ret = write(fd, name, strlen(name));
	if (ret < 0) {
		cbd_err("write fail - %s\n", name);
		goto exit;

	}
	cbd_info("%s/%s\n", path, name);
exit:
	if (fd >= 0)
		close(fd);
	return;
}

void BootDumpModap::build_std_dload_control()
{
	unsigned int idx, stage;
	struct std_dload_control *dl_ctrl = std_boot.dl_ctrl;
	struct std_toc_element *toc = std_boot.toc;

	for (idx = 0; idx < std_boot.num_stages + 1; idx++) {
		/* End of TOC */
		if (std_boot.toc_idx[TOC_OFFSET] == idx)
			break;

		if (idx < std_boot.start_stage)
			continue;

		stage = idx;

		dl_ctrl[stage].stage = stage;
		dl_ctrl[stage].start = 0;
		dl_ctrl[stage].download = 0;
		dl_ctrl[stage].validate = 0;
		dl_ctrl[stage].finish = 0;
		dl_ctrl[stage].b_fd = std_boot.fds[FD_BIN];
		dl_ctrl[stage].b_offset = toc[idx].b_offset;
		dl_ctrl[stage].m_offset = (toc[idx].m_offset & CP_MEMORY_MASK);
		dl_ctrl[stage].b_size = toc[idx].size;
		dl_ctrl[stage].crc = toc[idx].crc;
		dl_ctrl[stage].dl_once = 0;

		if (std_boot.toc_idx[TOC_VSS] == idx) {
			if (!(Container::getCbdArgs()->options & BOPT_REINIT_VSS))
				dl_ctrl[stage].dl_once = 1;
		}

		build_std_dload_control_nv_fd(stage, idx);

		cbd_info("stage=%u, name:%s b_off=0x%08x, m_offset=0x%08x b_size=0x%08x\n",
			dl_ctrl[stage].stage, toc[idx].name, dl_ctrl[stage].b_offset,
			dl_ctrl[stage].m_offset, dl_ctrl[stage].b_size);
	}
}

bool BootDumpModap::prepare_boot_args(enum cp_boot_mode mode)
{
	struct modem_comp *cpn = &(Container::getCbdArgs()->cpn);
	u32 toc_count = 0;

	/* Prepare BOOT arguments */
	if (!std_boot_prepare_args()) {
		cbd_info("ERR! std_boot_prepare_args fail\n");
		goto error;
	}

	if (!std_boot_parse_toc_img(mode, &toc_count))
		goto error;

	/*
	** Set binary stage information
	*/
	std_boot.start_stage = Container::getBootStage();

	if (toc_count == 1) {
		/* Support legacy TOC table */
		std_boot.num_stages = SHANNON_LEGACY_MAX_DL_STAGE;
	} else {
#ifdef CONFIG_SEC_CP_VERIFYING_ALL
		if (std_boot.toc[std_boot.toc_idx[TOC_TOC]].m_offset == 0) {
			/* Load TOC to cp dram for verification.
			   If TOC has no m_offset, it's not correct information
			   for TOC and VSS verification */
			cbd_info("ERR! invalid TOC : No info for TOC verification\n");
			goto error;
		}
		std_boot.start_stage = Container::getTocStage();
#endif
		std_boot.num_stages = toc_count - 1;
	}

	if (!cpn->path_nv_data[0]) {
		/* For wifi model */
		std_boot.num_stages = std::max(Container::getTocStage(), Container::getBootStage()) + 1;
	}

	if (mode == CP_BOOT_MODE_DUMP) {
		/* For Dump mode */
		std_boot.num_stages = Container::getBootStage();
	}

	/*
	** Set standard DLOAD control parameters with SHANNON BOOT arguments
	*/
	build_std_dload_control();

	return true;

error:
	std_boot_close_args();

	return false;
}

int BootDumpModap::load_cp_image_by_stage(u32 stage, enum cp_boot_mode mode)
{
	int ret = 0;
	int last = 0;
	struct std_dload_control *dlc = &std_boot.dl_ctrl[stage];
	struct cp_image img;
	unsigned total = 0;

	/* Prepare an image buffer */
	img.binary = (u8*)malloc(EXYNOS_PAYLOAD_LEN);
	if (!img.binary) {
		cbd_info("ERR! malloc(%d) fail\n", dlc->b_size);
		ret = -ENOMEM;
		goto exit;
	}
	img.size = dlc->b_size;
	img.m_offset = dlc->m_offset;
	img.b_offset = dlc->b_offset;
#ifdef LEGACY_IOCTL
	img.stage = stage - 1; /* kernel enum skips the TOC entry, therefore we are off by one */
#else
	img.mode = mode;
#endif
	img.len = EXYNOS_PAYLOAD_LEN;

	cbd_info("stage=%u(%u), b_off=0x%08x, m_off=0x%08x, b_size=0x%08x, mode=0x%08x\n",
		stage, dlc->stage, dlc->b_offset, dlc->m_offset, dlc->b_size, mode);

	ret = lseek(dlc->b_fd, img.b_offset, SEEK_SET);
	if (ret < 0) {
		cbd_err("ERR! lseek fail at stage %u\n", stage);
		goto exit;
	}

	while(1) {
		if(img.size == img.b_offset)
			break;

		if((img.size - total) < EXYNOS_PAYLOAD_LEN) {
			img.len = img.size - total;
			last = 1;
		}

		ret = read(dlc->b_fd, (void *)img.binary, img.len);
		if (ret < 0) {
			cbd_err("ERR! read fail at stage %u\n", stage);
			goto exit;
		}

		if ((u32)ret != img.len) {
			cbd_info("ERR! read %d != img.len %d\n", ret, img.len);
			ret = -EFAULT;
			goto exit;
		}

#ifdef LEGACY_IOCTL
		ret = ioctl(std_boot.fds[FD_DEV], IOCTL_XMIT_BIN, &img);
#else
		ret = ioctl(std_boot.fds[FD_DEV], IOCTL_LOAD_CP_IMAGE, &img);
#endif
		if (ret) {
			cbd_err("ERR! IOCTL_LOAD_CP_IMAGE fail (%u,%d)\n", stage, ret);
			goto exit;
		}

		if(last == 1)
			break;

		total += img.len;
		img.m_offset += img.len;
	}
	cbd_info("%u stage complelte\n", stage);

exit:
	if (img.binary)
		free(img.binary);

	return ret;
}

int BootDumpModap::load_cp_images(enum cp_boot_mode mode)
{
	int ret = 0;
	char prop_buf[PROPERTY_VALUE_MAX] = {0, };
	struct std_dload_control *dl_ctrl = std_boot.dl_ctrl;

	property_get(VPROP_FIRST_XMIT_DONE, prop_buf, "0");

	for (u32 stage = std_boot.start_stage; stage < std_boot.num_stages + 1; stage++) {
		if (dl_ctrl[stage].dl_once && prop_buf[0] == '1') {
			cbd_info("stage[%u] : stage is already xmit once\n", stage);
			continue;
		}

		if (dl_ctrl[stage].b_fd < 0) {
			cbd_info("fd is not valid. skip stage[%u]\n", stage);
			continue;
		}

		if (!dl_ctrl[stage].b_size) {
			cbd_info("b_size is not valid. skip stage[%u]\n", stage);
			continue;
		}

		ret = load_cp_image_by_stage(stage, mode);
		if(ret < 0) {
			cbd_info("ERR! load_cp_image_by_stage stage[%u] fail\n", stage);
			return ret;
		}
	}

	/* return xmit_boot state */
	ret = prop_buf[0] - '0';

	property_set(VPROP_FIRST_XMIT_DONE, "1");
	return ret;
}

int BootDumpModap::std_security_req(u32 mode, u32 p2, u32 p3)
{
	int ret;
	struct modem_sec_req msr;

	msr.mode = mode;
	msr.param2 = p2;
	msr.param3 = p3;
	msr.param4 = 0;

	cbd_info("security_req: %x:%x:%x:%x\n",
		msr.mode, msr.param2, msr.param3, msr.param4);

	ret = ioctl(std_boot.fds[FD_DEV], IOCTL_REQ_SECURITY, &msr);
	if (ret != 0) {
		cbd_err("ERR! IOCTL_CHECK_SECURITY fail (%d)\n", ret);

		/* 11 (CP not working) is an expected when mode == CP_BOOT_RE_INIT */
		if (mode == CP_BOOT_RE_INIT && ret == 11)
			return 0;

		if (std_check_cp_secure_fail(ret)) {
			cbd_info("secure err: Invalid Main image\n");
			if (Container::getProtocol()->check_csc_sales_code())
				Container::getProtocol()->std_reboot_system("secure");
		}

		return -1;
	}

	return 0;
}

int BootDumpModap::std_check_cp_secure_fail(u32 value)
{
	/*
	 * Only for ModAP model.
	 * CP Secure fail err code.
	 */
	u32 err_code[] = {
		0xFEED02,	/* Exynos3475 CP Boot */
		0xFEED04,	/* Exynos3475 CP Main */
		0xFEED0002,	/* Exynos7580, 8890 (EL3) */
		0x50E00,	/* RV_RSA_SIG_VERIFICATION_FAIL */
		0x50F01,	/* RV_ECDSA_SIG_VERIFICATION_FAIL */
	};

	int count = sizeof(err_code) / sizeof(u32);

	while (count--) {
		if (value == err_code[count])
			return 1;
	}

	return 0;
}

