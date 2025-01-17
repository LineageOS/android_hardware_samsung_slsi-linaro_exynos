///
/// @file hidl_defines.h
/// @copyright 2020 Samsung Electronics
///

#pragma once

#include <stdint.h>

enum class Error : int32_t {
        NONE            = 0,
        BAD_DESCRIPTOR  = 1,
        BAD_BUFFER      = 2,
        BAD_VALUE       = 3,
        NO_RESOURCES    = 5,
        UNSUPPORTED     = 7,
};

enum class PixelFormat : int32_t {
        UNDEFINED = 0,                  // private
        RGBA_8888 = 1,
        RGBX_8888 = 2,
        RGB_888 = 3,
        RGB_565 = 4,
        BGRA_8888 = 5,
        YCBCR_422_SP = 16,
        YCRCB_420_SP = 17,
        YCBCR_422_I = 20,
        RGBA_FP16 = 22,
        RAW16 = 32,
        BLOB = 33,
        IMPLEMENTATION_DEFINED = 34,
        YCBCR_420_888 = 35,
        RAW_OPAQUE = 36,
        RAW10 = 37,
        RAW12 = 38,
        RGBA_1010102 = 43,
        DEPTH_16 = 48,
        DEPTH_24 = 49,
        DEPTH_24_STENCIL_8 = 50,
        DEPTH_32F = 51,
        DEPTH_32F_STENCIL_8 = 52,
        STENCIL_8 = 53,
        YCBCR_P010 = 54,
        HSV_888 = 55 /* 0x37 */,
	R_8 = 56 /* 38 */,
        Y8 = 538982489,
        Y16 = 540422489,
        YV12 = 842094169,

        /// PrivateFormat
        /// Invariant: PrivateFormat should be non-overlapping with to PixelFormat
        /// From exynos definition hardware/samsung_slsi-linaro/exynos/include/exynos_format.h
        PRIVATE_YCBCR_420_P_M               = 0x101,
        PRIVATE_YCBCR_420_SP_M              = 0x105,
        PRIVATE_YCBCR_420_SP_M_TILED        = 0x107,
        PRIVATE_YV12_M                      = 0x11C,   /* YV12 */
        PRIVATE_YCRCB_420_SP_M              = 0x11D,   /* YCrCb_420_SP */
        PRIVATE_YCRCB_420_SP_M_FULL         = 0x11E,   /* YCrCb_420_SP_FULL */
        PRIVATE_YCBCR_420_P                 = 0x11F,
        PRIVATE_YCBCR_420_SP                = 0x120,
        PRIVATE_YCBCR_420_SPN               = 0x123,
        /* 10-bit format (8bit + separated 2bit) w/ private data buffer */
        PRIVATE_YCBCR_420_SP_M_S10B         = 0x125,
        /* 10-bit contiguous(single fd, 8bit + separated 2bit) custom formats */
        PRIVATE_YCBCR_420_SPN_S10B          = 0x126,
        /* 10-bit format (2 fd, 10bit, 2x byte) custom formats */
        PRIVATE_YCBCR_P010_M                = 0x127,

        /* SBWC format */
        PRIVATE_YCBCR_420_SP_M_SBWC         = 0x130,
        PRIVATE_YCBCR_420_SPN_SBWC          = 0x131,
        PRIVATE_YCBCR_420_SP_M_10B_SBWC     = 0x132,
        PRIVATE_YCBCR_420_SPN_10B_SBWC      = 0x133,
        PRIVATE_YCRCB_420_SP_M_SBWC         = 0x134,
        PRIVATE_YCRCB_420_SP_M_10B_SBWC     = 0x135,
        /* SBWC Lossy formats */
        PRIVATE_YCBCR_420_SP_M_SBWC_L50     = 0x140,
        PRIVATE_YCBCR_420_SP_M_SBWC_L75     = 0x141,
        PRIVATE_YCBCR_420_SPN_SBWC_L50      = 0x150,
        PRIVATE_YCBCR_420_SPN_SBWC_L75      = 0x151,
        PRIVATE_YCBCR_420_SP_M_10B_SBWC_L40 = 0x160,
        PRIVATE_YCBCR_420_SP_M_10B_SBWC_L60 = 0x161,
        PRIVATE_YCBCR_420_SP_M_10B_SBWC_L80 = 0x162,
        PRIVATE_YCBCR_420_SPN_10B_SBWC_L40  = 0x170,
        PRIVATE_YCBCR_420_SPN_10B_SBWC_L60  = 0x171,
        PRIVATE_YCBCR_420_SPN_10B_SBWC_L80  = 0x172,
};

enum class BufferUsage : uint64_t {
        CPU_READ_MASK = 15ull /* 0xfULL */,
        CPU_READ_NEVER = 0ull,
        CPU_READ_RARELY = 2ull,
        CPU_READ_OFTEN = 3ull,
        CPU_WRITE_MASK = 240ull /* (0xfULL << 4) */,
        CPU_WRITE_NEVER = 0ull /* (0 << 4) */,
        CPU_WRITE_RARELY = 32ull /* (2 << 4) */,
        CPU_WRITE_OFTEN = 48ull /* (3 << 4) */,
        GPU_TEXTURE = 256ull /* (1ULL << 8) */,
        GPU_RENDER_TARGET = 512ull /* (1ULL << 9) */,
        COMPOSER_OVERLAY = 2048ull /* (1ULL << 11) */,
        COMPOSER_CLIENT_TARGET = 4096ull /* (1ULL << 12) */,
        PROTECTED = 16384ull /* (1ULL << 14) */,
        COMPOSER_CURSOR = 32768ull /* (1ULL << 15) */,
        VIDEO_ENCODER = 65536ull /* (1ULL << 16) */,
        CAMERA_OUTPUT = 131072ull /* (1ULL << 17) */,
        CAMERA_INPUT = 262144ull /* (1ULL << 18) */,
        RENDERSCRIPT = 1048576ull /* (1ULL << 20) */,
        VIDEO_DECODER = 4194304ull /* (1ULL << 22) */,
        SENSOR_DIRECT_DATA = 8388608ull /* (1ULL << 23) */,
        GPU_DATA_BUFFER = 16777216ull /* (1ULL << 24) */,
        GPU_CUBE_MAP = 33554432ull /* (1ULL << 25) */,
        GPU_MIPMAP_COMPLETE = 67108864ull /* (1ULL << 26) */,
        HW_IMAGE_ENCODER = 134217728ull /* (1ULL << 27) */,

        VENDOR_MASK = 4026531840ull /* (0xfULL << 28) */,
        PRIVATE_GDC_MODE = (1ULL << 28),
        PRIVATE_NO_SAJC = (1ULL << 29),
        PRIVATE_CAMERA_RESERVED = (1ULL << 30),
        PRIVATE_SECURE_CAMERA_RESERVED = (1ULL << 31),

        VENDOR_MASK_HI = 18446462598732840960ull /* (0xffffULL << 48) */,
        /* = (1ULL << 48), */
        /* = (1ULL << 49), */
        /* = (1ULL << 50), */
        PRIVATE_DOWNSCALE = (1ULL << 51),
        PRIVATE_ROIINFO = (1ULL << 52),
        /* = (1ULL << 53), */
        PRIVATE_FORCE_BACKBUFFER = (1ULL << 54),
        PRIVATE_FRONTBUFFER = (1ULL << 55),
        PRIVATE_SBWC_REQUEST_10BIT = (1ULL << 56),
        PRIVATE_HFR_MODE = (1ULL << 57),
        PRIVATE_NOZEROED = (1ULL << 58),
        PRIVATE_PRIVATE_NONSECURE = (1ULL << 59),
        PRIVATE_VIDEO_PRIVATE_DATA = (1ULL << 60),
        PRIVATE_VIDEO_EXT = (1ULL << 61),
        PRIVATE_DAYDREAM_SINGLE_BUFFER_MODE = (1ULL << 62),
        PRIVATE_YUV_RANGE_FULL = (1ULL << 63),
};

enum class Dataspace : uint32_t {
        UNKNOWN = 0x0,
        ARBITRARY = 0x1,
        STANDARD_SHIFT = 16,
        STANDARD_MASK = 63 << STANDARD_SHIFT,
        STANDARD_UNSPECIFIED = 0 << STANDARD_SHIFT,
        STANDARD_BT709 = 1 << STANDARD_SHIFT,
        STANDARD_BT601_625 = 2 << STANDARD_SHIFT,
        STANDARD_BT601_625_UNADJUSTED = 3 << STANDARD_SHIFT,
        STANDARD_BT601_525 = 4 << STANDARD_SHIFT,
        STANDARD_BT601_525_UNADJUSTED = 5 << STANDARD_SHIFT,
        STANDARD_BT2020 = 6 << STANDARD_SHIFT,
        STANDARD_BT2020_CONSTANT_LUMINANCE = 7 << STANDARD_SHIFT,
        STANDARD_BT470M = 8 << STANDARD_SHIFT,
        STANDARD_FILM = 9 << STANDARD_SHIFT,
        STANDARD_DCI_P3 = 10 << STANDARD_SHIFT,
        STANDARD_ADOBE_RGB = 11 << STANDARD_SHIFT,
        TRANSFER_SHIFT = 22,
        TRANSFER_MASK = 31 << TRANSFER_SHIFT,
        TRANSFER_UNSPECIFIED = 0 << TRANSFER_SHIFT,
        TRANSFER_LINEAR = 1 << TRANSFER_SHIFT,
        TRANSFER_SRGB = 2 << TRANSFER_SHIFT,
        TRANSFER_SMPTE_170M = 3 << TRANSFER_SHIFT,
        TRANSFER_GAMMA2_2 = 4 << TRANSFER_SHIFT,
        TRANSFER_GAMMA2_6 = 5 << TRANSFER_SHIFT,
        TRANSFER_GAMMA2_8 = 6 << TRANSFER_SHIFT,
        TRANSFER_ST2084 = 7 << TRANSFER_SHIFT,
        TRANSFER_HLG = 8 << TRANSFER_SHIFT,
        RANGE_SHIFT = 27,
        RANGE_MASK = 7 << RANGE_SHIFT,
        RANGE_UNSPECIFIED =0 << RANGE_SHIFT,
        RANGE_FULL = 1 << RANGE_SHIFT,
        RANGE_LIMITED = 2 << RANGE_SHIFT,
        RANGE_EXTENDED = 3 << RANGE_SHIFT,
        SRGB_LINEAR = STANDARD_BT709 | TRANSFER_LINEAR | RANGE_FULL,
        SCRGB_LINEAR = STANDARD_BT709 | TRANSFER_LINEAR | RANGE_EXTENDED,
        SRGB = STANDARD_BT709 | TRANSFER_SRGB | RANGE_FULL,
        SCRGB = STANDARD_BT709 | TRANSFER_SRGB | RANGE_EXTENDED,
        JFIF = STANDARD_BT601_625 | TRANSFER_SMPTE_170M | RANGE_FULL,
        BT601_625 = STANDARD_BT601_625 | TRANSFER_SMPTE_170M | RANGE_LIMITED,
        BT601_525 = STANDARD_BT601_525 | TRANSFER_SMPTE_170M | RANGE_LIMITED,
        BT709 = STANDARD_BT709 | TRANSFER_SMPTE_170M | RANGE_LIMITED,
        DCI_P3_LINEAR = STANDARD_DCI_P3 | TRANSFER_LINEAR | RANGE_FULL,
        DCI_P3 = STANDARD_DCI_P3 | TRANSFER_GAMMA2_6 | RANGE_FULL,
        DISPLAY_P3_LINEAR = STANDARD_DCI_P3 | TRANSFER_LINEAR | RANGE_FULL,
        DISPLAY_P3 = STANDARD_DCI_P3 | TRANSFER_SRGB | RANGE_FULL,
        ADOBE_RGB = STANDARD_ADOBE_RGB | TRANSFER_GAMMA2_2 | RANGE_FULL,
        BT2020_LINEAR = STANDARD_BT2020 | TRANSFER_LINEAR | RANGE_FULL,
        BT2020 = STANDARD_BT2020 | TRANSFER_SMPTE_170M | RANGE_FULL,
        BT2020_PQ = STANDARD_BT2020 | TRANSFER_ST2084 | RANGE_FULL,
        DEPTH = 0x1000,
        SENSOR = 0x1001,
        BT2020_ITU = STANDARD_BT2020 | TRANSFER_SMPTE_170M | RANGE_LIMITED,
        BT2020_ITU_PQ = STANDARD_BT2020 | TRANSFER_ST2084 | RANGE_LIMITED,
        BT2020_ITU_HLG = STANDARD_BT2020 | TRANSFER_HLG | RANGE_LIMITED,
        BT2020_HLG = STANDARD_BT2020 | TRANSFER_HLG | RANGE_FULL,
        DISPLAY_BT2020 = STANDARD_BT2020 | TRANSFER_SRGB | RANGE_FULL,
        DYNAMIC_DEPTH = 0x1002,
        JPEG_APP_SEGMENTS = 0x1003,
        HEIF = 0x1004,
        BT709_FULL_RANGE = STANDARD_BT709 | TRANSFER_SMPTE_170M | RANGE_FULL,
};
