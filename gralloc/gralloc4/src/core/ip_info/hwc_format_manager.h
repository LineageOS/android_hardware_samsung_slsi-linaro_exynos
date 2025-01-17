///
/// @file hwc_format_manager.h
/// @copyright 2020-2021 Samsung Electronics
///

#include "ip_format_manager.h"

class HwcFormatManager : public IpFormatManager {
public:
        HwcFormatManager() = default;
        ~HwcFormatManager() = default;
        HwcFormatManager(const IpFormatManager&) = delete;
        HwcFormatManager(const IpFormatManager&&) = delete;
        HwcFormatManager& operator=(const IpFormatManager&) = delete;
        HwcFormatManager& operator=(const IpFormatManager&&) = delete;

        align_info    get_linear_alignment(PixelFormat format) const override;
};

