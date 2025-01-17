///
/// @file mali_gpu_format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include "mali_gpu_format_manager.h"

///
/// @brief Get raw alignment rule for linear layout
///
/// @param[in] format Format
///
/// @return ip alignment
///
align_info MaliGpuFormatManager::get_linear_alignment(PixelFormat format) const
{
        GRALLOC_UNUSED(format);

        align_info rgb_align = {
                .stride_in_bytes        = 16,
                .vstride_in_pixels      = 1,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 0
        };

        return rgb_align;
}

