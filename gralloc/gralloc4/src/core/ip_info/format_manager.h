///
/// @file format_manager.h
/// @copyright 2020-2021 Samsung Electronics
///

#pragma once

#include <cstdint>

#include "hidl_defines.h"
#include "ip_format_manager.h"

#include "mali_gralloc_log.h"
#include "gralloc_helper.h"

class FormatManager {
public:
        FormatManager();
        ~FormatManager();
        FormatManager(const FormatManager&) = delete;
        FormatManager(const FormatManager&&) = delete;
        FormatManager& operator=(const FormatManager&) = delete;
        FormatManager& operator=(const FormatManager&&) = delete;

        align_info get_alloc_align(PixelFormat format, uint64_t usage);

private:
        IpFormatManager *m_ip_format_manager[static_cast<uint32_t>(Ip::COUNT)];
};

