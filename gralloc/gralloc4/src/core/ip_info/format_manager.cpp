///
/// @file format_manager.cpp
/// @copyright 2020-2021 Samsung Electronics
///

#include <inttypes.h>

#include "format_manager.h"
#include "camera_format_manager.h"
#include "cpu_format_manager.h"
#include "hwc_format_manager.h"
#include "mali_gpu_format_manager.h"
#include "video_format_manager.h"

///
/// @brief FormatManager constructor
///
FormatManager::FormatManager()
{
        static CameraFormatManager camera_format_manager;
        static CpuFormatManager    cpu_format_manager;
        static HwcFormatManager    hwc_format_manager;
        static VideoFormatManager  video_format_manager;
        static MaliGpuFormatManager gpu_format_manager;

        m_ip_format_manager[static_cast<uint32_t>(Ip::CPU)]    = &cpu_format_manager;
        m_ip_format_manager[static_cast<uint32_t>(Ip::GPU)]    = &gpu_format_manager;
        m_ip_format_manager[static_cast<uint32_t>(Ip::HWC)]    = &hwc_format_manager;
        m_ip_format_manager[static_cast<uint32_t>(Ip::CAMERA)] = &camera_format_manager;
        m_ip_format_manager[static_cast<uint32_t>(Ip::VIDEO)]  = &video_format_manager;
}

static inline uint32_t get_ip_flags(uint64_t usage)
{
        // There are more usages but only consider format/IP-related usages
        constexpr uint64_t cpu_mask = (static_cast<uint64_t>(BufferUsage::CPU_READ_MASK) |
                        static_cast<uint64_t>(BufferUsage::CPU_WRITE_MASK));
        constexpr uint64_t gpu_mask = (static_cast<uint64_t>(BufferUsage::GPU_TEXTURE) |
                        static_cast<uint64_t>(BufferUsage::GPU_RENDER_TARGET) |
                        static_cast<uint64_t>(BufferUsage::GPU_DATA_BUFFER));
        constexpr uint64_t hwc_mask = (static_cast<uint64_t>(BufferUsage::COMPOSER_OVERLAY) |
                        static_cast<uint64_t>(BufferUsage::COMPOSER_CLIENT_TARGET) |
                        static_cast<uint64_t>(BufferUsage::COMPOSER_CURSOR));
        constexpr uint64_t video_mask = (static_cast<uint64_t>(BufferUsage::VIDEO_ENCODER) |
                        static_cast<uint64_t>(BufferUsage::VIDEO_DECODER));
        constexpr uint64_t camera_mask = (static_cast<uint64_t>(BufferUsage::CAMERA_OUTPUT) |
                        static_cast<uint64_t>(BufferUsage::CAMERA_INPUT));

        uint32_t ip_flags = 0;
        if (is_any_bitmask_set_64(usage, cpu_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::CPU));
        }

        if (is_any_bitmask_set_64(usage, gpu_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::GPU));
        }

        if (is_any_bitmask_set_64(usage, hwc_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::HWC));
        }

        if (is_any_bitmask_set_64(usage, camera_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::CAMERA));
        }

        if (is_any_bitmask_set_64(usage, video_mask)) {
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::VIDEO));
        }

        if (usage == 0) {
                ALOGE("Buffer Usage is 0, Defaulting to CPU");
                set_bitmask(&ip_flags, static_cast<uint32_t>(IpBitMask::CPU));
        }

        return ip_flags;
}

static inline void get_common_alignment(const align_info &align, align_info *common_align)
{
        common_align->stride_in_bytes = GRALLOC_MAX(align.stride_in_bytes, common_align->stride_in_bytes);

        common_align->vstride_in_pixels = GRALLOC_MAX(align.vstride_in_pixels, common_align->vstride_in_pixels);

        common_align->plane_padding_in_bytes =
                GRALLOC_MAX(align.plane_padding_in_bytes, common_align->plane_padding_in_bytes);

        common_align->alloc_padding_in_bytes =
                GRALLOC_MAX(align.alloc_padding_in_bytes, common_align->alloc_padding_in_bytes);
}

align_info FormatManager::get_alloc_align(PixelFormat format, uint64_t usage)
{
        uint32_t ip_flags = get_ip_flags(usage);

        align_info alloc_align = {
                .stride_in_bytes = 1,
                .vstride_in_pixels = 1,
                .plane_padding_in_bytes = 0,
                .alloc_padding_in_bytes = 0
        };

        for (uint32_t pos = 0; pos < static_cast<uint32_t>(Ip::COUNT) && ip_flags != 0; pos++) {
                if ((ip_flags & 0x1) == 0x1) {
                        align_info ip_align = m_ip_format_manager[pos]->get_linear_alignment(format);
                        get_common_alignment(ip_align, &alloc_align);
                }
                ip_flags >>= 1;
        }

        return alloc_align;
}
///
/// @brief FormatManager destructor
///
FormatManager::~FormatManager()
{
}

