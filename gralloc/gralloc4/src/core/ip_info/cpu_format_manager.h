///
/// @file cpu_format_manager.h
/// @copyright 2020-2021 Samsung Electronics
///
#pragma once

#include "ip_format_manager.h"

class CpuFormatManager : public IpFormatManager {
public:
        CpuFormatManager() = default;
        ~CpuFormatManager() = default;
        CpuFormatManager(const CpuFormatManager&) = delete;
        CpuFormatManager(const CpuFormatManager&&) = delete;
        CpuFormatManager& operator=(const CpuFormatManager&) = delete;
        CpuFormatManager& operator=(const CpuFormatManager&&) = delete;

        align_info    get_linear_alignment(PixelFormat format) const override;
};

