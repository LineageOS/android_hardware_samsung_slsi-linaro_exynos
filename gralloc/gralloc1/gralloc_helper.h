/*
 * Copyright (C) 2010 ARM Limited. All rights reserved.
 *
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GRALLOC_HELPER_H_
#define GRALLOC_HELPER_H_

#include <sys/mman.h>
#include <hardware/hardware.h>
#include <log/log.h>
#include <string.h>
#include <unistd.h>
#include <hardware/exynos/ion.h>
#include <errno.h>
#include <inttypes.h>
#include <hardware/gralloc1.h>
#include <sync/sync.h>
#include <stdlib.h>
#include <cutils/properties.h>

#include "VendorVideoAPI.h"
#include "mali_gralloc_formats.h"
#include "gralloc1_priv.h"
#include "exynos_format.h"

// Gralloc LOG
#ifndef AWAR
#define AWAR(fmt, args...) __android_log_print(ANDROID_LOG_WARN, "[Gralloc-Warning]", "%s:%d " fmt,__func__,__LINE__,##args)
#endif
#ifndef AINF
#define AINF(fmt, args...) __android_log_print(ANDROID_LOG_INFO, "[Gralloc]", fmt,##args)
#endif
#ifndef AERR
#define AERR(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, "[Gralloc-ERROR]", "%s:%d " fmt,__func__,__LINE__,##args)
#endif
#ifndef AERR_IF
#define AERR_IF( eq, fmt, args...) if ( (eq) ) AERR( fmt, args )
#endif

// GRALLOC UTILITY
#define MALI_GRALLOC_HARDWARE_MAX_STR_LEN                     8
#define GRALLOC_ALIGN(value, base)                            (((value) + ((base) - 1)) & ~((base) - 1))
#define GRALLOC_ALIGN_NON_BASE2(value, align)                 gralloc_align_non_base2((value), (align));
#define GRALLOC_UNUSED(x)                                     ((void) x)
#define INT_TO_PTR(var)                                       ((void *)(unsigned long)var)
#define GRALLOC1_SLSI_USAGE_CHECK(usage, _usage)              (usage & _usage)
#define GRALLOC1_FLAGS_CHECK(flags, _flags)                   (flags & _flags)

// Usage mask
#define GRALLOC1_USAGE_SW_WRITE_MASK                          0x000000F0
#define GRALLOC1_USAGE_SW_READ_MASK                           0x0000000F
#define GRALLOC1_USAGE_READ_OFTEN                            ((1ULL << 1) | (1ULL << 2))
#define GRALLOC1_USAGE_WRITE_OFTEN                           ((1ULL << 5) | (1ULL << 6))
// AFBC
#define GRALLOC_ARM_INTFMT_AFBC                              (1ULL << 32)
#define AFBC_PIXELS_PER_BLOCK                                 16
#define AFBC_BODY_BUFFER_BYTE_ALIGNMENT                       1024
#define AFBC_HEADER_BUFFER_BYTES_PER_BLOCKENTRY               16
// MSCL
#ifdef GRALLOC_MSCL_ALIGN_RESTRICTION
#define MSCL_EXT_SIZE                                         512
#else
#define MSCL_EXT_SIZE                                         0
#endif
#define MSCL_ALIGN                                            128
#ifdef GRALLOC_MSCL_ALLOC_RESTRICTION
#define SIZE_4K                                               4096
#else
#define SIZE_4K                                               0
#endif
// PRIV
#define PRIV_SIZE                                             sizeof(ExynosVideoMeta)

// Plane size
#define PLANE_SIZE(w, h, ext_size)                            ((w) * (h) + ext_size)

// 10bit format
#define P010_PLANE_SIZE(w, h, ext_size)                       ((((w) * 2) * (h)) + ext_size)

// 8+2 format
#define NV12N_S8B_LUMA_SIZE(w, h, ext_size)                   PLANE_SIZE(w, h, ext_size)
#define NV12N_S8B_CHROMA_SIZE(w, h, ext_size)                 (GRALLOC_ALIGN(((w) * (h)) + ext_size, 16))
#define NV12N_S2B_SIZE(w, h)                                  ((GRALLOC_ALIGN((w) / 4, 16) * (h)) + 64)
#define NV12M_S8B_SIZE(w, h, ext_size)                        PLANE_SIZE(w, h, ext_size)
#ifdef GRALLOC_10B_ALIGN_RESTRICTION
#define NV12M_S2B_LUMA_SIZE(w, h, ext_size)                   ((GRALLOC_ALIGN((w) / 4, 16) * (GRALLOC_ALIGN(h, 16))) + ext_size)
#define NV12M_S2B_CHROMA_SIZE(w, h, ext_size)                 ((GRALLOC_ALIGN((w) / 4, 16) * (GRALLOC_ALIGN(h, 16) / 2)) + ext_size)
#else
#define NV12M_S2B_LUMA_SIZE(w, h, ext_size)                   ((GRALLOC_ALIGN((w) / 4, 16) * (h)) + ext_size)
#define NV12M_S2B_CHROMA_SIZE(w, h, ext_size)                 ((GRALLOC_ALIGN((w) / 4, 16) * (h / 2)) + ext_size)
#endif

/* helper macros */
#ifndef __ALIGN_UP
#define __ALIGN_UP(x, a)		(((x) + ((a) - 1)) & ~((a) - 1))
#endif

/* SBWC : Stride is in bits */
#define SBWC_8B_STRIDE(w)		(128 * (((w) + 31) / 32))
#define SBWC_10B_STRIDE(w)		(160 * (((w) + 31) / 32))
#define SBWC_HEADER_STRIDE(w)		((((((w) + 63) / 64) + 15) / 16) * 16)

#define SBWC_Y_VSTRIDE_BLOCKS(h)		((__ALIGN_UP((h), 8) + 3) / 4)
#define SBWC_CBCR_VSTRIDE_BLOCKS(h)		(((__ALIGN_UP((h), 8) / 2) + 3) / 4)

#define SBWC_8B_Y_SIZE(w, h)		((SBWC_8B_STRIDE(w) * ((__ALIGN_UP((h), 8) + 3) / 4)) + 64)
#define SBWC_8B_Y_HEADER_SIZE(w, h)	__ALIGN_UP(((SBWC_HEADER_STRIDE(w) * ((__ALIGN_UP((h), 8) + 3) / 4)) + 256), 32)
#define SBWC_8B_CBCR_SIZE(w, h)		((SBWC_8B_STRIDE(w) * (((__ALIGN_UP((h), 8) / 2) + 3) / 4)) + 64)
#define SBWC_8B_CBCR_HEADER_SIZE(w, h)	((SBWC_HEADER_STRIDE(w) * (((__ALIGN_UP((h), 8) / 2) + 3) / 4)) + 128)

#define SBWC_10B_Y_SIZE(w, h)		((SBWC_10B_STRIDE(w) * ((__ALIGN_UP((h), 8) + 3) / 4)) + 64)
#define SBWC_10B_Y_HEADER_SIZE(w, h)	__ALIGN_UP(((SBWC_HEADER_STRIDE(w) * ((__ALIGN_UP((h), 8) + 3) / 4)) + 256), 32)
#define SBWC_10B_CBCR_SIZE(w, h)	((SBWC_10B_STRIDE(w) * (((__ALIGN_UP((h), 8) / 2) + 3) / 4)) + 64)
#define SBWC_10B_CBCR_HEADER_SIZE(w, h)	((SBWC_HEADER_STRIDE(w) * (((__ALIGN_UP((h), 8) / 2) + 3) / 4)) + 128)

typedef struct private_module_t mali_gralloc_module;

struct private_module_t;
struct private_handle_t;

int grallocUnmap(private_handle_t *hnd);
int gralloc_register_buffer(mali_gralloc_module const* module, buffer_handle_t handle);
int gralloc_unregister_buffer(mali_gralloc_module const* module, buffer_handle_t handle);
int fb_device_open(hw_module_t const* module, const char* name, hw_device_t** device);

static inline int getIonFd(mali_gralloc_module const *module)
{
	private_module_t* m = const_cast<private_module_t*>(reinterpret_cast<const private_module_t*>(module));
	if (m->ionfd == -1)
		m->ionfd = exynos_ion_open();
	return m->ionfd;
}

static inline int gralloc_align_non_base2(int value, int align)
{
	int r = value % align;

	if (r > 0)
		return value + align - r;

	return value;
}

static inline int get_byte_align_64(int bpp)
{
	const static int lcm64[] = {0, 64, 64, 192, 64, 320, 192, 448, 64};

	if (bpp >= 1 && bpp <= 8)
		return lcm64[bpp];

	return 0;
}

static inline int get_bytes_per_pixel(int format)
{
	int bpp = 0;
	switch (format)
	{
		case HAL_PIXEL_FORMAT_RGBA_FP16:
			bpp = 8;
			break;
		case HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888:
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_RGBX_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
		case HAL_PIXEL_FORMAT_RGBA_1010102:
			bpp = 4;
			break;
		case HAL_PIXEL_FORMAT_RGB_888:
			bpp = 3;
			break;
		case HAL_PIXEL_FORMAT_RGB_565:
		case HAL_PIXEL_FORMAT_RAW16:
		case HAL_PIXEL_FORMAT_RAW_OPAQUE:
			bpp = 2;
			break;
		case HAL_PIXEL_FORMAT_BLOB:
			bpp = 1;
			break;
	}
	return bpp;
}

static inline int is_sbwc_format(int format)
{
	switch (format)
	{
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC:
		case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC:
			return 1;
	}

	return 0;
}

#endif /* GRALLOC_HELPER_H_ */
