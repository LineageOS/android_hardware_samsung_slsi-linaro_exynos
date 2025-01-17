///
/// @file camera_format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include "camera_format_manager.h"

///
/// @brief Get raw alignment rule for linear layout
///
/// @param[in] format Format
///
/// @return ip alignment
///
align_info CameraFormatManager::get_linear_alignment(PixelFormat format) const
{
        GRALLOC_UNUSED(format);

        /// Camera IP requires 16B row alignment
        static const align_info align = {
                .stride_in_bytes        = 16,
                .vstride_in_pixels      = 1,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 0
        };

        return align;
}

