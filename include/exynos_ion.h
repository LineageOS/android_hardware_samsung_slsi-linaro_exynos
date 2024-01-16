/*
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
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

#ifndef _LIB_ION_H_
#define _LIB_ION_H_

#ifndef ION_HEAP_SYSTEM_MASK
#define ION_HEAP_SYSTEM_MASK            (1 << 0)
#endif
#define ION_HEAP_EXYNOS_CONTIG_MASK     (1 << 4)
#define ION_EXYNOS_VIDEO_EXT_MASK       (1 << 31)
#define ION_EXYNOS_VIDEO_EXT2_MASK      (1 << 29)
#define ION_EXYNOS_FIMD_VIDEO_MASK      (1 << 28)
#define ION_EXYNOS_GSC_MASK             (1 << 27)
#define ION_EXYNOS_MFC_OUTPUT_MASK      (1 << 26)
#define ION_EXYNOS_MFC_INPUT_MASK       (1 << 25)
#define ION_EXYNOS_G2D_WFD_MASK         (1 << 22)
#define ION_EXYNOS_VIDEO_MASK           (1 << 21)

#define EXYNOS_ION_HEAP_CRYPTO_MASK         (1 << ION_EXYNOS_HEAP_ID_CRYPTO)
#define EXYNOS_ION_HEAP_VIDEO_STREAM_MASK   (1 << ION_EXYNOS_HEAP_ID_VIDEO_STREAM)
#define EXYNOS_ION_HEAP_VIDEO_FRAME_MASK    (1 << ION_EXYNOS_HEAP_ID_VIDEO_FRAME)
#define EXYNOS_ION_HEAP_VIDEO_SCALER_MASK   (1 << ION_EXYNOS_HEAP_ID_VIDEO_SCALER)
#define EXYNOS_ION_HEAP_CAMERA              (1 << ION_EXYNOS_HEAP_ID_CAMERA)
#define EXYNOS_ION_HEAP_SECURE_CAMERA       (1 << ION_EXYNOS_HEAP_ID_SECURE_CAMERA)

#define ION_EXYNOS_FLAG_PROTECTED (1 << 16)

/* Exynos specific ION allocation flag */
#define ION_FLAG_PRESERVE_KMAP  4
#define ION_FLAG_NOZEROED       8
#define ION_FLAG_PROTECTED     (16 | ION_EXYNOS_FLAG_PROTECTED)
#define ION_FLAG_SYNC_FORCE     32
#define ION_FLAG_MAY_HWRENDER   64

#include <sys/types.h>
#include <sys/ioctl.h>
#include <string.h>
#include <linux/ion.h>

__BEGIN_DECLS

#ifndef ION_IOC_SYNC_PARTIAL
struct ion_fd_partial_data {
    int handle;
    int fd;
    off_t offset;
    size_t len;
};

#define ION_IOC_SYNC_PARTIAL _IOWR(ION_IOC_MAGIC, 9, struct ion_fd_partial_data)
static inline int ion_sync_fd_partial(int fd, int handle_fd, off_t offset, size_t len)
{
        struct ion_fd_partial_data data = { .fd = handle_fd, .offset = offset, .len = len, };
        return ioctl(fd, ION_IOC_SYNC_PARTIAL, &data);
}

#endif /* ION_IOC_SYNC_PARTIAL */

__END_DECLS

#endif /* _LIB_ION_H_ */
