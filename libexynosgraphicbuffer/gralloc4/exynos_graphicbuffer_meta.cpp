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

#define LOG_TAG "ExynosGraphicBuffer"

#include <ExynosGraphicBufferCore.h>
#include <ExynosGraphicBufferUtils.h>
#include <hidl_common/SharedMetadata_struct.h>
#include <log/log.h>
#include <mali_gralloc_buffer.h>
#include <mali_gralloc_formats.h>

using namespace android;
using namespace vendor::graphics;

using aidl::android::hardware::graphics::common::Dataspace;
using arm::mapper::common::shared_metadata;

#define UNUSED(x) ((void)x)
#define SZ_4k 0x1000

#ifdef GRALLOC_LEGACY_V3_HANDLE
/*
 * Only the wire block of a legacy v3 handle travels between processes. A C2
 * handle (C2HandleGralloc) is built from the fds/ints of the handle only, so
 * the gralloc4 extension (fd_count, plane_info, alloc_sizes, attr_size) is not
 * valid on it. Rebuild the extension on a local copy of the wire block.
 */
class HandleView {
  public:
    explicit HandleView(buffer_handle_t hnd) {
        if (hnd == nullptr)
            return;

        constexpr size_t wireSize = sizeof(native_handle) + PRIVATE_HANDLE_V3_WIRE_INTS * sizeof(int);

        if ((size_t)(hnd->numFds + hnd->numInts) < PRIVATE_HANDLE_V3_WIRE_INTS)
            return;

        memset(mStorage, 0, sizeof(mStorage));
        memcpy(mStorage, hnd, wireSize);

        auto *view = reinterpret_cast<private_handle_t *>(mStorage);
        if (view->magic != private_handle_t::sMagic)
            return;

        view->numInts = PRIVATE_HANDLE_V3_WIRE_INTS - view->numFds;
        view->rebuild_ext_from_wire();
        mView = view;
    }

    const private_handle_t *get() const { return mView; }

  private:
    alignas(private_handle_t) char mStorage[sizeof(private_handle_t)];
    const private_handle_t *mView = nullptr;
};

#define GRALLOC_HND(__var__, __hnd__)                                                                                  \
    HandleView __var__##_view(__hnd__);                                                                                \
    const private_handle_t *__var__ = __var__##_view.get()
#else
#define GRALLOC_HND(__var__, __hnd__) const private_handle_t *__var__ = static_cast<const private_handle_t *>(__hnd__)
#endif

uint64_t ExynosGraphicBufferMeta::get_metadata_size(buffer_handle_t hnd) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd) {
        ALOGE("gralloc handle is null");
        return 0;
    }

    return gralloc_hnd->attr_size;
}

int ExynosGraphicBufferMeta::get_video_metadata_fd(buffer_handle_t hnd) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return -EINVAL;

    uint64_t usage = gralloc_hnd->producer_usage | gralloc_hnd->consumer_usage;

    if (usage & ExynosGraphicBufferUsage::VIDEO_PRIVATE_DATA)
        return gralloc_hnd->get_share_attr_fd();
    else
        return -EINVAL;
}

#ifdef GRALLOC_LEGACY_V3_HANDLE
/* shared_metadata lives behind the gralloc3 attr_region that the vendor blobs read */
static constexpr size_t kSharedMetadataOffset = private_handle_t::kAttrPrefix;
#else
static constexpr size_t kSharedMetadataOffset = 0;
#endif

int ExynosGraphicBufferMeta::get_dataspace(buffer_handle_t hnd) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return -1;

    int attr_fd = gralloc_hnd->get_share_attr_fd();

    if (attr_fd < 0)
        return -1;

    constexpr size_t mapSize = kSharedMetadataOffset + sizeof(shared_metadata);
    void *map = mmap(0, mapSize, PROT_READ, MAP_SHARED, attr_fd, 0);
    if (map == MAP_FAILED)
        return -1;

    auto *metadata = reinterpret_cast<shared_metadata *>(static_cast<char *>(map) + kSharedMetadataOffset);
    std::optional<Dataspace> dataspace = metadata->dataspace.to_std_optional();

    int32_t ret = static_cast<int32_t>(dataspace.value_or(Dataspace::UNKNOWN));

    munmap(map, mapSize);

    return ret;
}

int ExynosGraphicBufferMeta::set_dataspace(buffer_handle_t hnd, android_dataspace_t dataspace) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return -1;

    int attr_fd = gralloc_hnd->get_share_attr_fd();

    if (attr_fd < 0)
        return -1;

    constexpr size_t mapSize = kSharedMetadataOffset + sizeof(shared_metadata);
    void *map = mmap(0, mapSize, PROT_READ | PROT_WRITE, MAP_SHARED, attr_fd, 0);
    if (map == MAP_FAILED)
        return -1;

    auto *metadata = reinterpret_cast<shared_metadata *>(static_cast<char *>(map) + kSharedMetadataOffset);
    metadata->dataspace = static_cast<Dataspace>(dataspace);
#ifdef GRALLOC_LEGACY_V3_HANDLE
    /* the vendor blobs read the dataspace from the gralloc3 attr_region */
    static_cast<attr_region_v3_t *>(map)->dataspace = static_cast<int>(dataspace);
#endif
    munmap(map, mapSize);

    return 0;
}

int ExynosGraphicBufferMeta::is_afbc(buffer_handle_t hnd) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return 0;

    if (gralloc_hnd->alloc_format & MALI_GRALLOC_INTFMT_AFBCENABLE_MASK)
        return 1;

    return 0;
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
        GRALLOC_HND(gralloc_hnd, hnd);                              \
        if (!gralloc_hnd)                                                                                              \
            return 0;                                                                                                  \
        return gralloc_hnd->__member__;                                                                                \
    }

uint32_t ExynosGraphicBufferMeta::get_format(buffer_handle_t hnd) {
    GRALLOC_HND(gralloc_hnd, hnd);
    if (!gralloc_hnd)
        return 0;

    return static_cast<uint32_t>(gralloc_hnd->alloc_format);
}

uint64_t ExynosGraphicBufferMeta::get_internal_format(buffer_handle_t hnd) {
    GRALLOC_HND(gralloc_hnd, hnd);
    if (!gralloc_hnd)
        return 0;

    return static_cast<uint64_t>(gralloc_hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK);
}

GRALLOC_META_GETTER(int, num_image_fds, fd_count);
GRALLOC_META_GETTER(uint64_t, frameworkFormat, req_format);

GRALLOC_META_GETTER(int, width, width);
GRALLOC_META_GETTER(int, height, height);
GRALLOC_META_GETTER(uint32_t, stride, stride);
GRALLOC_META_GETTER(uint32_t, vstride, plane_info[0].alloc_height);

GRALLOC_META_GETTER(uint64_t, buffer_id, backing_store_id);
GRALLOC_META_GETTER(uint64_t, producer_usage, producer_usage);
GRALLOC_META_GETTER(uint64_t, consumer_usage, consumer_usage);

GRALLOC_META_GETTER(uint64_t, flags, flags);

uint32_t ExynosGraphicBufferMeta::get_cstride(buffer_handle_t hnd) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return 0;

    uint32_t stride = gralloc_hnd->plane_info[1].alloc_width;
    if (is_semi_planar(static_cast<uint64_t>(gralloc_hnd->alloc_format))) {
        stride = gralloc_hnd->plane_info[0].alloc_width;
    }

    return stride;
}

int ExynosGraphicBufferMeta::get_fd(buffer_handle_t hnd, int num) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return -1;

    if (num > 2)
        return -1;

    return gralloc_hnd->fds[num];
}

int ExynosGraphicBufferMeta::get_size(buffer_handle_t hnd, int num) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return 0;

    if (num > 2)
        return 0;

    return gralloc_hnd->alloc_sizes[num];
}

uint64_t ExynosGraphicBufferMeta::get_usage(buffer_handle_t hnd) {
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return 0;

    return gralloc_hnd->producer_usage | gralloc_hnd->consumer_usage;
}

void *ExynosGraphicBufferMeta::get_video_metadata(buffer_handle_t hnd) {
    private_handle_t *gralloc_hnd = static_cast<private_handle_t *>(const_cast<native_handle_t *>(hnd));

    if (!gralloc_hnd)
        return nullptr;

#ifdef GRALLOC_LEGACY_V3_HANDLE
    return gralloc_hnd->metadata_base();
#else
    return gralloc_hnd->attr_base;
#endif
}

void *ExynosGraphicBufferMeta::get_video_metadata_roiinfo(buffer_handle_t hnd) {
    private_handle_t *gralloc_hnd = static_cast<private_handle_t *>(const_cast<native_handle_t *>(hnd));

    if (!gralloc_hnd)
        return nullptr;

    if (gralloc_hnd->get_usage() & ExynosGraphicBufferUsage::ROIINFO)
        return static_cast<char *>(gralloc_hnd->attr_base) + SZ_4k * 2;

    return nullptr;
}

int ExynosGraphicBufferMeta::get_pad_align(buffer_handle_t hnd, pad_align_t *pad_align) {
    UNUSED(pad_align);
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd)
        return -EINVAL;

    return 0;
}

int64_t ExynosGraphicBufferMeta::get_plane_offset(buffer_handle_t hnd, int plane_num) {
    GRALLOC_HND(gralloc_hnd, hnd);

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
    GRALLOC_HND(gralloc_hnd, hnd);

    if (!gralloc_hnd) {
        ALOGE("gralloc handle is null");
        return;
    }

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
    GRALLOC_HND(gralloc_hnd, handle);

    if (!gralloc_hnd)
        return;

    fd = gralloc_hnd->fds[0];
    fd1 = gralloc_hnd->fds[1];
    fd2 = gralloc_hnd->fds[2];

    size = gralloc_hnd->alloc_sizes[0];
    size1 = gralloc_hnd->alloc_sizes[1];
    size2 = gralloc_hnd->alloc_sizes[2];

    uint64_t usage = gralloc_hnd->producer_usage | gralloc_hnd->consumer_usage;
    if (usage & ExynosGraphicBufferUsage::VIDEO_PRIVATE_DATA) {
        switch (gralloc_hnd->get_share_attr_fd_index()) {
            case 1:
                size1 = gralloc_hnd->attr_size;
                break;
            case 2:
                size2 = gralloc_hnd->attr_size;
                break;
        }
    }

    internal_format = gralloc_hnd->alloc_format & MALI_GRALLOC_INTFMT_FMT_MASK;
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
