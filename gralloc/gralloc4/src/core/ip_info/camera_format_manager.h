///
/// @file camera_format_manager.h
/// @copyright 2020-2021 Samsung Electronics
///
#pragma once

#include "ip_format_manager.h"

class CameraFormatManager : public IpFormatManager {
public:
        CameraFormatManager() = default;
        ~CameraFormatManager() = default;
        CameraFormatManager(const IpFormatManager&) = delete;
        CameraFormatManager(const IpFormatManager&&) = delete;
        CameraFormatManager& operator=(const IpFormatManager&) = delete;
        CameraFormatManager& operator=(const IpFormatManager&&) = delete;

        align_info    get_linear_alignment(PixelFormat format) const override;
};

