/*
 * Copyright (C) 2021 Samsung Electronics Co. LTD
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
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#ifndef DRIVER_COMMON_H_
#define DRIVER_COMMON_H_

#include <cstdint>  // uint32_t
#include <memory>   // shared_ptr
#include <vector>   // vector

namespace android {
namespace nn {
namespace enn_driver {

// enum class RetCode : int32_t {
enum {
    RET_OK,

    GENERAL_FAILURE,
    INVALID_PARAMS,
    INVALID_TARGET_DEVICE,
    INVALID_NUM_OF_SUPPORTED_OPERATIONS,
    SUPPORTED_HIDL_MEMORY_TYPE,
    RET_PARAM_INVALID,
    OUTPUT_FULLY_NOT_SPECIFIED,
    UNSUPPORTED_OPERATION,
    NO_REQUIRED_OPTION_OPERAND,

    FAIL_TO_CONVT_NHWC_TO_NCHW,
    FAIL_TO_CONVT_NCHW_TO_NHWC,

    FAIL_TO_ALLOCATE_INPUT_BUFFERS_ON_EXECUTE,
    FAIL_TO_ALLOCATE_OUTPUT_BUFFERS_ON_EXECUTE,
    FAIL_TO_RESOLVE_UNKNOWN_DIMENSIONS,
    FAIL_TO_WAIT_FOR_SYNC_FENCE,

    FAIL_ON_ENN_INIT,
    FAIL_ON_ENN_OPEN_MODEL,
    FAIL_ON_ENN_OPEN_MODEL_FROM_MEMORY,
    FAIL_ON_ENN_EXECUTE_REQ,
    FAIL_ON_ENN_CLOSE_MODEL,
    FAIL_ON_ENN_SHUTDOWN,
    FAIL_ON_ENN_ALLOCATE_BUFFERS,
    FAIL_ON_ENN_FREE_BUFFERS,
    FAIL_ON_ENN_GET_INPUT_BUFFER_SHAPE,
    FAIL_ON_ENN_GET_OUTPUT_BUFFER_SHAPE,

    FAIL_ON_NPUC_IR_CONVERTER,
    FAIL_ON_NPU_COMPILER,

    GRAPHGEN_STATUS_ERROR,
    MODEL_FULLY_NOT_SUPPORTED,
    FAIL_ON_GENERATE_NNC,

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

typedef enum {
   CPU_GPU,
   NPU_DSP,
   NPU_GPU,
} HwType;

// Array index for dims in EdenShapeInfo
enum {
  N_NCHW = 0,
  C_NCHW = 1,
  H_NCHW = 2,
  W_NCHW = 3,
};

enum {
  N_NHWC = 0,
  H_NHWC = 1,
  W_NHWC = 2,
  C_NHWC = 3,
};

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_COMMON_H_

