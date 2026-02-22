/**
 * Copyright (C) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed,
 * transmitted, transcribed, stored in a retrieval system or translated into any human or
 * computer language in any form by any means, electronic, mechanical, manual or
 * otherwise or disclosed to third parties without the express written permission of
 * Samsung Electronics.
 */

/**
 * @file eden_memory.h
 * @brief eden_memory api definition
 */
#ifndef OSAL_INCLUDE_EDEN_MEMORY_H_
#define OSAL_INCLUDE_EDEN_MEMORY_H_

#include "osal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ION = 0,
    USER_HEAP,
    MMAP_FD,
    EXTERNAL_ION,
} mem_t;

#if defined(CONFIG_NPU_MEM_ION)
typedef struct _ion_buffer {
    uint32_t fd;
    uint64_t buf;
} ion_buf_t;
#endif

typedef struct _eden_memory {
    mem_t type;
    size_t size;
    size_t alloc_size;
    union {
        void* user_ptr;
#if defined(CONFIG_NPU_MEM_ION)
        ion_buf_t ion;
#endif
    } ref;
} eden_memory_t;

/**
 * @fn eden_mem_init
 * @brief init eden memory managerment
 * @details
 * @param
 * @return error state
 */
osal_ret_t eden_mem_init(void);

/**
 * @fn eden_mem_allocate
 * @brief memory allocation according to the type
 * @details
 * @param eden_memory_t*
 * @return error state
 */
osal_ret_t eden_mem_allocate(eden_memory_t* eden_mem);

/**
 * @fn eden_mem_allocate_with_ion_flag
 * @brief memory allocation according to the type and ion flag value
 * @details
 * @param eden_memory_t*
 * @param uint32_t ion_flag
 * @return error state
 */
osal_ret_t eden_mem_allocate_with_ion_flag(eden_memory_t* eden_mem,
                                           uint32_t ion_flag);

/**
 * @fn eden_mem_free
 * @brief memory free according to the type
 * @details
 * @param
 * @return error state
 */
osal_ret_t eden_mem_free(eden_memory_t* eden_mem);

/**
 * @fn eden_mem_convert
 * @brief convert memory type
 * @details frome from->tyep to to.type
 * @param
 * @return error state
 */
osal_ret_t eden_mem_convert(eden_memory_t* from, eden_memory_t* to);

/**
 * @fn eden_mem_shutdown
 * @brief shutdown eden memory managerment
 * @details
 * @param
 * @return error state
 */
osal_ret_t eden_mem_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif  // OSAL_INCLUDE_EDEN_MEMORY_H_
