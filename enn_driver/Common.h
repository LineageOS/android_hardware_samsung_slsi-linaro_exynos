/*
 * Copyright (C) 2019 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

/**
 * @file    Common.h
 * @brief   This has common data structure, defines etc.
 * @details This has common data structure, defines etc.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_COMMON_H_
#define DRIVER_COMMON_H_

#include <cstdint>  // uint32_t
#include <memory>   // shared_ptr
#include <vector>   // vector

namespace android {
namespace nn {
namespace eden_driver {

// enum class RetCode : int32_t {
enum {
    RET_OK,

    GENERAL_FAILURE,
    INVALID_PARAMS,
    INVALID_TARGET_DEVICE,
    INVALID_NUM_OF_SUPPORTED_OPERATIONS,
    SUPPORTED_HIDL_MEMORY_TYPE,

    UNSUPPORTED_OPERATION,
    NO_REQUIRED_OPTION_OPERAND,
    NO_REQUIRED_OPTION_OPERAND_BUT_THERE_IS,

    CALLBACK_IS_NULL,
    ADDR_IS_NULL,

    CONVERTED_NUM_OF_OPERAND_IS_DIFFERENT,
    FAIL_TO_FIND_A_INPUT_OPERAND_INDEX,
    LIFETIME_IS_NOT_SUBGRAPH_INPUT,
    BUFFER_ON_OPTION_IS_NULL,
    NUM_OF_INDEX_IS_NOT_MATCHED,

    FAIL_TO_STORE_MODEL_TO_CACHE,
    FAIL_TO_GET_SERVICE,
    FAIL_TO_GET_INPUT_MEM,
    FAIL_TO_GET_INPUT_MEM_SHAPE,
    FAIL_TO_GET_OUTPUT_MEM,
    FAIL_TO_GET_OUTPUT_MEM_SHAPE,
    FAIL_TO_FREE_MEM,
    FAIL_TO_FIND_KEY_IN_MAP,

    FAIL_TO_CONVT_NHWC_TO_NCHW,
    FAIL_TO_CONVT_NCHW_TO_NHWC,

    FAIL_TO_ALLOCATE_INPUT_BUFFERS_ON_EXECUTE,
    FAIL_TO_ALLOCATE_OUTPUT_BUFFERS_ON_EXECUTE,
    FAIL_TO_RESOLVE_UNKNOWN_DIMENSIONS,
    FAIL_TO_WAIT_FOR_SYNC_FENCE,

    FAIL_ON_EDEN_INIT,
    FAIL_ON_EDEN_OPEN_MODEL,
    FAIL_ON_EDEN_OPEN_MODEL_FROM_MEMORY,
    FAIL_ON_EDEN_EXECUTE_REQ,
    FAIL_ON_EDEN_CLOSE_MODEL,
    FAIL_ON_EDEN_SHUTDOWN,
    FAIL_ON_EDEN_ALLOCATE_INPUT_BUFFERS,
    FAIL_ON_EDEN_ALLOCATE_OUTPUT_BUFFERS,
    FAIL_ON_EDEN_FREE_BUFFERS,
    FAIL_ON_EDEN_GET_INPUT_BUFFER_SHAPE,
    FAIL_ON_EDEN_GET_OUTPUT_BUFFER_SHAPE,

    FAIL_ON_NPUC_IR_CONVERTER,
    FAIL_ON_NPU_COMPILER,

    GRAPHGEN_STATUS_ERROR,
    FAIL_ON_GENERATE_NNC,

#ifdef ENABLE_DSP
    FAIL_ON_OFI_VERIFY_MODEL,
    FAIL_ON_OFI_EXECUTE_MODEL,
    FAIL_ON_DSP_COMPILER,
#endif
};

enum class DATA_TYPE {
    FLOAT32,
    QUANT8,
    RELAXED_FLOAT32,
    INT32,
    BOOL8,
    FLOAT16,
    INT64,
    INT16,
    INT8,
    UINT64,
    UINT32,
    UINT16,
    UINT8,
};

struct OperationInfo {
    bool supported;
    std::shared_ptr<void> constraint;
};

// Common constraints
struct CommonConstraint {
    bool restrictZeroPoint;
    std::vector<int32_t> supportedZeroPoint;

    bool restrictInputDataType;
    std::vector<int32_t> supportedInputDataType;
};

// Data structure for specifying operation constraint
struct Conv2DConstraint {
    int maxKernelSize;
    int maxStrideSize;
    int maxPaddingSize;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_COMMON_H_

