/*
 * This file is auto-generated. Modifications will be lost.
 *
 * See https://android.googlesource.com/platform/bionic/+/master/libc/kernel/
 * for more information.
 */
#ifndef EXYNOS_DRM_MODIFIER_H
#define EXYNOS_DRM_MODIFIER_H
#include <drm/drm_fourcc.h>
#define DRM_FORMAT_MOD_PROTECTION fourcc_mod_code(NONE, (1ULL << 51))
#define DRM_FORMAT_MOD_SAMSUNG_COLORMAP fourcc_mod_code(SAMSUNG, 4)
#define VOTF_IDENTIFIER (1 << 9)
#define VOTF_BUF_IDX_MASK (0xfULL << 10)
#define VOTF_BUF_IDX_SET(buf_idx) ((buf_idx << 10) & VOTF_BUF_IDX_MASK)
#define VOTF_BUF_IDX_GET(modifier) (((modifier) & VOTF_BUF_IDX_MASK) >> 10)
#define DRM_FORMAT_MOD_SAMSUNG_VOTF(buf_idx) fourcc_mod_code(SAMSUNG, (VOTF_BUF_IDX_SET(buf_idx) | VOTF_IDENTIFIER))
#define SBWC_IDENTIFIER (1 << 4)
#define SBWC_MOD_NONE (0ULL)
#define SBWC_MOD_LOSSLESS (1ULL)
#define SBWC_MOD_LOSSY (2ULL)
#define SBWC_MOD_TYPE_MASK (0x3ULL << 5)
#define SBWC_MOD_TYPE_SET(t) (((t) << 5) & SBWC_MOD_TYPE_MASK)
#define SBWC_MOD_TYPE_GET(mod) (((mod) & SBWC_MOD_TYPE_MASK) >> 5)
#define SBWCL_8B_50 (1ULL)
#define SBWCL_8B_75 (2ULL)
#define SBWCL_10B_40 (1ULL)
#define SBWCL_10B_60 (2ULL)
#define SBWCL_10B_80 (3ULL)
#define SBWCL_MOD_RATIO_MASK (0x3ULL << 7)
#define SBWCL_MOD_RATIO_SET(r) (((r) << 7) & SBWCL_MOD_RATIO_MASK)
#define SBWCL_MOD_RATIO_GET(mod) (((mod) & SBWCL_MOD_RATIO_MASK) >> 7)
#define DRM_FORMAT_MOD_SAMSUNG_SBWC(type,ratio) fourcc_mod_code(SAMSUNG, SBWC_IDENTIFIER | SBWC_MOD_TYPE_SET((type)) | SBWCL_MOD_RATIO_SET((ratio)))
#define SAJC_IDENTIFIER (1 << 14)
#define SAJC_FORMAT_MOD_BLK_SIZE_MASK (0x3ULL << 15)
#define SAJC_BLK_SIZE_SET(blk_size) ((blk_size << 15) & SAJC_FORMAT_MOD_BLK_SIZE_MASK)
#define SAJC_BLK_SIZE_GET(modifier) (((modifier) & SAJC_FORMAT_MOD_BLK_SIZE_MASK) >> 15)
#define DRM_FORMAT_MOD_SAMSUNG_SAJC(blk_size) fourcc_mod_code(SAMSUNG, (SAJC_BLK_SIZE_SET(blk_size) | SAJC_IDENTIFIER))
#define AFBC_FORMAT_MOD_SOURCE_MASK (0xfULL << 52)
#define AFBC_FORMAT_MOD_SOURCE_GPU (1ULL << 52)
#define AFBC_FORMAT_MOD_SOURCE_G2D (2ULL << 52)
#endif
