///
/// @file video_format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include "video_format_manager.h"

///
/// @brief Get raw alignment rule for linear layout
///
/// @param[in] format Format
///
/// @return ip alignment
///
align_info VideoFormatManager::get_linear_alignment(PixelFormat format) const
{
        align_info align = {
                .stride_in_bytes        = 16,
                .vstride_in_pixels      = 16,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 256 // padding = max(64B for decoding, 256B for encoding)
        };

        if (format == PixelFormat::YV12) {
                align.vstride_in_pixels = 1;
        } else if (format == PixelFormat::BLOB) {
                align.vstride_in_pixels = 1;
        } else if (format == PixelFormat::YCBCR_P010) {
                align.stride_in_bytes = 128;
        }

        return align;
}

