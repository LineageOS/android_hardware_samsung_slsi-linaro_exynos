///
/// @file cpu_format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include "cpu_format_manager.h"

///
/// @brief Get raw alignment rule for linear layout
///
/// @param[in] format Format
///
/// @return ip alignment
///
align_info CpuFormatManager::get_linear_alignment(PixelFormat format) const
{
        align_info align = {
                .stride_in_bytes        = 1,
                .vstride_in_pixels      = 1,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 0
        };

        // Android format requirement - a horizontal stride multiple of 16 pixels
        switch (format) {
        case PixelFormat::Y8:
        case PixelFormat::YV12:
                align.stride_in_bytes = 16;
                break;
        case PixelFormat::RAW16:
        case PixelFormat::Y16:
                align.stride_in_bytes = 32;
                break;
        default:
                break;
        }

        return align;
}

