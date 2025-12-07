/*
 * Copyright (C) 2020 Samsung Electronics Co. Ltd.
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

#ifndef EXYNOS_GRAPHIC_BUFFER_CORE_H_
#define EXYNOS_GRAPHIC_BUFFER_CORE_H_

#include <cstdint>
#include <cutils/native_handle.h>
#include <system/graphics.h>

namespace vendor {
namespace graphics {

/* S.LSI specific usages */
enum ExynosGraphicBufferUsage {
    GDC_MODE = 1ULL << 28,
    NO_AFBC = 1ULL << 29,
    CAMERA_RESERVED = 1ULL << 30,
    SECURE_CAMERA_RESERVED = 1ULL << 31,
    DOWNSCALE = 1ULL << 51,
    ROIINFO = 1ULL << 52,
    AFBC_PADDING = 1ULL << 53,
    FORCE_BACKBUFFER = 1ULL << 54,
    FRONTBUFFER = 1ULL << 55,
    SBWC_REQUEST_10BIT = 1ULL << 56,
    HFR_MODE = 1ULL << 57,
    NOZEROED = 1ULL << 58,
    PRIVATE_NONSECURE = 1ULL << 59,
    VIDEO_PRIVATE_DATA = 1ULL << 60,
    VIDEO_EXT = 1ULL << 61,
    DAYDREAM_SINGLE_BUFFER_MODE = 1ULL << 62,
    YUV_RANGE_FULL = 1ULL << 63,
};

/* A wrapper class to gralloc private handle */
class ExynosGraphicBufferMeta {
  public:
    enum {
        PRIV_FLAGS_USES_2PRIVATE_DATA = 1U << 4,
        PRIV_FLAGS_USES_3PRIVATE_DATA = 1U << 5,
    };

    typedef struct pad_align {
        struct {
            short w;
            short h;
        } pad;

        struct {
            short w;
            short h;
        } align;

        pad_align() {
            pad.w = 0;
            pad.h = 0;
            align.w = 0;
            align.h = 0;
        }
    } pad_align_t;

    int fd = -1;
    int fd1 = -1;
    int fd2 = -1;

    int size = 0;
    int size1 = 0;
    int size2 = 0;

    union {
        uint32_t format;
        uint64_t internal_format = 0llu;
    };
    int frameworkFormat = 0;

    int width = 0;
    int height = 0;
    uint32_t stride = 0;
    uint32_t vstride = 0;

    uint64_t producer_usage = 0llu;
    uint64_t consumer_usage = 0llu;

    int flags = 0;

    ExynosGraphicBufferMeta(){};
    ExynosGraphicBufferMeta(const buffer_handle_t handle);

    void init(const buffer_handle_t handle);
    void dump(const char *str);

    static int get_fd(buffer_handle_t, int num);
    static int get_num_image_fds(buffer_handle_t);
    static int get_size(buffer_handle_t, int num);
    static uint32_t get_format(buffer_handle_t);
    static uint64_t get_internal_format(buffer_handle_t);
    static uint64_t get_frameworkFormat(buffer_handle_t);
    static int get_width(buffer_handle_t);
    static int get_height(buffer_handle_t);
    static uint32_t get_stride(buffer_handle_t);
    static uint32_t get_vstride(buffer_handle_t);
    static uint32_t get_cstride(buffer_handle_t);
    static uint64_t get_producer_usage(buffer_handle_t);
    static uint64_t get_consumer_usage(buffer_handle_t);
    static uint64_t get_flags(buffer_handle_t);

    static uint64_t get_buffer_id(buffer_handle_t);
    static uint64_t get_usage(buffer_handle_t);
    static int is_afbc(buffer_handle_t);
    static bool is_sajc(buffer_handle_t);
    static void *get_video_metadata(buffer_handle_t);
    static void dump_hnd(buffer_handle_t, const char *str);

    /* get_video_metadata_roiinfo is only supported with gralloc4
     * When gralloc3 is used, will always return nullptr
     */
    static void *get_video_metadata_roiinfo(buffer_handle_t);
    static int get_pad_align(buffer_handle_t, pad_align_t *pad_align);
    static int get_video_metadata_fd(buffer_handle_t);
    static int get_dataspace(buffer_handle_t);
    static int set_dataspace(buffer_handle_t hnd, android_dataspace_t dataspace);
    static uint64_t get_metadata_size(buffer_handle_t);
    static int64_t get_plane_offset(buffer_handle_t, int plane_num);

    /* SAJC interface */
    static int32_t get_sajc_independent_block_size(buffer_handle_t);
    static int32_t get_sajc_key_offset(buffer_handle_t);
    static int32_t get_sajc_sw_mode(buffer_handle_t);

    /* Sub-Allocation interface */
    static int32_t get_sub_format(buffer_handle_t);
    static int32_t get_sub_stride(buffer_handle_t);
    static int32_t get_sub_vstride(buffer_handle_t);
    static int64_t get_sub_plane_offset(buffer_handle_t, int);
    static bool get_sub_valid(buffer_handle_t);
    static int set_sub_valid(buffer_handle_t, bool);
};

} /* namespace graphics */
} /* namespace vendor */

#endif /* EXYNOS_GRAPHIC_BUFFER_CORE_H_ */
