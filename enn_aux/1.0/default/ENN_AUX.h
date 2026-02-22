/*
 * Copyright (C) 2018 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/**
 * @file    ENN_AUX.h
 * @brief   This is EDEN Service header.
 * @details This is the EDEN Service header file.
 */

#ifndef VENDOR_SAMSUNG_SLSI_HARDWARE_ENN_AUX_V1_0_H
#define VENDOR_SAMSUNG_SLSI_HARDWARE_ENN_AUX_V1_0_H

#include <fcntl.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <memory>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <vendor/samsung_slsi/hardware/enn_aux/1.0/IENN_AUX.h>

namespace vendor {
namespace samsung_slsi {
namespace hardware {
namespace enn_aux {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

using ::android::hidl::base::V1_0::IBase;
using ::vendor::samsung_slsi::hardware::enn_aux::V1_0::IENN_AUX;
#define GPU_BOOST_SLEEP 30000  // in microseconds
#define TIMEOUT_GPU_BOOST 5000000  // in microseconds

enum class DeviceProcessingUnit {
    CPU_LITTLE = 0,
    CPU_MID = 1,
    CPU_BIG = 2,
    AMD_CPU_LITTLE = 3,
    AMD_CPU_MID = 4,
    AMD_CPU_BIG = 5,
    GPU = 6
};

enum class DeviceFreq {
    MIN = 0,
    MAX = 1
};

enum class SoC {
    OLYMPUS = 0,
    PAPAYA = 1,
    PAMIR = 2,
    UNKNOWN = 15
};

extern int32_t timeout;
extern bool flag_thread_run;
SoC device;

constexpr const char *CPU_LTL_FREQ_TABLE = "/sys/devices/system/cpu/cpufreq/policy0/scaling_available_frequencies";
constexpr const char *CPU_LTL_FREQ_SCALE_MIN = "/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq";
constexpr const char *CPU_LTL_FREQ_SCALE_MAX = "/sys/devices/system/cpu/cpufreq/policy0/scaling_max_freq";

constexpr const char *CPU_MID_FREQ_TABLE = "/sys/devices/system/cpu/cpufreq/policy4/scaling_available_frequencies";
constexpr const char *CPU_MID_FREQ_SCALE_MIN = "/sys/devices/system/cpu/cpufreq/policy4/scaling_min_freq";
constexpr const char *CPU_MID_FREQ_SCALE_MAX = "/sys/devices/system/cpu/cpufreq/policy4/scaling_max_freq";

constexpr const char *CPU_BIG_FREQ_SCALE_MIN = "/sys/devices/system/cpu/cpufreq/policy7/scaling_min_freq";
constexpr const char *CPU_BIG_FREQ_TABLE = "/sys/devices/system/cpu/cpufreq/policy7/scaling_available_frequencies";
constexpr const char *CPU_BIG_FREQ_SCALE_MAX = "/sys/devices/system/cpu/cpufreq/policy7/scaling_max_freq";

constexpr const char *AMD_CPU_LTL_FREQ_TABLE = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies";
constexpr const char *AMD_CPU_LTL_FREQ_SCALE_MIN = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq";
constexpr const char *AMD_CPU_LTL_FREQ_SCALE_MAX = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq";

constexpr const char *AMD_CPU_MID_FREQ_TABLE = "/sys/devices/system/cpu/cpu4/cpufreq/scaling_available_frequencies";
constexpr const char *AMD_CPU_MID_FREQ_SCALE_MIN = "/sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq";
constexpr const char *AMD_CPU_MID_FREQ_SCALE_MAX = "/sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq";

constexpr const char *AMD_CPU_BIG_FREQ_TABLE = "/sys/devices/system/cpu/cpu7/cpufreq/scaling_available_frequencies";
constexpr const char *AMD_CPU_BIG_FREQ_SCALE_MIN = "/sys/devices/system/cpu/cpu7/cpufreq/scaling_min_freq";
constexpr const char *AMD_CPU_BIG_FREQ_SCALE_MAX = "/sys/devices/system/cpu/cpu7/cpufreq/scaling_max_freq";

constexpr const char *GPU_FREQ_TABLE = "/sys/kernel/gpu/gpu_freq_table";
constexpr const char *GPU_MM_MIN_CLOCK = "/sys/kernel/gpu/gpu_mm_min_clock";
constexpr const char *GPU_POWEROFF_DELAY = "/sys/kernel/gpu/gpu_poweroff_delay";

struct ENN_AUX : public IENN_AUX {
    ENN_AUX();
    virtual ~ENN_AUX();

    Return<uint32_t> _apply_GPU_boost(int32_t pid) override;

private:
    char string[100];
};

// utils
std::string GetDeviceFreq(DeviceProcessingUnit unit, DeviceFreq min_max, int id = 0);
int WriteSysfs(const char *sysfs_node, const std::string &value);
void SetDeviceFreq(DeviceProcessingUnit unit, DeviceFreq min_max);

static std::string &CpuLittleMinFreq() {
	  static std::string freq = GetDeviceFreq(DeviceProcessingUnit::CPU_LITTLE, DeviceFreq::MIN);
	    return freq;
}
static std::string &CpuLittleMaxFreq() {
	  static std::string freq = GetDeviceFreq(DeviceProcessingUnit::CPU_LITTLE, DeviceFreq::MAX);
	    return freq;
}
static std::string &CpuMidMinFreq() {
	  static std::string freq = GetDeviceFreq(DeviceProcessingUnit::CPU_MID, DeviceFreq::MIN);
	    return freq;
}
static std::string &CpuMidMaxFreq() {
	  static std::string freq = GetDeviceFreq(DeviceProcessingUnit::CPU_MID, DeviceFreq::MAX);
	    return freq;
}
static std::string &CpuBigMinFreq() {
	  static std::string freq = GetDeviceFreq(DeviceProcessingUnit::CPU_BIG, DeviceFreq::MIN);
	    return freq;
}
static std::string &CpuBigMaxFreq() {
	  static std::string freq = GetDeviceFreq(DeviceProcessingUnit::CPU_BIG, DeviceFreq::MAX);
	    return freq;
}
static std::string &AmdCpuLittleMinFreq() {
          static std::string freq = GetDeviceFreq(DeviceProcessingUnit::AMD_CPU_LITTLE, DeviceFreq::MIN);
            return freq;
}
static std::string &AmdCpuLittleMaxFreq() {
          static std::string freq = GetDeviceFreq(DeviceProcessingUnit::AMD_CPU_LITTLE, DeviceFreq::MAX);
            return freq;
}
static std::string &AmdCpuMidMinFreq() {
          static std::string freq = GetDeviceFreq(DeviceProcessingUnit::AMD_CPU_MID, DeviceFreq::MIN);
            return freq;
}
static std::string &AmdCpuMidMaxFreq() {
          static std::string freq = GetDeviceFreq(DeviceProcessingUnit::AMD_CPU_MID, DeviceFreq::MAX);
            return freq;
}
static std::string &AmdCpuBigMinFreq() {
          static std::string freq = GetDeviceFreq(DeviceProcessingUnit::AMD_CPU_BIG, DeviceFreq::MIN);
            return freq;
}
static std::string &AmdCpuBigMaxFreq() {
          static std::string freq = GetDeviceFreq(DeviceProcessingUnit::AMD_CPU_BIG, DeviceFreq::MAX);
            return freq;
}
static std::string &GpuMaxFreq() {
	  static std::string freq = GetDeviceFreq(DeviceProcessingUnit::GPU, DeviceFreq::MAX);
	    return freq;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace enn_aux
}  // namespace hardware
}  // namespace samsung_slsi
}  // namespace vendor

#endif  // VENDOR_SAMSUNG_SLSI_HARDWARE_ENN_AUX_V1_0_H
