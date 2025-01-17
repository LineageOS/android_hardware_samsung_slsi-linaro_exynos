///
/// @file hwc_format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include "hwc_format_manager.h"

///
/// @brief Get raw alignment rule for linear layout
///
/// @param[in] format Format
///
/// @return ip alignment
///
align_info HwcFormatManager::get_linear_alignment(PixelFormat format) const
{
        GRALLOC_UNUSED(format);

        align_info align = {
                .stride_in_bytes        = 1,
                .vstride_in_pixels      = 1,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 0
        };

        return align;
}

