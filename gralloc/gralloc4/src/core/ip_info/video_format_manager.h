///
/// @file video_format_manager.h
/// @copyright 2020-2021 Samsung Electronics
///
#pragma once

#include "ip_format_manager.h"

class VideoFormatManager : public IpFormatManager {
public:
        VideoFormatManager() = default;
        ~VideoFormatManager() = default;
        VideoFormatManager(const IpFormatManager&) = delete;
        VideoFormatManager(const IpFormatManager&&) = delete;
        VideoFormatManager& operator=(const IpFormatManager&) = delete;
        VideoFormatManager& operator=(const IpFormatManager&&) = delete;

        align_info    get_linear_alignment(PixelFormat format) const override;
};

