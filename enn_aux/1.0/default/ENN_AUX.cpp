/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <sys/mman.h>             // mmap
#include <sys/types.h>            // pid_t
#include <errno.h>                // errno
#include <unistd.h>               // access
#include <signal.h>               // kill
#include <thread>
#include <map>                    // map
#include <mutex>
#include <shared_mutex>
#include <algorithm>              // remove
#include <system_error>

//#include <android/sysprop/PlatformProperties.sysprop.h>
#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include "ENN_AUX.h"
#include "log.h"
#define LOG_TAG "SERVICE"

#define CLRHIGH(u64)    ((u64) & 0x00000000FFFFFFFF)
#define LOWU32(u64)     ((uint32_t)((u64) & 0x00000000FFFFFFFF))
#define HIGHU32(u64)    ((uint32_t)((u64) >> 32))


namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace enn_aux {
namespace V1_0 {
namespace implementation {

using ::vendor::samsung_slsi::hardware::enn_aux::V1_0::IENN_AUX;
using ::android::hardware::hidl_memory;
using ::android::hidl::memory::V1_0::IMemory;
//using ::android::sysprop;

int32_t timeout = TIMEOUT_GPU_BOOST;
bool flag_thread_run = false;
std::mutex gpu_boost_mtx_;

std::string GetDeviceFreq(DeviceProcessingUnit unit, DeviceFreq min_max, int cpu_id) {
    auto read_min_max_freq = [](const char *sysfs_node, std::string *min, std::string *max) {
        char buffer[4096] = {};
        int fd = open(sysfs_node, O_RDONLY);
        if(-1 != fd)
        {
            int ret = read(fd, buffer, sizeof(buffer));
            if (-1 == ret) {
                LOGE(EDEN_HIDL, "[SYSFS] Reading %s failed.\n", sysfs_node);
            }
            buffer[ret-1] = '\0';
            close(fd);
        }
        else
            LOGE(EDEN_HIDL, "[SYSFS] Failed to open %s\n", sysfs_node);
        std::string buffer_str = buffer;
        if (buffer_str.size()==0) {
            LOGE(EDEN_HIDL, "[SYSFS] File contents invalid %s\n", sysfs_node);
        } else if (buffer_str.substr(buffer_str.size()-1, 1)==" ") {
            buffer_str = buffer_str.substr(0, buffer_str.size()-1);
        }
        LOGD(EDEN_HIDL, "[SYSFS] buffer_str %s\n", buffer_str.c_str());

        // find min and max freq
        std::size_t pos0 = buffer_str.find(" ");
        std::string freq0 = buffer_str.substr(0, pos0);
        std::size_t pos1 = buffer_str.rfind(" ");
        std::string freq1 = buffer_str.substr(pos1 + 1, buffer_str.length());
        if (atoi(freq1.c_str()) > atoi(freq0.c_str())) {
            *max = freq1;
            *min = freq0;
        } else {
            *max = freq0;
            *min = freq1;
        }
        LOGD(EDEN_HIDL, "[SYSFS] Min Freq %s. Max Freq %s\n", min->c_str(),max->c_str());
    };
    std::string min_freq, max_freq;
    switch (unit) {
    case DeviceProcessingUnit::CPU_LITTLE: read_min_max_freq(CPU_LTL_FREQ_TABLE, &min_freq, &max_freq); break;
    case DeviceProcessingUnit::CPU_MID: read_min_max_freq(CPU_MID_FREQ_TABLE, &min_freq, &max_freq); break;
    case DeviceProcessingUnit::CPU_BIG: read_min_max_freq(CPU_BIG_FREQ_TABLE, &min_freq, &max_freq); break;
    case DeviceProcessingUnit::AMD_CPU_LITTLE: read_min_max_freq(AMD_CPU_LTL_FREQ_TABLE, &min_freq, &max_freq); break;
    case DeviceProcessingUnit::AMD_CPU_MID: read_min_max_freq(AMD_CPU_MID_FREQ_TABLE, &min_freq, &max_freq); break;
    case DeviceProcessingUnit::AMD_CPU_BIG: read_min_max_freq(AMD_CPU_BIG_FREQ_TABLE, &min_freq, &max_freq); break;
    case DeviceProcessingUnit::GPU: read_min_max_freq(GPU_FREQ_TABLE, &min_freq, &max_freq); break;
    default: LOGW(EDEN_HIDL, "Unknown device type : %d.", static_cast<int>(unit)); break;
    }
    return (min_max == DeviceFreq::MAX ? max_freq : min_freq);
}

int WriteSysfs(const char *sysfs_node, const std::string &value) {
    int fd = open(sysfs_node, O_WRONLY);
    int ret = write(fd, value.c_str(), value.length());
    if (ret == -1) {
        LOGW(EDEN_HIDL, "[SYSFS] Writing to %s failed.", sysfs_node);
    }

    close(fd);
    return ret;
}

void SetDeviceFreq(DeviceProcessingUnit unit, DeviceFreq min_max) {
    auto write_freq = [](const char *min_sysfs,
                         const char *max_sysfs,
                         const std::string &min,
                         const std::string &max,
                         DeviceFreq min_max) {
        if (min_max == DeviceFreq::MAX) {
            // set max-freq
            WriteSysfs(min_sysfs, max);
            WriteSysfs(max_sysfs, max);
        } else {
            WriteSysfs(min_sysfs, min);
        }
    };
    switch (unit) {
    case DeviceProcessingUnit::CPU_LITTLE:
        write_freq(CPU_LTL_FREQ_SCALE_MIN, CPU_LTL_FREQ_SCALE_MAX, CpuLittleMinFreq(), CpuLittleMaxFreq(), min_max);
        break;
    case DeviceProcessingUnit::CPU_MID:
        write_freq(CPU_MID_FREQ_SCALE_MIN, CPU_MID_FREQ_SCALE_MAX, CpuMidMinFreq(), CpuMidMaxFreq(), min_max);
        break;
    case DeviceProcessingUnit::CPU_BIG:
        write_freq(CPU_BIG_FREQ_SCALE_MIN, CPU_BIG_FREQ_SCALE_MAX, CpuBigMinFreq(), CpuBigMaxFreq(), min_max);
        break;
    case DeviceProcessingUnit::AMD_CPU_LITTLE:
        write_freq(AMD_CPU_LTL_FREQ_SCALE_MIN, AMD_CPU_LTL_FREQ_SCALE_MAX, AmdCpuLittleMinFreq(), AmdCpuLittleMaxFreq(), min_max);
        break;
    case DeviceProcessingUnit::AMD_CPU_MID:
        write_freq(AMD_CPU_MID_FREQ_SCALE_MIN, AMD_CPU_MID_FREQ_SCALE_MAX, AmdCpuMidMinFreq(), AmdCpuMidMaxFreq(), min_max);
        break;
    case DeviceProcessingUnit::AMD_CPU_BIG:
        write_freq(AMD_CPU_BIG_FREQ_SCALE_MIN, AMD_CPU_BIG_FREQ_SCALE_MAX, AmdCpuBigMinFreq(), AmdCpuBigMaxFreq(), min_max);
        break;
    default: LOGW(EDEN_HIDL, "Unsupported Device: %d", static_cast<int>(unit)); break;
    }
}
void boost_off_func() {
    if(SoC::OLYMPUS == device) {
       // cpu sysfs
       SetDeviceFreq(DeviceProcessingUnit::CPU_LITTLE, DeviceFreq::MIN);
       SetDeviceFreq(DeviceProcessingUnit::CPU_MID, DeviceFreq::MIN);
    } else if(SoC::PAPAYA == device) {
       // cpu sysfs
       SetDeviceFreq(DeviceProcessingUnit::CPU_LITTLE, DeviceFreq::MIN);
    } else if(SoC::PAMIR == device) {
       // cpu sysfs
       SetDeviceFreq(DeviceProcessingUnit::AMD_CPU_LITTLE, DeviceFreq::MIN);
       SetDeviceFreq(DeviceProcessingUnit::AMD_CPU_MID, DeviceFreq::MIN);
    }
}

void boost_on_func() {
    LOGD(EDEN_HIDL, "%s: started", __func__);
    while (1) {
        std::string delay_val = "200";
        if(SoC::OLYMPUS == device) {
           // cpu sysfs
           SetDeviceFreq(DeviceProcessingUnit::CPU_LITTLE, DeviceFreq::MAX);
           SetDeviceFreq(DeviceProcessingUnit::CPU_MID, DeviceFreq::MAX);
           // gpu sysfs
           WriteSysfs(GPU_MM_MIN_CLOCK, GpuMaxFreq());
           WriteSysfs(GPU_POWEROFF_DELAY, delay_val);
        } else if(SoC::PAPAYA == device) {
           // cpu sysfs
           SetDeviceFreq(DeviceProcessingUnit::CPU_LITTLE, DeviceFreq::MAX);
           // gpu sysfs
           WriteSysfs(GPU_MM_MIN_CLOCK, GpuMaxFreq());
           WriteSysfs(GPU_POWEROFF_DELAY, delay_val);
        } else if(SoC::PAMIR == device) {
           // cpu sysfs
           SetDeviceFreq(DeviceProcessingUnit::AMD_CPU_LITTLE, DeviceFreq::MAX);
           SetDeviceFreq(DeviceProcessingUnit::AMD_CPU_MID, DeviceFreq::MAX);
           // gpu sysfs
           WriteSysfs(GPU_MM_MIN_CLOCK, GpuMaxFreq() + " " + delay_val);
        }
        usleep(GPU_BOOST_SLEEP);
        timeout -= GPU_BOOST_SLEEP;
        std::unique_lock<std::mutex> lk(gpu_boost_mtx_);
        if (timeout <= 0) {
            boost_off_func();
            timeout = TIMEOUT_GPU_BOOST;
            flag_thread_run = false;
            break;
        }
        lk.unlock();
        // LOGD(EDEN_HIDL, "timeout: %d\n", timeout);
    }
    return;
}

Return<uint32_t> ENN_AUX::_apply_GPU_boost(int32_t pid) {
    LOGD(EDEN_HIDL, "started %s func\n", __func__);

    if(SoC::UNKNOWN == device) {
        LOGW(EDEN_HIDL, "Unsupported device. %s (-)\n", __func__);
        return 0;
    }

    std::thread thread_gpuBoost;
    std::unique_lock<std::mutex> lk(gpu_boost_mtx_);
    if (flag_thread_run == false) {
        flag_thread_run = true;
        thread_gpuBoost = std::thread(boost_on_func);
        thread_gpuBoost.detach();
    } else {
        timeout = TIMEOUT_GPU_BOOST;  // Reset
    }
    lk.unlock();

    LOGD(EDEN_HIDL, " %s (-)\n", __func__);
    return 0;
}

ENN_AUX::ENN_AUX() {
    LOGD(EDEN_HIDL, "%s: started", __func__);
    int len=0;
    timeout = TIMEOUT_GPU_BOOST;
    memset(string,0,100);
    flag_thread_run = false;

    int fd = open("/sys/devices/soc0/soc_id", O_RDONLY);
    if(-1 == fd)
    {
        LOGE(EDEN_HIDL, "[SYSFS] Failed to open /sys/devices/soc0/soc_id.\n");
        device = SoC::UNKNOWN;
        return;
    }
    int ret = read(fd, string, sizeof(string));
    close(fd);
    if(-1 == ret)
    {
        LOGE(EDEN_HIDL, "[SYSFS] Reading soc id failed.\n");
        device = SoC::UNKNOWN;
        return;
    }
    string[ret-1] = '\0';
    len=strlen(string);
    LOGD(EDEN_HIDL, "SoC String : %s %d\n", string,len);
    if(0 == strncmp(string,"S5E8825",len-1)) {
        device = SoC::PAPAYA;
        LOGD(EDEN_HIDL, "8825 device detected : %s\n",string);
    }
    else if(0 == strncmp(string,"S5E9925",len-1)) {
        device = SoC::PAMIR;
        LOGD(EDEN_HIDL, "9925 device detected : %s\n",string);
    }
    else if(0 == strncmp(string,"EXYNOS2100",len-1)) {
        device = SoC::OLYMPUS;
        LOGD(EDEN_HIDL, "2100 device detected : %s\n",string);
    }
    else {
        device = SoC::UNKNOWN;
        LOGE(EDEN_HIDL, "Device Not supported : %s\n",string);
    }
}

ENN_AUX::~ENN_AUX() {
    LOGD(EDEN_HIDL, "%s: started", __func__);
    timeout = 0;
    flag_thread_run = false;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace enn_aux
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor
