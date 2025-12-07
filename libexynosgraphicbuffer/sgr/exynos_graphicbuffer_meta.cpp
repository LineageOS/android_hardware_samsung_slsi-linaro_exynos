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

#include "ExynosGraphicBufferCore.h"
#include "ExynosGraphicBufferUtils.h"
#include "metadata_gpu.h"
#include "metadata_gralloc.h"
#include "private_handle.h"
#include <errno.h>
#include <exynos_format.h>
#include <inttypes.h>
#include <log/log.h>
#include <string.h>
#include <sys/mman.h>

using namespace vendor::graphics;

#define UNUSED(x) ((void)x)
#define SZ_4k 0x1000

#define SGR_LOGD(...)
#define SGR_LOGE(...) ALOGE(__VA_ARGS__)

constexpr int SGR_META_OFFSET[3] = {SGR_METADATA_OFFSET_VIDEO, SGR_METADATA_OFFSET_GRALLOC, SGR_METADATA_OFFSET_GPU};
enum METADATA_TYPE {
    TYPE_VIDEO = 0,
    TYPE_GRALLOC,
    TYPE_GPU,
};

static void *map_and_get_metadata(buffer_handle_t hnd, METADATA_TYPE type, bool write) {
    if (hnd == nullptr) {
        SGR_LOGD("[%s] buffer handle is null", __func__);
        return nullptr;
    }
    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    const uint32_t metadata_fd_index = phnd->numFds - 1;
    int prot = write ? PROT_READ | PROT_WRITE : PROT_READ;

    void *metadata = mmap(0, SGR_METADATA_SIZE_SUB_TOTAL, prot, MAP_SHARED, phnd->fds[metadata_fd_index], 0);
    if (metadata == MAP_FAILED) {
        SGR_LOGE("[%s] mmap failed: %s", __func__, strerror(errno));
        return nullptr;
    }

    return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(metadata) + SGR_META_OFFSET[type]);
}

static void unmap_metadata(void *metadata, METADATA_TYPE type) {
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(metadata) - SGR_META_OFFSET[type]);
    munmap(addr, SGR_METADATA_SIZE_SUB_TOTAL);
}

int ExynosGraphicBufferMeta::get_video_metadata_fd(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -1;
    }

    int ret = -1;
    if (phnd->usage & ExynosGraphicBufferUsage::VIDEO_PRIVATE_DATA) {
        const uint32_t metadata_fd_index = phnd->numFds - 1;
        SGR_LOGD("[%s] usage has video_private_data, numFds %d, fdindex %u", __func__, phnd->numFds, metadata_fd_index);
        ret = phnd->fds[metadata_fd_index];
    } else {
        SGR_LOGD("[%s] usage has NO video_private_data", __func__);
    }

    return ret;
}

int ExynosGraphicBufferMeta::get_dataspace(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    int ret = HAL_DATASPACE_UNKNOWN;
    struct sgr_metadata *metadata = (struct sgr_metadata *)map_and_get_metadata(hnd, TYPE_GRALLOC, false);
    if (metadata == nullptr) {
        SGR_LOGE("[%s] metadata is null", __func__);
        return ret;
    }
    ret = metadata->dataspace;
    unmap_metadata((void *)metadata, TYPE_GRALLOC);

    return ret;
}

int ExynosGraphicBufferMeta::set_dataspace(buffer_handle_t hnd, android_dataspace_t dataspace) {
    SGR_LOGD("[%s] entry", __func__);

    struct sgr_metadata *metadata = (struct sgr_metadata *)map_and_get_metadata(hnd, TYPE_GRALLOC, true);
    if (metadata == nullptr) {
        SGR_LOGE("[%s] metadata is null", __func__);
        return -1;
    }
    int temp = metadata->dataspace;
    metadata->dataspace = dataspace;
    unmap_metadata((void *)metadata, TYPE_GRALLOC);

    SGR_LOGD("[%s] dataspace changed %d to %d", __func__, temp, dataspace);
    UNUSED(temp);

    return 0;
}

bool ExynosGraphicBufferMeta::is_sajc(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return false;
    }

    return (phnd->alloc_layout == SGR_ALLOC_LAYOUT_DCC);
}

int ExynosGraphicBufferMeta::is_afbc(buffer_handle_t) {
    return 0;
}

uint64_t ExynosGraphicBufferMeta::get_buffer_id(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    struct sgr_metadata *metadata = sgr_get_metadata(hnd);
    if (metadata == nullptr) {
        SGR_LOGD("metadata is null(not imported), so mmap gralloc metadata");

        metadata = (struct sgr_metadata *)map_and_get_metadata(hnd, TYPE_GRALLOC, false);
        if (metadata == nullptr) {
            SGR_LOGE("[%s] metadata is null, return id as 0", __func__);
            return 0;
        }
        uint64_t ret = metadata->buffer_id;
        unmap_metadata((void *)metadata, TYPE_GRALLOC);

        return ret;
    }
    return metadata->buffer_id;
}

#define GRALLOC_META_GETTER(__type__, __name__, __member__)                                                            \
    __type__ ExynosGraphicBufferMeta::get_##__name__(buffer_handle_t hnd) {                                            \
        SGR_LOGD("[%s] entry", __func__);                                                                              \
        const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);                                     \
        if (phnd == nullptr) {                                                                                         \
            SGR_LOGD("[%s] private_handle is null", __func__);                                                         \
            return -EINVAL;                                                                                            \
        }                                                                                                              \
        return phnd->__member__;                                                                                       \
    }

GRALLOC_META_GETTER(int, num_image_fds, num_allocs);
GRALLOC_META_GETTER(uint32_t, format, alloc_format);
GRALLOC_META_GETTER(uint64_t, internal_format, alloc_format);
GRALLOC_META_GETTER(uint64_t, frameworkFormat, format);

GRALLOC_META_GETTER(int, width, width);
GRALLOC_META_GETTER(int, height, height);

GRALLOC_META_GETTER(uint32_t, vstride, plane_infos[0].height_in_samples);

GRALLOC_META_GETTER(uint64_t, producer_usage, usage);
GRALLOC_META_GETTER(uint64_t, consumer_usage, usage);
GRALLOC_META_GETTER(uint64_t, usage, usage);

uint32_t ExynosGraphicBufferMeta::get_stride(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return 0;
    }

    uint32_t stride;
    switch (phnd->alloc_format) {
        case HAL_PIXEL_FORMAT_RAW10:
        case HAL_PIXEL_FORMAT_RAW12:
            stride = phnd->plane_infos[0].stride_in_bytes;
            break;
        default:
            stride = phnd->plane_infos[0].width_in_samples;
            break;
    }
    return stride;
}

uint32_t ExynosGraphicBufferMeta::get_cstride(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return 0;
    }

    uint32_t stride = phnd->plane_infos[1].width_in_samples;
    if (is_semi_planar(phnd->alloc_format)) {
        stride = phnd->plane_infos[0].width_in_samples;
    }

    return stride;
}

int ExynosGraphicBufferMeta::get_fd(buffer_handle_t hnd, int num) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -1;
    }

    return phnd->fds[num];
}

int ExynosGraphicBufferMeta::get_size(buffer_handle_t hnd, int num) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return 0;
    }

    return phnd->alloc_infos[num].size;
}

void *ExynosGraphicBufferMeta::get_video_metadata(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    void *metadata = sgr_get_metadata_video(const_cast<native_handle_t *>(hnd));
    SGR_LOGD("[%s] metadata %p", __func__, metadata);
    if (metadata == nullptr) {
        SGR_LOGD("[%s] metadata is null", __func__);
        return nullptr;
    }

    return metadata;
}

void *ExynosGraphicBufferMeta::get_video_metadata_roiinfo(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return nullptr;
    }

    void *video_metadata = sgr_get_metadata_video(hnd);
    // TODO
    if (phnd->usage & ExynosGraphicBufferUsage::ROIINFO) {
        SGR_LOGE("[%s] ROIINFO this shouldn't be called", __func__);
        return reinterpret_cast<char *>(video_metadata) + SZ_4k * 2;
    }

    return nullptr;
}

int64_t ExynosGraphicBufferMeta::get_plane_offset(buffer_handle_t hnd, int plane_num) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    uint32_t buffer_num_planes = phnd->num_planes;
    if (plane_num > buffer_num_planes) {
        SGR_LOGD("[%s] buffer have %" PRIu32 " planes, but input is %d", __func__, buffer_num_planes, plane_num);
        return -EINVAL;
    }

    return phnd->plane_infos[plane_num].offset;
}

int ExynosGraphicBufferMeta::get_pad_align(buffer_handle_t hnd, pad_align_t *pad_align) {
    SGR_LOGD("[%s] entry", __func__);

    UNUSED(pad_align);
    const private_handle_t *gralloc_hnd = static_cast<const private_handle_t *>(hnd);

    if (!gralloc_hnd)
        return -EINVAL;

    return 0;
}

int64_t ExynosGraphicBufferMeta::get_sub_plane_offset(buffer_handle_t hnd, int plane_num) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    if (~phnd->usage & ExynosGraphicBufferUsage::DOWNSCALE) {
        SGR_LOGD("[%s] buffer has no usage for DOWNSCALE", __func__);
        return -EINVAL;
    }

    uint32_t sub_num_plane = phnd->num_planes / 2;
    if (plane_num > sub_num_plane) {
        SGR_LOGE("[%s] sub have %" PRIu32 " planes, but input is %d", __func__, sub_num_plane, plane_num);
        return -EINVAL;
    }

    return phnd->plane_infos[sub_num_plane + plane_num].offset;
}

int32_t ExynosGraphicBufferMeta::get_sajc_independent_block_size(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    int32_t ret = -1;
    if (phnd->alloc_layout != SGR_ALLOC_LAYOUT_DCC) {
        SGR_LOGE("[%s] buffer is not SAJC buffer", __func__);
    } else {
        uint32_t sajc_independent_64;
        uint32_t sajc_independent_128;

        struct sgr_metadata_gpu *gpu_meta = (struct sgr_metadata_gpu *)sgr_get_metadata_gpu(hnd);
        if (gpu_meta == nullptr) {
            SGR_LOGD("metadata is null(not imported), so mmap gpu metadata");

            gpu_meta = (struct sgr_metadata_gpu *)map_and_get_metadata(hnd, TYPE_GPU, false);
            if (gpu_meta == nullptr) {
                SGR_LOGE("[%s] metadata is null", __func__);
                return ret;
            }
            sajc_independent_64 = gpu_meta->dcc_independent_block_size_64;
            sajc_independent_128 = gpu_meta->dcc_independent_block_size_128;
            ret = (sajc_independent_128 << 1) | (sajc_independent_64);
            unmap_metadata((void *)gpu_meta, TYPE_GPU);

            return ret;
        }
        sajc_independent_64 = gpu_meta->dcc_independent_block_size_64;
        sajc_independent_128 = gpu_meta->dcc_independent_block_size_128;

        ret = (sajc_independent_128 << 1) | (sajc_independent_64);
    }

    return ret;
}

int32_t ExynosGraphicBufferMeta::get_sajc_key_offset(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    int32_t ret = -1;
    if (phnd->alloc_layout != SGR_ALLOC_LAYOUT_DCC) {
        SGR_LOGE("[%s] buffer is not SAJC buffer", __func__);
    } else {
        ret = static_cast<int32_t>(phnd->alloc_infos[0].key_offset);
    }

    return ret;
}

int32_t ExynosGraphicBufferMeta::get_sajc_sw_mode(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    int32_t ret = -1;
    if (phnd->alloc_layout != SGR_ALLOC_LAYOUT_DCC) {
        SGR_LOGE("[%s] buffer is not SAJC buffer", __func__);
    } else {
        struct sgr_metadata_gpu *gpu_meta = (struct sgr_metadata_gpu *)sgr_get_metadata_gpu(hnd);
        if (gpu_meta == nullptr) {
            SGR_LOGD("metadata is null(not imported), so mmap gpu metadata");

            gpu_meta = (struct sgr_metadata_gpu *)map_and_get_metadata(hnd, TYPE_GPU, false);
            if (gpu_meta == nullptr) {
                SGR_LOGE("[%s] metadata is null", __func__);
                return ret;
            }

            ret = gpu_meta->dcc_sw_mode;
            unmap_metadata((void *)gpu_meta, TYPE_GPU);

            return ret;
        }
        ret = gpu_meta->dcc_sw_mode;
    }

    return ret;
}

int32_t ExynosGraphicBufferMeta::get_sub_stride(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    if (~phnd->usage & ExynosGraphicBufferUsage::DOWNSCALE) {
        SGR_LOGD("[%s] buffer has no usage for DOWNSCALE", __func__);
        return -EINVAL;
    }

    uint32_t sub_idx = phnd->num_allocs / 2;
    return static_cast<int32_t>(phnd->plane_infos[sub_idx].width_in_samples);
}

int32_t ExynosGraphicBufferMeta::get_sub_vstride(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    if (~phnd->usage & ExynosGraphicBufferUsage::DOWNSCALE) {
        SGR_LOGD("[%s] buffer has no usage for DOWNSCALE", __func__);
        return -EINVAL;
    }

    uint32_t sub_idx = phnd->num_allocs / 2;
    return static_cast<int32_t>(phnd->plane_infos[sub_idx].height_in_samples);
}

int32_t ExynosGraphicBufferMeta::get_sub_format(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    if (~phnd->usage & ExynosGraphicBufferUsage::DOWNSCALE) {
        SGR_LOGD("[%s] buffer has no usage for DOWNSCALE", __func__);
        return -EINVAL;
    }

    uint32_t alloc_format = phnd->alloc_format;

    struct sub_format_list {
        uint32_t base_format;
        uint32_t sbwc_format;
    };

    static const sub_format_list format_table[] = {
        {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC},
        {HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC},
    };

    int32_t sub_format = 0;
    for (uint32_t i = 0; i < sizeof(format_table) / sizeof(format_table[0]); i++) {
        if (alloc_format == format_table[i].base_format) {
            sub_format = static_cast<int32_t>(format_table[i].sbwc_format);
            break;
        }
    }

    return sub_format;
}

bool ExynosGraphicBufferMeta::get_sub_valid(buffer_handle_t hnd) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return false;
    }

    if (~phnd->usage & ExynosGraphicBufferUsage::DOWNSCALE) {
        SGR_LOGD("[%s] buffer has no usage for DOWNSCALE", __func__);
        return false;
    }

    struct sgr_metadata *metadata = (struct sgr_metadata *)map_and_get_metadata(hnd, TYPE_GRALLOC, false);
    if (metadata == nullptr) {
        SGR_LOGE("[%s] metadata is null", __func__);
        return false;
    }

    bool ret = metadata->sub_valid;
    unmap_metadata((void *)metadata, TYPE_GRALLOC);

    return ret;
}

int ExynosGraphicBufferMeta::set_sub_valid(buffer_handle_t hnd, bool valid) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(hnd);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return -EINVAL;
    }

    if (~phnd->usage & ExynosGraphicBufferUsage::DOWNSCALE) {
        SGR_LOGD("[%s] buffer has no usage for DOWNSCALE", __func__);
        return -EINVAL;
    }

    struct sgr_metadata *metadata = (struct sgr_metadata *)map_and_get_metadata(hnd, TYPE_GRALLOC, true);
    if (metadata == nullptr) {
        SGR_LOGE("[%s] metadata is null", __func__);
        return -EINVAL;
    }

    metadata->sub_valid = valid;
    unmap_metadata((void *)metadata, TYPE_GRALLOC);

    return 0;
}

void ExynosGraphicBufferMeta::init(const buffer_handle_t handle) {
    SGR_LOGD("[%s] entry", __func__);

    const private_handle_t *phnd = static_cast<const private_handle_t *>(handle);
    if (phnd == nullptr) {
        SGR_LOGD("[%s] private_handle is null", __func__);
        return;
    }

    fd = phnd->fds[0];
    fd1 = phnd->fds[1];
    fd2 = phnd->fds[2];

    SGR_LOGD("[%s] fds (%d, %d, %d)", __func__, fd, fd1, fd2);

    size = phnd->alloc_infos[0].size;
    size1 = phnd->alloc_infos[1].size;
    size2 = phnd->alloc_infos[2].size;

    SGR_LOGD("[%s] size (%d, %d, %d)", __func__, size, size1, size2);

    uint64_t usage = phnd->usage;
    if (usage & ExynosGraphicBufferUsage::VIDEO_PRIVATE_DATA) {
        SGR_LOGD("[%s] VIDEO_PRIVATE_DATA", __func__);
        const uint32_t metadata_fd_index = phnd->numFds - 1;

        switch (metadata_fd_index) {
            case 1:
                size1 = SGR_METADATA_SIZE_SUB_TOTAL;
                break;
            case 2:
                size2 = SGR_METADATA_SIZE_SUB_TOTAL;
                break;
        }
    }

    internal_format = phnd->alloc_format;
    frameworkFormat = phnd->format;

    SGR_LOGD("[%s] internal_format %x, frameworkFormat %x", __func__, (int)internal_format, (int)frameworkFormat);

    width = phnd->width;
    height = phnd->height;
    stride = phnd->alloc_width;
    vstride = phnd->alloc_height;

    SGR_LOGD("[%s] width %d height %d, strid %d, vstride %d", __func__, width, height, stride, vstride);

    producer_usage = usage;
    consumer_usage = usage;

    SGR_LOGD("[%s] usage %" PRIx64, __func__, usage);
    flags = 0;
}

ExynosGraphicBufferMeta::ExynosGraphicBufferMeta(buffer_handle_t handle) {
    init(handle);
}
