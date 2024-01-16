/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <limits.h>
#include <sys/cdefs.h>
#include <hardware/gralloc.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <cutils/native_handle.h>
#include <linux/fb.h>
#include <linux/ion.h>

/* SLSI specific usages */
#define GRALLOC_USAGE_PROTECTED_DPB                 0x00800000U
#define GRALLOC_USAGE_PHYSICALLY_LINEAR             0x01000000U
#define GRALLOC_USAGE_PRIVATE_NONSECURE             0x02000000U
#define GRALLOC_USAGE_CAMERA_RESERVED               0x04000000U
#define GRALLOC_USAGE_NOZEROED                      0x08000000U
#define GRALLOC_USAGE_VIDEO_EXT                     GRALLOC_USAGE_PRIVATE_0
#define GRALLOC_USAGE_YUV_RANGE_FULL                GRALLOC_USAGE_PRIVATE_1
#define GRALLOC_USAGE_DAYDREAM_SINGLE_BUFFER_MODE   GRALLOC_USAGE_PRIVATE_2
#define GRALLOC_USAGE_SECURE_CAMERA_RESERVED        GRALLOC_USAGE_PRIVATE_3

#define AFBC_INFO_SIZE                              (sizeof(int))
#define AFBC_ENABLE                                 (0xafbc)

typedef enum{
	MALI_YUV_NO_INFO,
	MALI_YUV_BT601_NARROW,
	MALI_YUV_BT601_WIDE,
	MALI_YUV_BT709_NARROW,
	MALI_YUV_BT709_WIDE,
	MALI_YUV_BT2020_NARROW,
	MALI_YUV_BT2020_WIDE
} mali_gralloc_yuv_info;

/*****************************************************************************/

struct private_module_t;
struct private_handle_t;

struct private_module_t {
	gralloc_module_t base;

	private_handle_t* framebuffer;
	uint32_t flags;
	uint32_t numBuffers;
	uint32_t bufferMask;
	pthread_mutex_t lock;
	buffer_handle_t currentBuffer;
	int ionfd;

	struct fb_var_screeninfo info;
	struct fb_fix_screeninfo finfo;
	int xres;
	int yres;
	int line_length;
	float xdpi;
	float ydpi;
	float fps;
	void *queue;
	pthread_mutex_t queue_lock;

};

/*****************************************************************************/

#ifdef __cplusplus
struct private_handle_t : public native_handle {
#else
struct private_handle_t {
	struct native_handle nativeHandle;
#endif
	enum {
		PRIV_FLAGS_FRAMEBUFFER = 0x00000001,
		PRIV_FLAGS_USES_UMP    = 0x00000002,
		PRIV_FLAGS_USES_ION    = 0x00000020
	};

	// file-descriptors
	int         fd;
	int         fd1;
	int         fd2;
	// ints
	int         magic;
	int         flags;
	int         size;
	int         size1;
	int         size2;
	int         offset;
	int         format;
	uint64_t    internal_format;
	int         frameworkFormat;
	int         width;
	int         height;
	int         stride;
	int         vstride;
	int         is_compressible;
	int         compressed_out;
	int         prefer_compression;

	int         lock_usage;
	int         lock_offset;
	int         lock_len;

	mali_gralloc_yuv_info     yuv_info;
	int     PRIVATE_1;
	int     PRIVATE_2;
	int     PRIVATE_3;
	int     PRIVATE_4;

	ion_user_handle_t handle;
	ion_user_handle_t handle1;
	ion_user_handle_t handle2;

	uint64_t base __attribute__((aligned(8)));
	uint64_t base1 __attribute__((aligned(8)));
	uint64_t base2 __attribute__((aligned(8)));

#ifdef __cplusplus
	static inline int sNumInts() {
		return (((sizeof(private_handle_t) - sizeof(native_handle_t))/sizeof(int)) - sNumFds);
	}
	static const int sNumFds = 3;
	static const int sMagic = 0x3141592;

	private_handle_t(int _fd, int _size, int _flags) :
		fd(_fd), fd1(-1), fd2(-1), magic(sMagic), flags(_flags), size(_size), size1(0), size2(0),
		offset(0), format(0), internal_format(0), frameworkFormat(0), width(0), height(0), stride(0),
		vstride(0), is_compressible(0), compressed_out(0), prefer_compression(0),
		lock_usage(0), lock_offset(0), lock_len(0),	yuv_info(MALI_YUV_NO_INFO), PRIVATE_1(0), PRIVATE_2(0), PRIVATE_3(0), PRIVATE_4(0),
		handle(0), handle1(0), handle2(0), base(0), base1(0), base2(0)
	{
		version = sizeof(native_handle);
		numInts = sNumInts() + 2;
		numFds = sNumFds -2 ;
	}

	private_handle_t(int _fd, int _fd1, int _fd2, int _size, int _size1, int _size2,
			int _flags, int _w, int _h, int _format, uint64_t _internal_format, int _frameworkFormat,
			int _stride, int _vstride, int _is_compressible) :
		fd(_fd), fd1(_fd1), fd2(_fd2), magic(sMagic), flags(_flags), size(_size), size1(_size1), size2(_size2),
		offset(0), format(_format), internal_format(_internal_format), frameworkFormat(_frameworkFormat),
		width(_w), height(_h), stride(_stride), vstride(_vstride),
		is_compressible(_is_compressible), compressed_out(0), prefer_compression(0),
		lock_usage(0), lock_offset(0), lock_len(0), yuv_info(MALI_YUV_NO_INFO), PRIVATE_1(0), PRIVATE_2(0), PRIVATE_3(0), PRIVATE_4(0),
		handle(0), handle1(0), handle2(0), base(0), base1(0), base2(0)
	{
		int FDNum = 0;
		if(fd1 == -1 and fd2 == -1) FDNum = 2;
		if(fd1 != -1 and fd2 == -1) FDNum = 1;
		version = sizeof(native_handle);
		numInts = sNumInts() + FDNum;
		numFds = sNumFds - FDNum;
	}

	~private_handle_t() {
		magic = 0;
	}

	static int validate(const native_handle* h) {
		const private_handle_t* hnd = static_cast<const private_handle_t*>(h);
		if (!h || h->version != sizeof(native_handle) ||
				hnd->numInts + hnd->numFds != sNumInts() + sNumFds ||
				hnd->magic != sMagic)
		{
			ALOGE("invalid gralloc handle (at %p)", reinterpret_cast<void *>(const_cast<native_handle *>(h)));
			return -EINVAL;
		}
		return 0;
	}

	static private_handle_t* dynamicCast(const native_handle* in)
	{
		if (validate(in) == 0)
			return const_cast<private_handle_t*>(static_cast<const private_handle_t*>(in));

		return NULL;
	}
#endif
};
#endif /* GRALLOC_PRIV_H_ */
