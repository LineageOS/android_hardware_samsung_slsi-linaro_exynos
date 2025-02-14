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

#include "BootDump.h"
#include "Container.h"
#include "Protocol.h"

#define SIM_CONF_KERN_PATH	"/sys/devices/platform/cpif/sim/ds_detect"

BootDump::BootDump() {}

int BootDump::upload(int fd2)
{
	int fd = fd2;
	int ret = -1;

	if (fd < 0) {
		char *node_boot = Container::getCbdArgs()->cpn.node_boot;

		fd = open(node_boot, O_RDWR | O_NDELAY);
		if (fd < 0) {
			cbd_err("%s open fail\n", node_boot);
			goto exit;
		}
	}

	cbd_info("Go to UPLOAD mode\n");

	ret = ioctl(fd, IOCTL_TRIGGER_KERNEL_PANIC, Container::getCbdArgs()->reason);

exit:
	if (fd2 < 0 && fd >= 0)
		close(fd);

	return ret;
}

int BootDump::forceUpload(const char *reason)
{
	snprintf(Container::getCbdArgs()->reason, CRASH_REASON_SIZE, "%s", reason);

	return upload(-1);
}

void BootDump::set_sim_configuration(void)
{
	int sim_count = Container::getProtocol()->get_factory_prop();

	cbd_info("sim count: %d (echo to ds_detect file)\n", sim_count);

	if (sim_count > 0 && sim_count <= 4) {
		char cmd_string[4] = {0, };
		int fd, ret;

		fd = open(SIM_CONF_KERN_PATH, O_CREAT | O_WRONLY | O_NOFOLLOW,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
		if (fd < 0) {
			cbd_err("SIM conf: open failed\n");
			return;
		}

		sprintf(cmd_string, "%d", sim_count);
		ret = write(fd, cmd_string, strlen(cmd_string));
		if (ret < 0) {
			cbd_err("SIM conf: write failed\n");
			close(fd);
			return;
		}
		fsync(fd);
		close(fd);
	}
}

int BootDump::get_log_dump(const char *name, enum cp_log_dump_index idx)
{
	int ret;
	unsigned long copied = 0;
	char buf[PAGE_SIZE];
	int dump_fd = -1;
	char path[MAX_PATH_LEN];
	char suffix[MAX_SUFFIX_LEN];
	time_t now;
	struct tm result;
	struct cp_log_dump log_dump;

	/* Get size and trigger log dump */
	memset(&log_dump, 0, sizeof(log_dump));
	strncpy(log_dump.name, name, sizeof(log_dump.name) - 1);
	log_dump.idx = idx;
	cbd_info("name:%s idx:%u\n", log_dump.name, log_dump.idx);
	ret = ioctl(getStdDump()->fds[FD_DEV], IOCTL_GET_LOG_DUMP, &log_dump);
	if (ret < 0) {
		cbd_dump_err("ERR! ioctl fail\n");
		return -EINVAL;
	}
	cbd_dump_info("size:%u\n", log_dump.size);

	/* Open a log dump file */
	time(&now);
	localtime_r(&now, &result);
	strftime(suffix, MAX_SUFFIX_LEN, "%Y%m%d-%H%M", &result);
	sprintf(path, "%s/cpcrash_%s_dump_%s_%s.log", Log::get_log_dir(), name, Container::getCbdArgs()->cpn.rat, suffix);

	dump_fd = open(path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (dump_fd < 0) {
		cbd_dump_err( "ERR! %s open fail\n", path);
		return -EINVAL;
	}
	cbd_dump_info( "%s opened (fd %d)\n", path, dump_fd);

	/* Read & save log dump */
	while (copied < log_dump.size) {
		ret = Container::getProtocol()->std_ul_rx_frame(buf, sizeof(buf));
		if (ret < 0) {
			cbd_dump_info("ERR! log_dump std_ul_rx_frame fail (ret %d)\n", ret);
			goto exit;
		}

		/* not verified */
		copied += ret;

		ret = write(dump_fd, buf, ret);
		if (ret < 0) {
			cbd_dump_err("ERR! log_dump write fail\n");
			goto exit;
		}
	}

	cbd_dump_info("%s Complete! (%lu bytes)\n", path, copied);
exit:
	if (dump_fd >= 0)
		close(dump_fd);

	return ret;
}

int BootDump::get_modem_state(int fd2)
{
	int fd = fd2;
	int status = -1;

	if (fd < 0) {
		char *node_boot = Container::getCbdArgs()->cpn.node_boot;

		fd = open(node_boot, O_RDWR);
		if (fd < 0) {
			cbd_err("%s open fail\n", node_boot);
			goto exit;
		}
	}

	status = ioctl(fd, IOCTL_GET_CP_STATUS);

	cbd_info("modem_status: %d\n", status);

exit:
	if (fd2 < 0 && fd >= 0)
		close(fd);

	return status;
}

int BootDump::std_boot_power_on()
{
	int ret;

#ifdef LEGACY_IOCTL
	int sim_slot_cnt = 2;
	ret = ioctl(getStdBoot()->fds[FD_DEV], IOCTL_POWER_ON, &sim_slot_cnt);
#else
	ret = ioctl(getStdBoot()->fds[FD_DEV], IOCTL_POWER_ON, NULL);
#endif
	if (ret < 0) {
		cbd_err("ERR! IOCTL_POWER_ON fail (%d)\n", ret);
		goto exit;
	}

	return 0;

exit:
	return ret;
}

int BootDump::std_boot_power_reset(enum cp_boot_mode mode_idx)
{
	int ret;
	int dev_fd = getStdBoot()->fds[FD_DEV];
	struct boot_mode mode;

	mode.idx = mode_idx;
	ret = ioctl(dev_fd, IOCTL_POWER_RESET, &mode);
	if (ret < 0) {
		cbd_err("ERR! IOCTL_POWER_RESET fail\n");
		goto exit;
	}

	return 0;

exit:
	return ret;
}

int BootDump::std_boot_start_cp_bootloader(enum cp_boot_mode mode_idx)
{
	struct boot_mode mode = {.idx = mode_idx};
	int ret;

#ifdef LEGACY_IOCTL
	ret = ioctl(getStdBoot()->fds[FD_DEV], IOCTL_MODEM_BOOT_ON, NULL);
#else
	ret = ioctl(getStdBoot()->fds[FD_DEV], IOCTL_START_CP_BOOTLOADER, &mode);
#endif
	if (ret < 0) {
		cbd_err("ERR! IOCTL_START_CP_BOOTLOADER fail\n");
		goto exit;
	}

	return 0;

exit:
	return ret;
}

int BootDump::std_boot_complete_normal_bootup()
{
	int ret;

	ret = ioctl(getStdBoot()->fds[FD_DEV], IOCTL_COMPLETE_NORMAL_BOOTUP, NULL);
	if (ret < 0) {
		cbd_err("ERR! IOCTL_COMPLETE_NORMAL_BOOTUP fail (%d)\n", ret);
		ioctl(getStdBoot()->fds[FD_DEV], IOCTL_GET_CP_BOOTLOG, NULL);
		goto exit;
	}

	ioctl(getStdBoot()->fds[FD_DEV], IOCTL_CLR_CP_BOOTLOG, NULL);

	return 0;

exit:
	return ret;
}

void BootDump::build_std_dload_control_nv_fd(unsigned int stage, unsigned int idx)
{
	struct std_dload_control *dl_ctrl = getStdBoot()->dl_ctrl;

#ifdef CONFIG_PROTOCOL_SIT
	if (getStdBoot()->toc_idx[TOC_NV_NORM] == idx)
		dl_ctrl[stage].b_fd = getStdBoot()->fds[FD_NV_NORM];
	else if (getStdBoot()->toc_idx[TOC_NV_PROT] == idx)
		dl_ctrl[stage].b_fd = getStdBoot()->fds[FD_NV_PROT];
	else if (getStdBoot()->toc_idx[TOC_NV] == idx)
		dl_ctrl[stage].b_fd = -1;
#else
	if (getStdBoot()->toc_idx[TOC_NV_NORM] == idx)
		dl_ctrl[stage].b_fd = -1;
	else if (getStdBoot()->toc_idx[TOC_NV_PROT] == idx)
		dl_ctrl[stage].b_fd = -1;
	else if (getStdBoot()->toc_idx[TOC_NV] == idx)
		dl_ctrl[stage].b_fd = getStdBoot()->fds[FD_NV_DATA];
#endif
}

void BootDump::std_boot_init_args()
{
	memset(getStdBoot(), 0, sizeof(struct std_boot_args));

	for (int i = 0; i < ARRAY_SIZE(getStdBoot()->fds); i++)
		getStdBoot()->fds[i] = -1;

	for (int i = 0; i < ARRAY_SIZE(getStdBoot()->toc_idx); i++)
		getStdBoot()->toc_idx[i] = -1;
}

bool BootDump::std_boot_prepare_args()
{
	int dev_fd = -1;
	struct modem_comp *cpn = &(Container::getCbdArgs()->cpn);

	std_boot_init_args();

	/* Open the boot device */
	dev_fd = open(cpn->node_boot, O_RDWR);
	if (dev_fd < 0) {
		cbd_err("ERR! DEV(%s) open fail\n", cpn->node_boot);
		return false;
	}

	cbd_info("DEV(%s) opened (fd %d)\n", cpn->node_boot, dev_fd);
	getStdBoot()->fds[FD_DEV] = dev_fd;
	getStdBoot()->num_stages = cpn->num_stages;

	return true;
}

bool BootDump::std_boot_parse_toc_img(enum cp_boot_mode mode, u32 *toc_count)
{
	int ret;
	int fd = -1;
	struct modem_comp *cpn = &(Container::getCbdArgs()->cpn);
	struct std_toc_element *toc;
	u32 nv_size[2] = {0};
	char *nv_path[2] = {NULL};

	/*
	 * Open CP binary file
	 */
	fd = open(cpn->path_bin, O_RDONLY);
	if(fd < 0) {
		cbd_err("ERR! BIN(%s) open fail\n", cpn->path_bin);
		return false;
	}
	cbd_info("BIN(%s) opened (fd %d)\n", cpn->path_bin, fd);
	getStdBoot()->fds[FD_BIN] = fd;

	/*
	 * Load and check TOC
	 */
	toc = getStdBoot()->toc;
	ret = read(getStdBoot()->fds[FD_BIN], toc, sizeof(getStdBoot()->toc));
	if (ret < 0) {
		cbd_err("ERR! TOC read fail\n");
		return false;
	}

	/* The 1st toc should be TOC */
	if (strcmp(toc[0].name, "TOC")) {
		char tocHexStr[sizeof(*toc)*3] = {0};

		Util::byteToHex(tocHexStr, sizeof(tocHexStr), (char *)toc, ret);
		cbd_info("ERR! invalid TOC: No TOC, content: %s\n", tocHexStr);
		return false;
	}

	if (toc[0].toc_count > MAX_TOC_INDEX) {
		cbd_info("ERR! invalid TOC: Total TOC count is %d\n",
			toc[0].toc_count);
		return false;
	}

	*toc_count = toc[0].toc_count;

	for (int i = 0; i < ARRAY_SIZE(getStdBoot()->toc); i++) {
		if (!strcmp(toc[i].name, "TOC"))
			getStdBoot()->toc_idx[TOC_TOC] = i;
		else if (!strcmp(toc[i].name, "BOOT"))
			getStdBoot()->toc_idx[TOC_BOOT] = i;
		else if (!strcmp(toc[i].name, "MAIN"))
			getStdBoot()->toc_idx[TOC_MAIN] = i;
		else if (!strcmp(toc[i].name, "VSS"))
			getStdBoot()->toc_idx[TOC_VSS] = i;
		else if (!strcmp(toc[i].name, "NV_NORM"))
			getStdBoot()->toc_idx[TOC_NV_NORM] = i;
		else if (!strcmp(toc[i].name, "NV_PROT"))
			getStdBoot()->toc_idx[TOC_NV_PROT] = i;
		else if (!strcmp(toc[i].name, "NV"))
			getStdBoot()->toc_idx[TOC_NV] = i;
		else if (!strcmp(toc[i].name, "OFFSET"))
			getStdBoot()->toc_idx[TOC_OFFSET] = i;

		if (!toc[i].name[0])
			continue;

		cbd_info("TOC[%d].name = %s, b_off=0x%08x, m_off=0x%08x, size=0x%08x crc=0x%08x\n",
			i, toc[i].name, toc[i].b_offset, toc[i].m_offset,
			toc[i].size, toc[i].crc);
	}

	/*
	 * Open NV data file
	 */
#ifdef CONFIG_PROTOCOL_SIT
	nv_path[0] = cpn->path_nv_norm;
	if (getStdBoot()->toc_idx[TOC_NV_NORM] >= 0) {
		nv_size[0] = toc[getStdBoot()->toc_idx[TOC_NV_NORM]].size;
		if (!Util::getNvFd(mode, nv_path[0], nv_size[0], &fd))
			return false;
		getStdBoot()->fds[FD_NV_NORM] = fd;
	}

	nv_path[1] = cpn->path_nv_prot;
	if (getStdBoot()->toc_idx[TOC_NV_PROT] >= 0) {
		nv_size[1] = toc[getStdBoot()->toc_idx[TOC_NV_PROT]].size;
		if (!Util::getNvFd(mode, nv_path[1], nv_size[1], &fd))
			return false;
		getStdBoot()->fds[FD_NV_PROT] = fd;
	}
#else
	nv_path[0] = cpn->path_nv_data;
	if (getStdBoot()->toc_idx[TOC_NV] >= 0) {
		nv_size[0] = toc[getStdBoot()->toc_idx[TOC_NV]].size;
		if (!Util::getNvFd(mode, nv_path[0], nv_size[0], &fd))
			return false;
		getStdBoot()->fds[FD_NV_DATA] = fd;
	}
#endif

	if ((mode != CP_BOOT_MODE_DUMP) && nv_path[0] && !nv_size[0]) {
		cbd_info("ERR! invalid TOC : There is no NV\n");
		return false;
	}

	return true;
}

void BootDump::std_boot_close_args()
{
	for (int i = 0; i < FD_MAX; i++) {
		if (getStdBoot()->fds[i] >= 0)
			close(getStdBoot()->fds[i]);
	}

	std_boot_init_args();
}

void BootDump::std_dump_init_args()
{
	memset(getStdDump(), 0, sizeof(struct std_dump_args));

	for (int i = 0; i < FD_MAX; i++)
		getStdDump()->fds[i] = -1;
}

bool BootDump::std_dump_prepare_args()
{
	struct modem_comp *cpn = &(Container::getCbdArgs()->cpn);
	int fd = -1, err;
	char path[MAX_PATH_LEN];
	char prefix[MAX_PREFIX_LEN];
	char suffix[MAX_SUFFIX_LEN];
	time_t now;
	struct tm result;

#ifdef CONFIG_DUMP_LIMIT
	/* remove garbage directory if necessary */
	Log::remove_directory(Log::get_log_dir());
#endif

	err = Log::create_log_directory();
	if (err)
		goto exit;

	std_dump_init_args();

	Util::toUpperCase((char *)cpn->rat, getStdDump()->reason);
	strcat(getStdDump()->reason, ": ");

	/* Open the DUMP device */
	fd = open(cpn->node_dump, O_RDWR);
	if (fd < 0) {
		cbd_err("ERR! %s open fail\n", cpn->node_dump);
		goto exit;
	}
	cbd_info("%s opened (fd %d)\n", cpn->node_dump, fd);
	getStdDump()->fds[FD_DEV] = fd;

	/* Set prefix and suffix for DUMP file paths */
	snprintf(prefix, MAX_PREFIX_LEN, "cpcrash_%s", cpn->rat);
	time(&now);
	localtime_r(&now, &result);
	strftime(suffix, MAX_SUFFIX_LEN, "%Y%m%d-%H%M", &result);

	for (int i = 0; i < FD_MAX; i++) {
		switch (i) {
		case FD_LOG:
			sprintf(path, "%s/%s_log_%s.log", Log::get_log_dir(), prefix, suffix);
			break;
		case FD_INFO:
			sprintf(path, "%s/%s_info_%s_%s.log", Log::get_log_dir(), prefix, cpn->name, suffix);
			break;
		case FD_DUMP:
			sprintf(path, "%s/%s_dump_%s.log", Log::get_log_dir(), prefix, suffix);
			break;
		default:
			continue;
		}

		fd = open(path, O_WRONLY | O_CREAT,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		if (fd < 0) {
			cbd_dump_err("ERR! %s open fail\n", path);
			goto exit;
		}

		fchmod(fd, 0664);
		getStdDump()->fds[i] = fd;

		if (i == FD_LOG)
			std_dump_write_versions();
		cbd_dump_info("%s opened (fd %d)\n", path, getStdDump()->fds[i]);
	}

	return true;

exit:
	return false;
}

void BootDump::std_dump_close_args()
{
	for (int i = 0; i < FD_MAX; i++) {
		if (getStdDump()->fds[i] >= 0)
			close(getStdDump()->fds[i]);
	}

	std_dump_init_args();
}

int BootDump::std_dump_receive_cp_dump()
{
	return Container::getProtocol()->std_dump_receive_cp_dump();
}

int BootDump::std_dump_upload()
{
	/* sync and close files */
	if (getStdDump()->fds[FD_DUMP] >= 0) {
		fsync(getStdDump()->fds[FD_DUMP]);
		close(getStdDump()->fds[FD_DUMP]);
		getStdDump()->fds[FD_DUMP] = -1;
	}

	if (getStdDump()->fds[FD_INFO] >= 0) {
		fsync(getStdDump()->fds[FD_INFO]);
		close(getStdDump()->fds[FD_INFO]);
		getStdDump()->fds[FD_INFO] = -1;
	}

	if (fsync(getStdDump()->fds[FD_LOG])) {
		cbd_dump_err("ERR! fsync(log_fd) fail\n");
		return -1;
	}

	return upload(getStdDump()->fds[FD_DEV]);
}

int BootDump::std_dump_write_versions()
{
	struct cpif_version version;
	int ret;

	ret = ioctl(getStdDump()->fds[FD_DEV], IOCTL_GET_CPIF_VERSION, &version);
	if (ret < 0) {
		cbd_dump_err("ERR! IOCTL_GET_CPIF_VERSION fail\n");
		return -EINVAL;
	}

	cbd_dump_info("CPIF version: %s\n", version.string);
	cbd_dump_info("CP Boot Daemon (CBD) version: %s\n", Log::get_cbd_version());

	return 0;
}

