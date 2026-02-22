/*
 * Copyright (C) 2018 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */

#ifndef SOURCE_DSP_NN_INCLUDE_OFI_MEMORY_MANAGER_H_
#define SOURCE_DSP_NN_INCLUDE_OFI_MEMORY_MANAGER_H_

// System includes
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sys/mman.h>
#include "../memory_manager/ofi_mm_memory.h"

#ifdef OFI_USE_HIDL_INTERFACE
#include "ofi_hidl_header.h"
using ::vendor::samsung_slsi::hardware::ofi::OFI_NAMESPACE_VER::rt_hidl_mem_t;
#endif

#ifdef OFI_USE_GRAPH_PARSER
#ifdef OFI_USE_HIDL_INTERFACE
#include "ofi_graph_renamer.h"
#else
#include "ofi_defined_struct.h"
#endif
#endif

using namespace std;

typedef unordered_map<uint64_t, OfiMemory *> MemoryMap;
typedef vector<OfiMemory *> PageList;

class OfiMemoryManager {
public:
  OfiMemoryManager();
  ~OfiMemoryManager();

  /*
   * public memory api
   */
  MemItem_t *createMemItem(int32_t size, uint32_t type, uint32_t flag);
  int32_t deleteMemItem(MemItem_t *item);

#ifndef BUILD_X86
  MemItem_t *addMemItem(int32_t size, int32_t shared_fd, int32_t offset,
                        void *predefined_va, int32_t memtype,
                        native_handle_t *ntv_hdl = nullptr);
  int32_t removeMemItem(MemItem_t *item);
  int32_t getSharedFd(MemItem_t *item);
#else
  MemItem_t *addMemItem(void *predefined_va, int32_t size, int32_t memtype);
  int32_t removeMemItem(MemItem_t *item);
#endif
  int32_t getMemoryFlags(MemItem_t *item);

  int32_t getMemoryOffset(MemItem_t *item);
#ifdef OFI_USE_HIDL_INTERFACE
  native_handle_t *getNativeHandle(MemItem_t *item);
#endif

  int32_t getMemType(MemItem_t *item);
#ifndef OFI_USE_GRAPH_PARSER
  MemItem_t *getMemItem(int32_t shared_fd);
#endif

  int32_t checkMemItemAlloced(MemItem_t *memItem) {
    if (memItem == nullptr)
      return 0;
    return (memItem->MAGIC == MITEM_MAGIC_ALLOCED ||
            memItem->MAGIC == MITEM_MAGIC_EXTMAKE);
  }

  void *Mmap(uint32_t fd, uint32_t size);
  void Munmap(void *va, uint32_t size);
  void *GetAddressWithOffset(void *va, uint32_t offset);

  void showAllMemImte();

private:
  OfiMemory *createMemory(int32_t size, int32_t type, int32_t flag);
  int32_t deleteMemory(MemItem_t *item);
  uint32_t getMemoryPoolSize() const {
    return static_cast<uint32_t>(mMemoryPool.size());
  }

  MemoryMap mMemoryPool;
  int32_t mIonClientFd;
  int32_t mHeapmask;
  mutex mMemory_mutex;
};

#endif // SOURCE_DSP_NN_INCLUDE_OFI_MEMORY_MANAGER_H_
