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

#include <ExynosGraphicBufferCore.h>
#include <ExynosGraphicBufferUtils.h>
#include <gralloc_buffer_priv.h>
#include <log/log.h>
#include <mali_gralloc_buffer.h>

#define UNUSED(x) ((void)x)

using namespace vendor::graphics;

int ExynosGraphicBufferMeta::is_afbc(buffer_handle_t buffer_hnd_p) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(buffer_hnd_p);

    if (!gralloc_hnd)
        return 0;

    return gralloc_hnd->is_compressible;
}

#define GRALLOC_META_NO_IMPL(__type__, __funcname__)                                                                   \
    __type__ ExynosGraphicBufferMeta::__funcname__(buffer_handle_t hnd) {                                              \
        UNUSED(hnd);                                                                                                   \
        return {};                                                                                                     \
    }

GRALLOC_META_NO_IMPL(bool, is_sajc);
GRALLOC_META_NO_IMPL(int32_t, get_sajc_key_offset);
GRALLOC_META_NO_IMPL(int32_t, get_sajc_independent_block_size);
GRALLOC_META_NO_IMPL(int32_t, get_sajc_sw_mode);

GRALLOC_META_NO_IMPL(int32_t, get_sub_format);
GRALLOC_META_NO_IMPL(int32_t, get_sub_stride);
GRALLOC_META_NO_IMPL(int32_t, get_sub_vstride);
GRALLOC_META_NO_IMPL(bool, get_sub_valid);

#define GRALLOC_META_GETTER(__type__, __name__, __member__)                                                            \
    __type__ ExynosGraphicBufferMeta::get_##__name__(buffer_handle_t hnd) {                                            \
        const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);                              \
        if (!gralloc_hnd)                                                                                              \
            return 0;                                                                                                  \
        return gralloc_hnd->__member__;                                                                                \
    }

GRALLOC_META_GETTER(uint32_t, format, format);
GRALLOC_META_GETTER(uint64_t, internal_format, internal_format);
GRALLOC_META_GETTER(uint64_t, frameworkFormat, req_format);

GRALLOC_META_GETTER(int, width, width);
GRALLOC_META_GETTER(int, height, height);
GRALLOC_META_GETTER(uint32_t, stride, stride);
GRALLOC_META_GETTER(uint32_t, vstride, plane_info[0].alloc_height);

GRALLOC_META_GETTER(uint64_t, buffer_id, backing_store_id);
GRALLOC_META_GETTER(uint64_t, producer_usage, producer_usage);
GRALLOC_META_GETTER(uint64_t, consumer_usage, consumer_usage);

GRALLOC_META_GETTER(uint64_t, flags, flags);

uint64_t ExynosGraphicBufferMeta::get_metadata_size(buffer_handle_t hnd) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd) {
        ALOGE("gralloc handle is null");
        return 0;
    }

    int idx = -1;
    if (gralloc_hnd->flags & ExynosGraphicBufferMeta::PRIV_FLAGS_USES_2PRIVATE_DATA)
        idx = 1;
    else if (gralloc_hnd->flags & ExynosGraphicBufferMeta::PRIV_FLAGS_USES_3PRIVATE_DATA)
        idx = 2;

    if (idx < 0) {
        ALOGE("handle doesn't have proper flags for metadata");
        return 0;
    }

    return gralloc_hnd->sizes[idx];
}

int ExynosGraphicBufferMeta::get_dataspace(buffer_handle_t hnd) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return -1;

    int attr_fd = gralloc_hnd->get_share_attr_fd();
    if (attr_fd < 0) {
        ALOGE("Shared attribute region not available to be mapped");
        return -1;
    }
    attr_region *region;
    region = (attr_region *)mmap(NULL, sizeof(attr_region), PROT_READ, MAP_SHARED, attr_fd, 0);
    if (region == NULL)
        return -1;
    else if (region == MAP_FAILED)
        return -1;

    int dataspace = region->force_dataspace == -1 ? region->dataspace : region->force_dataspace;

    munmap(region, sizeof(attr_region));

    return dataspace;
}

int ExynosGraphicBufferMeta::set_dataspace(buffer_handle_t hnd, android_dataspace_t dataspace) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return -1;

    int attr_fd = gralloc_hnd->get_share_attr_fd();

    if (attr_fd < 0)
        return -1;

    attr_region *region =
        (attr_region *)mmap(NULL, sizeof(attr_region), PROT_READ | PROT_WRITE, MAP_SHARED, attr_fd, 0);
    if (region == NULL)
        return -1;
    else if (region == MAP_FAILED)
        return -1;

    region->dataspace = dataspace;
    region->force_dataspace = dataspace;
    munmap(region, sizeof(attr_region));

    return 0;
}

uint32_t ExynosGraphicBufferMeta::get_cstride(buffer_handle_t hnd) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return 0;

    uint32_t stride = gralloc_hnd->plane_info[1].alloc_width;
    if (is_semi_planar(static_cast<uint32_t>(gralloc_hnd->alloc_format))) {
        stride = gralloc_hnd->plane_info[0].alloc_width;
    }

    return stride;
}

int ExynosGraphicBufferMeta::get_fd(buffer_handle_t hnd, int num) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return -1;

    if (num > 2)
        return -1;

    return gralloc_hnd->fds[num];
}

int ExynosGraphicBufferMeta::get_num_image_fds(buffer_handle_t hnd) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return -1;

    int num_fd = hnd->numFds;
    uint64_t usage = gralloc_hnd->producer_usage | gralloc_hnd->consumer_usage;
    if (usage & ExynosGraphicBufferUsage::VIDEO_PRIVATE_DATA) {
        num_fd--;
    }

    return num_fd;
}

int ExynosGraphicBufferMeta::get_size(buffer_handle_t hnd, int num) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return 0;

    if (num > 2)
        return 0;

    return gralloc_hnd->sizes[num];
}

uint64_t ExynosGraphicBufferMeta::get_usage(buffer_handle_t hnd) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return 0;

    return gralloc_hnd->producer_usage | gralloc_hnd->consumer_usage;
}

void *ExynosGraphicBufferMeta::get_video_metadata(buffer_handle_t hnd) {
    private_handle_t *gralloc_hnd = static_cast<private_handle_t *>(const_cast<native_handle_t *>(hnd));

    if (!gralloc_hnd)
        return nullptr;

    int idx = -1;

    if (gralloc_hnd->flags & private_handle_t::PRIV_FLAGS_USES_2PRIVATE_DATA)
        idx = 1;
    else if (gralloc_hnd->flags & private_handle_t::PRIV_FLAGS_USES_3PRIVATE_DATA)
        idx = 2;

    if (idx == -1)
        return nullptr;

    if (gralloc_hnd->bases[idx])
        return (void *)gralloc_hnd->bases[idx];
    else {
        ALOGW("gralloc handle must be registered before video metadata can be used");
        return nullptr;
    }
}

int ExynosGraphicBufferMeta::get_video_metadata_fd(buffer_handle_t hnd) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return -EINVAL;

    int idx = -1;

    if (gralloc_hnd->flags & ExynosGraphicBufferMeta::PRIV_FLAGS_USES_2PRIVATE_DATA)
        idx = 1;
    else if (gralloc_hnd->flags & ExynosGraphicBufferMeta::PRIV_FLAGS_USES_3PRIVATE_DATA)
        idx = 2;

    if (idx < 0)
        return -EINVAL;

    return gralloc_hnd->fds[idx];
}

/* This function is not supported with gralloc3. return nullptr */
void *ExynosGraphicBufferMeta::get_video_metadata_roiinfo(buffer_handle_t hnd) {
    UNUSED(hnd);
    return nullptr;
}

int ExynosGraphicBufferMeta::get_pad_align(buffer_handle_t hnd, pad_align_t *pad_align) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return -EINVAL;

    pad_align->pad.w = gralloc_hnd->pad_w;
    pad_align->pad.h = gralloc_hnd->pad_h;
    pad_align->align.w = gralloc_hnd->alignment_w;
    pad_align->align.h = gralloc_hnd->alignment_h;

    return 0;
}

int64_t ExynosGraphicBufferMeta::get_plane_offset(buffer_handle_t hnd, int plane_num) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return -EINVAL;

    return gralloc_hnd->plane_info[plane_num].offset;
}

int64_t ExynosGraphicBufferMeta::get_sub_plane_offset(buffer_handle_t hnd, int plane_num) {
    UNUSED(hnd);
    UNUSED(plane_num);
    return -1;
}

int ExynosGraphicBufferMeta::set_sub_valid(buffer_handle_t hnd, bool valid) {
    UNUSED(hnd);
    UNUSED(valid);
    return -1;
}

void ExynosGraphicBufferMeta::dump_hnd(buffer_handle_t hnd, const char *str) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return;

    gralloc_hnd->dump(str);
}

void ExynosGraphicBufferMeta::dump(const char *str) {
    ALOGE("[%s] "
          "fd(%d %d %d) "
          "size(%d %d %d) "
          "format(0x%" PRIx32 ") "
          "internal_format(0x%" PRIx64 ") "
          "framework_format(%d) "
          "w/h(%d %d) "
          "stride(0x%" PRIu32 ") "
          "vstride(0x%" PRIu32 ") "
          "producer_usage(0x%" PRIx64 ") "
          "consumer_usage(0x%" PRIx64 ") "
          "flag(%d)",
          str, fd, fd1, fd2, size, size1, size2, format, internal_format, frameworkFormat, width, height, stride,
          vstride, producer_usage, consumer_usage, flags);
}

void ExynosGraphicBufferMeta::init(const buffer_handle_t handle) {
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(handle);

    if (!gralloc_hnd)
        return;

    fd = gralloc_hnd->fds[0];
    fd1 = gralloc_hnd->fds[1];
    fd2 = gralloc_hnd->fds[2];

    size = gralloc_hnd->sizes[0];
    size1 = gralloc_hnd->sizes[1];
    size2 = gralloc_hnd->sizes[2];

    internal_format = gralloc_hnd->internal_format;
    frameworkFormat = gralloc_hnd->req_format;

    width = gralloc_hnd->width;
    height = gralloc_hnd->height;
    stride = gralloc_hnd->stride;
    vstride = gralloc_hnd->plane_info[0].alloc_height;

    producer_usage = gralloc_hnd->producer_usage;
    consumer_usage = gralloc_hnd->consumer_usage;

    flags = gralloc_hnd->flags;
}

ExynosGraphicBufferMeta::ExynosGraphicBufferMeta(buffer_handle_t handle) {
    init(handle);
}
