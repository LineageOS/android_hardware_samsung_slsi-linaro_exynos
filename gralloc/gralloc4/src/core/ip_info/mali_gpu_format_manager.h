///
/// @file mali_gpu_format_manager.h
/// @copyright 2020-2021 Samsung Electronics
///
#pragma once

#include "ip_format_manager.h"

class MaliGpuFormatManager : public IpFormatManager {
public:
        MaliGpuFormatManager() = default;
        ~MaliGpuFormatManager() = default;
        MaliGpuFormatManager(const IpFormatManager&) = delete;
        MaliGpuFormatManager(const IpFormatManager&&) = delete;
        MaliGpuFormatManager& operator=(const IpFormatManager&) = delete;
        MaliGpuFormatManager& operator=(const IpFormatManager&&) = delete;

        align_info    get_linear_alignment(PixelFormat format) const override;
};

