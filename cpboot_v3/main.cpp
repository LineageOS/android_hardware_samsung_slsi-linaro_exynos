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
#ifdef CONFIG_PROTOCOL_SIT
#include "ProtocolSit.h"
#else
#include "ProtocolSipc.h"
#endif
#include "Container.h"
#include "Srinfo.h"
#include "Type.h"
#include "Util.h"
#include "Log.h"

#define CRASH_DUMP_RETRY_COUNT	5

#ifndef CONFIG_PROTOCOL_SIT
int shutdown()
{
	int ret = 0;
	char *node_boot = Container::getCbdArgs()->cpn.node_boot;
	int fd;

	fd = open(node_boot, O_RDWR | O_NDELAY);
	if (fd < 0) {
		cbd_err("%s open fail\n", node_boot);
		ret = -errno;
		goto exit;
	}

	ioctl(fd, IOCTL_POWER_OFF, NULL);
	close(fd);

exit:
	return ret;
}
#endif

#ifndef LEGACY_IOCTL
/*Read the property set by APK, and then set the btl size*/
static int adjust_btl_ramsize()
{
	char btl_buff[PROPERTY_VALUE_MAX] = {0};
	u32 btl_size, btl_size_M;
	int dev_fd, ret;

	// open device node
	dev_fd = open(PATH_BTL_NODE, O_RDWR);
	if (dev_fd < 0) {
		cbd_err("ERR! (%s) open fail\n", PATH_BTL_NODE);
		goto exit;
	}

	btl_size_M = property_get_int32(VPROP_BTL_SIZE, 0);
	btl_size = btl_size_M * SZ_1M;

	cbd_info("persist.vendor.cbd.btl_size 0x%08x\n", btl_size);

	ret = ioctl(dev_fd, IOCTL_SET_BTL_SIZE, &btl_size);
	if (ret < 0) {
		cbd_err("Failed to IOCTL_SET_BTL_SIZE\n");
	}

	ret = ioctl(dev_fd, IOCTL_GET_BTL_SIZE, &btl_size);
	if (ret < 0) {
		cbd_err("Failed to IOCTL_GET_BTL_SIZE\n");
		goto exit;
	}

	btl_size_M = btl_size / SZ_1M;
	snprintf(btl_buff, sizeof(btl_buff), "%d", btl_size_M);

exit:
	if (dev_fd > 0)
		close(dev_fd);

	ret = property_set(VPROP_BTL_SIZE, strlen(btl_buff) ? btl_buff : "");
	if (ret < 0) {
		cbd_err("Failed to set btl_size property to %s\n", btl_buff);
	}

	return ret;
}
#endif

static int status_loop()
{
	int fd = 0;
	int err, status;
	struct pollfd pfd = {
		.events = POLLHUP,
	};
#ifndef CONFIG_PROTOCOL_SIT
	char deviceOff[PROPERTY_VALUE_MAX];
#endif
	char cpboot_log[PROPERTY_VALUE_MAX];
	char suffix[MAX_SUFFIX_LEN];

	int dump_retry = CRASH_DUMP_RETRY_COUNT;

	if (!Container::getCbdArgs()->daemon) {
		cbd_info("Cp boot Oneshot\n");
		return -EBUSY;
	}

	cbd_info("CP status_loop start (modem = %d)\n", Container::getCbdArgs()->type);

	fd = open(Container::getCbdArgs()->cpn.node_status, O_RDWR);
	if (fd < 0) {
		cbd_err("%s open fail\n", Container::getCbdArgs()->cpn.node_status);

		/* if ipc0 node was not created, we can think cp was crass while
		* CP boot time, try get the dump and reset */
		if (Log::debug_level == DBG_LOW)
			goto exit;

		if (Log::debug_cp_opt == DBG_CP_NORMAL) {
			cbd_info("%s open fail, err = %d\n",
				Container::getCbdArgs()->cpn.node_status, fd);
			err = Container::getBootDump()->dump();
			if (err < 0)
				cbd_info("start_dump fail\n");
			goto exit;
		}
	}

	/* Set the property for checking CP reset, for CLD */
	property_get("vendor.cbd.cp_reset", cpboot_log, "0");
	snprintf(suffix, MAX_SUFFIX_LEN, "%d", atoi(cpboot_log) + 1);
	property_set("vendor.cbd.cp_reset", suffix);

	if (Log::debug_level != DBG_LOW && Log::debug_cp_opt == DBG_CP_NORMAL) {
		property_get(VPROP_CPBOOT_DONE, cpboot_log, "0");
		if (cpboot_log[0] == '1') {
			snprintf(suffix, MAX_SUFFIX_LEN, "cp_boot_done_%s",
					Container::getCbdArgs()->cpn.rat);
			/* cp was not first boot, save log*/
			Log::save_logs_thread(LOGB_DMESG, suffix);
#ifdef CONFIG_DUMP_LIMIT
			usleep(30000);/* waiting above save_logs_thread finish using dump limit property */
			Log::organize_dump_files(); /* erase previous dump files if necessary */
#endif
		}

		property_set(VPROP_CPRESET_DONE, "1");
	}

	/* Set the property for checking CP normal boot*/
	property_set(VPROP_CPBOOT_DONE, "1");

	pfd.fd = fd;

	if (Log::debug_level != DBG_LOW && Log::debug_cp_opt == DBG_CP_AUTORESET) {
		cbd_info("CP Silent reset repeat\n");
		err = poll(&pfd, 1, 50000);
		goto exit;
	}

	cbd_info("Wait event from modem type: %d\n", Container::getCbdArgs()->type);

	while (1) {
		pfd.revents = 0;
		err = poll(&pfd, 1, -1);
		if (!(pfd.revents & POLLHUP))
			continue;

		status = ioctl(fd, IOCTL_GET_CP_STATUS);
#ifndef CONFIG_PROTOCOL_SIT
		property_get(PROP_DEV_OFFREQ, deviceOff, "0");
		cbd_info("deviceOff = %s\n", deviceOff);
		if (deviceOff[0] == '1') {
			cbd_info("deviceOff = %s\n", deviceOff);
			cbd_info("shutdown\n");
			err = shutdown();

			/* M0 SKT workaround request - 2012-04-10
			 * sometimes, rild can't get the PHONE_ACTIVE event
			 * while waiting cp off.
			 */
			property_set(VPROP_DEV_OFFRES, "1");
			while(1)
				sleep(0xff);
		}
#endif

		cbd_info("get event %d\n", status);

		switch (status) {
		case STATE_CRASH_RESET:
			if (Log::debug_cp_opt == DBG_CP_FORCEPANIC) {
				err = Container::getBootDump()->forceUpload("Force upload");
				if (err)
					cbd_info("forceUpload() error %d\n", err);
			}

			if (Log::debug_cp_opt != DBG_CP_NORMAL)
				goto exit;

#ifndef CONFIG_PROTOCOL_SIT
			cbd_info("STATE_CRASH_RESET, wait onrestart by rild\n");
			sleep(3);
#endif
			err = ioctl(fd, IOCTL_POWER_OFF);
			if (err)
				cbd_info("cp off ioctl fail err=%d\n", err);
#ifndef CONFIG_PROTOCOL_SIT
			cbd_info("RILD dosenot restart for 3sec, save logs\n\n");
			if (Log::debug_level != DBG_LOW) {
				snprintf(suffix, MAX_SUFFIX_LEN, "cbd_only_%s",
						Container::getCbdArgs()->cpn.rat);
				Log::save_logs_thread(LOGB_DMESG, suffix);
			}
			sleep(27);
#endif
			goto exit;
			break;

		case STATE_CRASH_WATCHDOG:
		case STATE_CRASH_EXIT:
			Log::check_debug_level();
			if (Log::debug_cp_opt == DBG_CP_FORCEPANIC) {
				err = Container::getBootDump()->forceUpload("Force upload");
				if (err)
					cbd_info("forceUpload() error %d\n", err);
			}


			if (Log::debug_level == DBG_LOW) {
#ifndef CONFIG_PROTOCOL_SIT
				/* In case of debug level low, cbd should wait for rild
				 * to kill current cbd process and start new cbd process */
				cbd_info("DBG_LOW, wait onrestart by rild\n");
				sleep(3);
#endif
				goto exit;
			}

			if (Log::debug_cp_opt != DBG_CP_NORMAL)
				goto exit;

			cbd_info("CP status CRASH\n");

			/* save CP RAMDUMP */
			err = Log::create_log_directory();
			if (err == -ENOSPC) {
				err = Container::getBootDump()->forceUpload("Not enough freespace");
				if (err)
					cbd_info("forceUpload() error %d\n", err);
			} else if (err) {
				goto exit;
			}

			while (Container::getBootDump()->dump() < 0) {
				cbd_info("start_dump fail\n");
				if (dump_retry-- < 0)
					break;
				sleep(1);
			}

			cbd_info("CP status RESET\n");

			/* Restart boot daemon */
			goto exit;

		case STATE_NV_REBUILDING:
		default:
			cbd_info("unknown Modem status\n");
			continue;
		}
	};

exit:
	/*
	 * If this process was start for boot daemon, below code will be exit
	 * the process and will be restart by daemon service.
	 */
	if (fd > 0)
		close(fd);

	cbd_info("status loop exit\n");
	return 0;
}

int main(int argc, char **argv)
{
	int err;
	Status status;
	char suffix[MAX_SUFFIX_LEN];
#ifndef CONFIG_PROTOCOL_SIT
	char deviceOff[PROPERTY_VALUE_MAX];
#endif
#if defined(CONFIG_THROUGHPUT_MONITOR)
	pthread_t tm_thread;
#endif

	umask(2);
	Log::kprintf_init();
	Log::check_debug_cp_opt();
	Log::check_debug_level();
	Log::update_log_dir();

	cbd_info("Start CP Boot Daemon v3 (CBD) %s\n", Log::get_cbd_version());

#ifdef CONFIG_PROTOCOL_SIT
	Container::setProtocol(new ProtocolSit());
#else
	Container::setProtocol(new ProtocolSipc());
#endif

	status = Container::parseArgs(argc, argv);
	switch (status) {
	case OK:
		break;
	default:
		return -EINVAL;
	}

	if (!Container::getCbdArgs()->type) {
		cbd_info("Invaild modem type %d\n", Container::getCbdArgs()->type);
		/* if boot daemon was started with init.rc service, below loop
		 * will not restart boot daemon
		 */
		while(1);
		goto exit;
	}

	if (Container::getCbdArgs()->options & BOPT_CPUPLOAD) {
		cbd_info("\n\n ---- upload ----\n\n");
		sleep(5);
		err = Container::getBootDump()->dump();
		if (err < 0) {
			cbd_info("start boot fail\n");
			goto exit;
		}
		goto exit;
	}

	if (Container::getCbdArgs()->options & BOPT_ROOT)
		cbd_info("Start with root\n");
	else
		Util::switch_user();

#ifndef LEGACY_IOCTL
	err = adjust_btl_ramsize();
	if (err < 0) {
		cbd_err("adjust btl ramsize fail\n");
		goto exit;
	}
#endif

__cpboot_retry:
#ifndef CONFIG_PROTOCOL_SIT
	property_get(PROP_DEV_OFFREQ, deviceOff, "0");
	if (deviceOff[0] == '1') {
		cbd_info("deviceOff = %s\n", deviceOff);
		err = shutdown();
		while(1)
			sleep(0xff);
	}
#endif

	/* CP debugging */
	if (Log::debug_level != DBG_LOW) {
		char cpboot[PROPERTY_VALUE_MAX];
		switch (Log::debug_cp_opt) {
		case DBG_CP_NOBOOT:
			cbd_info("CP_DEBUG NOBOOT\n");
			goto skip_cpboot;
			break;
		case DBG_CP_NORESET:
			property_get(VPROP_CPBOOT, cpboot, "");
			if (cpboot[0] == '1') {
				cbd_info("CP_DEBUG NORESET\n");
				goto skip_cpboot;
			} else {
				cbd_info("CP_DEBUG NORESET prop set\n");
				property_set(VPROP_CPBOOT, "1");
			}
			break;
		default:
			break;
		}

		/* save cp reset log once */
		property_get(VPROP_CPRESET_DONE, cpboot, "");
		if (cpboot[0] == '1') {
			snprintf(suffix, MAX_SUFFIX_LEN, "cp_boot_%s",
					Container::getCbdArgs()->cpn.rat);
			Log::save_logs_thread(LOGB_DMESG, suffix);
			property_set(VPROP_CPRESET_DONE, "0");
		}
	}

	/* call start boot code */
	cbd_info("Call boot start\n");
	cbd_info("Modem type = %d\n", Container::getCbdArgs()->type);
	cbd_info("Boot link = %d\n", Container::getCbdArgs()->lnk_boot);
	cbd_info("Main link = %d\n", Container::getCbdArgs()->lnk_main);

	err = Container::getBootDump()->boot();
	if (err < 0) {
		cbd_info("start boot fail\n");
		if (Log::debug_level != DBG_LOW && Log::debug_cp_opt == DBG_CP_NORMAL) {
			if (Container::getBootDump()->get_modem_state(-1) == STATE_CRASH_EXIT ||
					Container::getBootDump()->get_modem_state(-1) == STATE_CRASH_WATCHDOG) {
				cbd_info("CP Crash when CP booting..\n");
				err = Log::create_log_directory();
				if (err == -ENOSPC) {
					err = Container::getBootDump()->forceUpload("Not enough freespace");
					if (err)
						cbd_info("forceUpload() error %d\n", err);
				} else if (err){
					goto exit;
				}
				Container::getBootDump()->dump();
			}
		}
		goto exit;
	}

skip_cpboot:
	cbd_info("Boot up process done!!!\n");

#if defined(CONFIG_THROUGHPUT_MONITOR)
	err = pthread_create(&tm_thread, NULL, Util::traffic_monitor, (void*)NULL);
	if (err != 0) {
		cbd_info("Fail to create traffic monitor thread!!!\n");
	} else {
		cbd_info("Success to create traffic monitor thread!!!\n");
	}
#endif

	if (status_loop())
		goto deinit;

exit:
	if (Log::debug_cp_opt == DBG_CP_NOCRASH) {
		cbd_info("hang cbd for debugging\n");
		while(1)
			sleep(0xff);
	} else
		sleep(1);

#ifdef CONFIG_PROTOCOL_SIT
	property_set(VPROP_RILD_RESET, "1");
#endif
	goto __cpboot_retry;

deinit:
#ifdef CONFIG_PROTOCOL_SIT
	property_set(VPROP_RILD_RESET, "1");
#endif

	Log::kprintf_deinit();
	return 0;
}

