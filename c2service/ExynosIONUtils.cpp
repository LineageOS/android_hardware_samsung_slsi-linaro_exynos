/*
 *
 * Copyright 2018 Samsung Electronics S.LSI Co. LTD
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
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <algorithm>

#include "ExynosIONUtils.h"

#include "hardware/exynos/ion.h"

#define ION_EXYNOS_HEAP_NAME_SYSTEM "ion_system_heap"
#define ION_EXYNOS_HEAP_NAME_VIDEO_STREAM "vstream_heap"  /* ion */
#define DMA_EXYNOS_HEAP_NAME_VIDEO_STREAM_SECURE "system-secure-vstream-secure"  /* dmaheap */
#define DMA_EXYNOS_HEAP_NAME_SYSTEM "system"
#define DMA_EXYNOS_HEAP_NAME_SYSTEM_UNCACHED "system-uncached"

static uint32_t getHeapMask(const char *heapName) {
    if (heapName == nullptr) {
        return 0;
    }

    return exynos_ion_get_heap_mask(heapName);
}

C2R ExynosIONUtils::setIonUsage(C2InterfaceHelper::C2P<C2StoreIonUsageInfo> &me) {
    me.set().heapMask     = getHeapMask(ION_EXYNOS_HEAP_NAME_SYSTEM);
    me.set().allocFlags   = 0;
    me.set().minAlignment = 0;

    return C2R::Ok();
}

C2R ExynosIONUtils::setDmaUsage(C2InterfaceHelper::C2P<C2StoreDmaBufUsageInfo> &me) {
    const char *heapName = nullptr;
    int32_t allocFlags = 0;

    if (me.v.m.usage & (C2MemoryUsage::CPU_READ | C2MemoryUsage::CPU_WRITE)) {
        heapName    = DMA_EXYNOS_HEAP_NAME_SYSTEM;
        allocFlags  = O_RDWR;
    } else {
        heapName    = DMA_EXYNOS_HEAP_NAME_SYSTEM_UNCACHED;
        allocFlags  = O_RDWR;
    }

    strncpy(me.set().m.heapName, heapName, std::min(me.v.flexCount(), strlen(heapName)));
    me.set().m.allocFlags = allocFlags;

    return C2R::Ok();
}

uint32_t ExynosIONUtils::getDmaUsageMask() {
    uint32_t mask = O_RDONLY |
                    O_WRONLY |
                    O_RDWR;
    return mask;
}

