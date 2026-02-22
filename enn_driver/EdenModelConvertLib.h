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
 * @file    EdenModelConvertLib.h
 * @brief   This is EdenModelConvertLib file.
 * @details This header defines functions to convert Android NN Model to Eden NN Model.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_EDENMODELCONVERTLIB_H_
#define DRIVER_EDENMODELCONVERTLIB_H_

#include <android/hardware_buffer.h>
#include <vndk/hardware_buffer.h>

#include <memory>   // unique_ptr
#include <vector>
#include <set>
#include <map>
#include <cstdint>  // int32_t

#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
#include <ui/GraphicBuffer.h>

#include "Common.h"         // DATA_TYPE

#include "../include/eden_model.h"

using namespace eden::nn;
using namespace android::hardware;
using ::android::hidl::memory::V1_0::IMemory;

namespace android {
namespace nn {
namespace eden_driver {

enum class NN_TARGET_DEVICE {
    NPU = 0,
    GPU = 1,
    CPU = 2,
    DSP = 3,
};

constexpr int32_t NUM_OF_INPUTS_ON_ADD_1_2 = 3;                              // 0
constexpr int32_t NUM_OF_INPUTS_ON_AVERAGE_POOL_2D_EXPLICIT_1_1 = 10;        // 1
constexpr int32_t NUM_OF_INPUTS_ON_AVERAGE_POOL_2D_IMPLICIT_1_1 = 7;
constexpr int32_t NUM_OF_INPUTS_ON_AVERAGE_POOL_2D_EXPLICIT_1_2 = 11;
constexpr int32_t NUM_OF_INPUTS_ON_AVERAGE_POOL_2D_IMPLICIT_1_2 = 8;
// constexpr int32_t NUM_OF_INPUTS_ON_CONCATENATION_1_2;                     // 2
constexpr int32_t NUM_OF_INPUTS_ON_CONV_2D_EXPLICIT_1_1 = 10;                // 3
constexpr int32_t NUM_OF_INPUTS_ON_CONV_2D_IMPLICIT_1_1 = 7;
constexpr int32_t NUM_OF_INPUTS_ON_CONV_2D_EXPLICIT_1_2_ALL = 13;
constexpr int32_t NUM_OF_INPUTS_ON_CONV_2D_IMPLICIT_1_2_ALL = 10;
constexpr int32_t NUM_OF_INPUTS_ON_CONV_2D_EXPLICIT_1_2_LAYOUT = 11;
constexpr int32_t NUM_OF_INPUTS_ON_CONV_2D_IMPLICIT_1_2_LAYOUT = 8;

constexpr int32_t NUM_OF_INPUTS_ON_DEPTHWISE_CONV_2D_EXPLICIT_1_1 = 11;      // 4
constexpr int32_t NUM_OF_INPUTS_ON_DEPTHWISE_CONV_2D_IMPLICIT_1_1 = 8;
constexpr int32_t NUM_OF_INPUTS_ON_DEPTHWISE_CONV_2D_EXPLICIT_1_2 = 14;
constexpr int32_t NUM_OF_INPUTS_ON_DEPTHWISE_CONV_2D_IMPLICIT_1_2 = 11;
constexpr int32_t NUM_OF_INPUTS_ON_DEPTH_TO_SPACE_1_1 = 2;                   // 5
constexpr int32_t NUM_OF_INPUTS_ON_DEPTH_TO_SPACE_1_2 = 3;
// constexpr int32_t NUM_OF_INPUTS_ON_DEQUANTIZE_1_2 = 1;                    // 6
// constexpr int32_t NUM_OF_INPUTS_ON_EMBEDDING_LOOKUP_1_2 = 2;              // 7
// constexpr int32_t NUM_OF_INPUTS_ON_FLOOR_1_2 = 1;                         // 8
constexpr int32_t NUM_OF_INPUTS_ON_FULLY_CONNECTED_1_2 = 4;                  // 9
// constexpr int32_t NUM_OF_INPUTS_ON_HASHTABLE_LOOKUP_1_2 = 3;              // 10
constexpr int32_t NUM_OF_INPUTS_ON_L2_NORMALIZATION_1_1 = 1;                 // 11
constexpr int32_t NUM_OF_INPUTS_ON_L2_NORMALIZATION_1_2 = 2;
constexpr int32_t NUM_OF_INPUTS_ON_L2_POOL_2D_EXPLICIT_1_1 = 10;             // 12
constexpr int32_t NUM_OF_INPUTS_ON_L2_POOL_2D_IMPLICIT_1_1 = 7;
constexpr int32_t NUM_OF_INPUTS_ON_L2_POOL_2D_EXPLICIT_1_2 = 11;
constexpr int32_t NUM_OF_INPUTS_ON_L2_POOL_2D_IMPLICIT_1_2 = 8;
constexpr int32_t NUM_OF_INPUTS_ON_LOCAL_RESPONSE_NORMALIZATION_1_1 = 5;     // 13
constexpr int32_t NUM_OF_INPUTS_ON_LOCAL_RESPONSE_NORMALIZATION_1_2 = 6;
// constexpr int32_t NUM_OF_INPUTS_ON_LOGISTIC_1_2 = 1;                      // 14
constexpr int32_t NUM_OF_INPUTS_ON_LSH_PROJECTION_1_2 = 4;                   // 15
constexpr int32_t NUM_OF_INPUTS_ON_LSTM_1_1 = 23;                            // 16
constexpr int32_t NUM_OF_INPUTS_ON_LSTM_1_2 = 27;
constexpr int32_t NUM_OF_INPUTS_ON_MAX_POOL_2D_EXPLICIT_1_1 = 10;            // 17
constexpr int32_t NUM_OF_INPUTS_ON_MAX_POOL_2D_IMPLICIT_1_1 = 7;
constexpr int32_t NUM_OF_INPUTS_ON_MAX_POOL_2D_EXPLICIT_1_2 = 11;
constexpr int32_t NUM_OF_INPUTS_ON_MAX_POOL_2D_IMPLICIT_1_2 = 8;
constexpr int32_t NUM_OF_INPUTS_ON_MUL_1_2 = 3;                              // 18
// constexpr int32_t NUM_OF_INPUTS_ON_RELU_1_2 = 1;                          // 19
// constexpr int32_t NUM_OF_INPUTS_ON_RELU1_1_2 = 1;                         // 20
// constexpr int32_t NUM_OF_INPUTS_ON_RELU6_1_2 = 1;                         // 21
constexpr int32_t NUM_OF_INPUTS_ON_RESHAPE_1_2 = 2;                          // 22
constexpr int32_t NUM_OF_INPUTS_ON_RESIZE_BILINEAR_1_1 = 3;                  // 23
constexpr int32_t NUM_OF_INPUTS_ON_RESIZE_BILINEAR_1_2 = 4;
// some CTS/VTS 1.3 tests have 5 inputs which should be 6
constexpr int32_t NUM_OF_INPUTS_ON_RESIZE_BILINEAR_1_3_fixcts = 5;
constexpr int32_t NUM_OF_INPUTS_ON_RESIZE_BILINEAR_1_3 = 6;
constexpr int32_t NUM_OF_INPUTS_ON_RNN_1_2 = 6;                              // 24
constexpr int32_t NUM_OF_INPUTS_ON_SOFTMAX_1_1 = 2;                          // 25
constexpr int32_t NUM_OF_INPUTS_ON_SOFTMAX_1_2 = 3;
constexpr int32_t NUM_OF_INPUTS_ON_SPACE_TO_DEPTH_1_1 = 2;                   // 26
constexpr int32_t NUM_OF_INPUTS_ON_SPACE_TO_DEPTH_1_2 = 3;
constexpr int32_t NUM_OF_INPUTS_ON_SVDF_1_2 = 7;                             // 27
// constexpr int32_t NUM_OF_INPUTS_ON_TANH_1_2 = 1;                          // 28
constexpr int32_t NUM_OF_INPUTS_ON_BATCH_TO_SPACE_ND_1_1 = 2;                // 29
constexpr int32_t NUM_OF_INPUTS_ON_BATCH_TO_SPACE_ND_1_2 = 3;
constexpr int32_t NUM_OF_INPUTS_ON_DIV_1_2 = 3;                              // 30
// constexpr int32_t NUM_OF_INPUTS_ON_MEAN_1_2 = 3;                          // 31
// constexpr int32_t NUM_OF_INPUTS_ON_PAD_1_2 = 2;                           // 32
constexpr int32_t NUM_OF_INPUTS_ON_SPACE_TO_BATCH_ND_1_1 = 3;                // 33
constexpr int32_t NUM_OF_INPUTS_ON_SPACE_TO_BATCH_ND_1_2 = 4;
constexpr int32_t NUM_OF_INPUTS_ON_SQUEEZE_1_2 = 2;                          // 34
constexpr int32_t NUM_OF_INPUTS_ON_STRIDED_SLICE_1_2 = 7;                    // 35
constexpr int32_t NUM_OF_INPUTS_ON_SUB_1_2 = 3;                              // 36
// constexpr int32_t NUM_OF_INPUTS_ON_TRANSPOSE_1_2 = 2;                     // 37
constexpr int32_t NUM_OF_INPUTS_ON_ABS_1_2 = 1;                              // 38
constexpr int32_t NUM_OF_INPUTS_ON_CAST_1_2 = 1;                             // 45
constexpr int32_t NUM_OF_INPUTS_ON_CHANNEL_SHUFFLE_1_2 = 3;                  // 46
constexpr int32_t NUM_OF_INPUTS_ON_DETECTION_POSTPROCESSING_1_2 = 14;        // 47
constexpr int32_t NUM_OF_INPUTS_ON_EXP_1_2 = 1;                              // 49
constexpr int32_t NUM_OF_INPUTS_ON_HEATMAP_MAX_KEYPOINT_1_2 = 3;             // 56
constexpr int32_t NUM_OF_INPUTS_ON_INSTANCE_NORMALIZATION_1_2 = 5;           // 57
constexpr int32_t NUM_OF_INPUTS_ON_LOG_1_2 = 1;                              // 60
constexpr int32_t NUM_OF_INPUTS_ON_LOGICAL_AND_1_2 = 2;                      // 61
constexpr int32_t NUM_OF_INPUTS_ON_LOGICAL_OR_1_2 = 2;                       // 63
constexpr int32_t NUM_OF_INPUTS_ON_LOG_SOFTMAX_1_2 = 3;                      // 64
constexpr int32_t NUM_OF_INPUTS_ON_RANDOM_MULTINOMIAL_1_2 = 3;               // 74
constexpr int32_t NUM_OF_INPUTS_ON_QUANTIZE_1_2 = 1;                         // 72

constexpr int32_t NUM_OF_INPUTS_ON_AXIS_ALIGNED_BBOX_TRANSFORM_1_2 = 4;      // 41
constexpr int32_t NUM_OF_INPUTS_ON_BIDIRECTIONAL_SEQUENCE_LSTM_1_2 = 61;     // 42
constexpr int32_t NUM_OF_INPUTS_ON_UNIDIRECTIONAL_SEQUENCE_LSTM_1_2 = 28;    // 53
constexpr int32_t NUM_OF_INPUTS_ON_GENERATE_PROPOSALS_1_2 = 11;              // 52
constexpr int32_t NUM_OF_INPUTS_ON_GROUP_CONV_2D_EXPLICIT_1_2 = 12;          // 55
constexpr int32_t NUM_OF_INPUTS_ON_GROUP_CONV_2D_IMPLICIT_1_2 = 9;           // 55
constexpr int32_t NUM_OF_INPUTS_ON_LOGICAL_NOT_1_2 = 1;                      // 62
constexpr int32_t NUM_OF_INPUTS_ON_ROI_ALIGN_1_2 = 10;                       // 81
constexpr int32_t NUM_OF_INPUTS_ON_RESIZE_NEAREST_NEIGHBOR_1_2 = 4;          // 94

constexpr int32_t NUM_OF_INPUTS_ON_ARGMAX_1_2 = 2;                           // 40
constexpr int32_t NUM_OF_INPUTS_ON_DECONV_2D_EXPLICIT_1_2 = 11;              // 49
constexpr int32_t NUM_OF_INPUTS_ON_DECONV_2D_IMPLICIT_1_2 = 9;               // 49
constexpr int32_t NUM_OF_INPUTS_ON_BIDIRECTIONAl_RNN_1_2 = 15;               // 54
constexpr int32_t NUM_OF_INPUTS_ON_UNIDIRECTIONAL_RNN_1_2 = 7;               // 55
constexpr int32_t NUM_OF_INPUTS_ON_NEG_1_2 = 1;                              // 67
//constexpr int32_t NUM_OF_INPUTS_ON_SIN_1_2 = 1;                            // 75
//constexpr int32_t NUM_OF_INPUTS_ON_RSQRT_1_2 = 1;                          // 76
constexpr int32_t NUM_OF_INPUTS_ON_PAD_V2_1_2 = 3;                           // 77
constexpr int32_t NUM_OF_INPUTS_ON_TOPK_V2_1_2 = 2;                          // 78
constexpr int32_t NUM_OF_INPUTS_ON_TFLITEROIPOOL_1_2 = 8;                    // 79
constexpr int32_t NUM_OF_INPUTS_ON_ARGMIN_1_2 = 2;                           // 80
//constexpr int32_t NUM_OF_INPUTS_ON_SQRT_1_2 = 1;                           // 81
constexpr int32_t NUM_OF_INPUTS_ON_BOX_WITH_NMS_LIMIT_1_2 = 9;               // 82
constexpr int32_t NUM_OF_INPUTS_ON_EQUAL_1_2 = 2;                            // 48
constexpr int32_t NUM_OF_INPUTS_ON_EXPAND_DIMS_1_2 = 2;                      // 50
constexpr int32_t NUM_OF_INPUTS_ON_GATHER_1_2 = 3;                           // 51
constexpr int32_t NUM_OF_INPUTS_ON_GREATER_1_2 = 2;                          // 53
constexpr int32_t NUM_OF_INPUTS_ON_GREATER_EQUAL_1_2 = 2;                    // 54
constexpr int32_t NUM_OF_INPUTS_ON_LESS_1_2 = 2;                             // 58
constexpr int32_t NUM_OF_INPUTS_ON_LESS_EQUAL_1_2 = 2;                       // 59
constexpr int32_t NUM_OF_INPUTS_ON_NOT_EQUAL_1_2 = 2;                        // 69
constexpr int32_t NUM_OF_INPUTS_ON_POW_1_2 = 2;                              // 70
constexpr int32_t NUM_OF_INPUTS_ON_PRELU_1_2 = 2;                            // 71
constexpr int32_t NUM_OF_INPUTS_ON_QUANTIZED_16BIT_LSTM_1_2 = 15;            // 73
constexpr int32_t NUM_OF_INPUTS_ON_SELECT_1_2 = 3;                           // 84
constexpr int32_t NUM_OF_INPUTS_ON_SPLIT_1_2 = 3;

enum EDEN_OPERAND_NAME_IDX {
    IFM = 0,
    TENSOR,
    FILTER,
    BIAS,

    // SVDF
    WEIGHTS_FEATURE,
    WEIGHTS_TIME,
    STATE,

    // LSH_PROJECTION
    HASH,
    WEIGHT,

    // BATCH_TO_SPACE_ND
    BLOCKSHAPE,

    // PAD
    PADDINGS,

    // STRIDED_SLICE
    BEGIN,
    END,
    STRIDES,

    // SPACE_TO_BATCH_ND
    BLOCK,
    PAD,
    AXIS,
    KEEP_DIMS,

    // SQUEEZE
    SQUEEZE_DIMS,

    // HASHTABLE_LOOKUP, EMBEDDING_LOOKUP
    LOOKUPS,
    KEYS,
    VALUES,
    HITS,

    // RNN
    WEIGHTS,
    RECURRENT_WEIGHTS,
    HIDDEN_STATE,

    // LSTM
    INPUT_TO_INPUT_WEIGTHS,
    INPUT_TO_FORGET_WEIGTHS,
    INPUT_TO_CELL_WEIGTHS,
    INPUT_TO_OUTPUT_WEIGTHS,

    RECURRENT_TO_INPUT_WEIGTHS,
    RECURRENT_TO_FORGET_WEIGTHS,
    RECURRENT_TO_CELL_WEIGTHS,
    RECURRENT_TO_OUTPUT_WEIGTHS,

    CELL_TO_INPUT_WEIGTHS,
    CELL_TO_FORGET_WEIGTHS,
    CELL_TO_OUTPUT_WEIGTHS,

    INPUT_GATE_BIAS,
    FORGET_GATE_BIAS,
    CELL_GATE_BIAS,
    OUTPUT_GATE_BIAS,

    PROJECTION_WEIGHTS,
    PROJECTION_BIAS,

    INPUT_ACTIVATION_STATE,
    INPUT_CELL_STATE,

    SCRATCH_BUFFER,
    OUTPUT_STATE,
    CELL_STATE,

    INPUT_LAYER_NORM_WEIGHTS,
    FORGET_LAYER_NORM_WEIGHTS,
    CELL_LAYER_NORM_WEIGHTS,
    OUTPUT_LAYER_NORM_WEIGHTS,

    // BIDIRECTIONAL_SEQUENCE_LSTM
    FW_INPUT_TO_INPUT_WEIGHTS,
    FW_INPUT_TO_FORGET_WEIGHTS,
    FW_INPUT_TO_CELL_WEIGHTS,
    FW_INPUT_TO_OUT_WEIGHTS,

    FW_RECURRENT_TO_INPUT_WEIGHTS,
    FW_RECURRENT_TO_FORGET_WEIGHTS,
    FW_RECURRENT_TO_CELL_WEIGHTS,
    FW_RECURRENT_TO_OUTPUT_WEIGHTS,

    FW_CELL_TO_INPUT_WEIGHTS,
    FW_CELL_TO_FORGET_WEIGHTS,
    FW_CELL_TO_OUT_WEIGHTS,

    FW_INPUT_GATE_BIAS,
    FW_FORGET_GATE_BIAS,
    FW_CELL_GATE_BIAS,
    FW_OUTPUT_GATE_BIAS,
    FW_PROJECTION_WEIGHTS,
    FW_PROJECTION_BIAS,

    BW_INPUT_TO_INPUT_WEIGHTS,
    BW_INPUT_TO_FORGET_WEIGHTS,
    BW_INPUT_TO_CELL_WEIGHTS,
    BW_INPUT_TO_OUTPUT_WEIGHTS,

    BW_RECURRENT_TO_INPUT_WEIGHTS,
    BW_RECURRENT_TO_FORGET_WEIGHTS,
    BW_RECURRENT_TO_CELL_WEIGHTS,
    BW_RECURRENT_TO_OUTPUT_WEIGHTS,

    BW_CELL_TO_INPUT_WEIGHTS,
    BW_CELL_TO_FORGET_WEIGHTS,
    BW_CELL_TO_OUTPUT_WEIGHTS,

    BW_INPUT_GATE_BIAS,
    BW_GORGET_GATE_BIAS,
    BW_CELL_GATE_BIAS,
    BW_OUTPUT_GATE_BIAS,
    BW_PROJECTION_WEIGHTS,
    BW_PROJECTION_BIAS,

    FW_INPUT_ACTIVATION_STATE,
    FW_INPUT_CELL_STATE,
    BW_INPUT_ACTIVATION_STATE,
    BW_INPUT_CELL_STATE,

    FW_AUX_INPUT_TO_INPUT_WEIGHTS,
    FW_AUX_INPUT_TO_FORGET_WEIGHTS,
    FW_AUX_INPUT_TO_CELL_WEIGHTS,
    FW_AUX_INPUT_TO_OUTPUT_WEIGHTS,

    BW_AUX_INPUT_TO_INPUT_WEIGHTS,
    BW_AUX_INPUT_TO_FORGET_WEIGHTS,
    BW_AUX_INPUT_TO_CELL_WEIGHTS,
    BW_AUX_INPUT_TO_OUTPUT_WEIGHTS,

    FW_INPUT_LAYER_NORM_WEIGHTS,
    FW_FORGET_LAYER_NORM_WEIGHTS,
    FW_CELL_LAYER_NORM_WEIGHTS,
    FW_OUTPUT_LAYER_NORM_WEIGHTS,

    BW_INPUT_LAYER_NORM_WEIGHTS,
    BW_FORGET_LAYER_NORM_WEIGHTS,
    BW_CELL_LAYER_NORM_WEIGHTS,
    BW_OUTPUT_LAYER_NORM_WEIGHTS,

  // UNIDIRECTIONAL_SEQUENCE_LSTM
    LSTM_INTOIN,
    LSTM_INTOFORGET,
    LSTM_INTOCELL,
    LSTM_INTOOUT,
    LSTM_RETOIN,
    LSTM_RETOFORGET,
    LSTM_RETOCELL,
    LSTM_RETOOUT,
    LSTM_CELLTOIN,
    LSTM_CELLTOFORGET,
    LSTM_CELLTOOUT,
    LSTM_INGATE,
    LSTM_FORGETGATE,
    LSTM_CELLGATE,
    LSTM_OUTGATE,
    LSTM_PROJWEIGHT,
    LSTM_PROJBIAS,
    LSTM_INACT,
    LSTM_INCELL,
    LSTM_INPUT_LAYER_NORM_WEIGHTS,
    LSTM_FORGET_LAYER_NORM_WEIGHTS,
    LSTM_CELL_LAYER_NORM_WEIGHTS,
    LSTM_OUTPUT_LAYER_NORM_WEIGHTS,

    // PAD_V2
    PADDING_SHAPE,
    PADDING_VALUE,

    ANCHORS,

    PRELU,

    // GATHER
    INDICES,
    GATHER,

    SPLIT,

    EXPAND_DIMS,

    SIZE,
    MULTIPLIERS,

    // BIDIRECTIONAL_SEQUENCE_RNN
    FW_WEIGHTS,
    FW_RECURRENT_WEIGHTS,
    FW_BIAS,
    FW_HIDDEN_STATE,

    BW_WEIGHTS,
    BW_RECURRENT_WEIGHTS,
    BW_BIAS,
    BW_HIDDEN_STATE,

    FW_AUX_WEIGHTS,
    BW_AUX_WEIGHTS,
};

enum Pad_axis{
    HORIZONTAL,
    VERTICAL,
};

typedef struct __NNSubOperationList {
    std::vector<int32_t> operationList;
    std::set<int32_t> inputOperandIndexes;
    std::set<int32_t> outputOperandIndexes;
    NN_TARGET_DEVICE targetDevice;
} NNSubOperationList;

enum hidl_mem_type {
    ASHMEM,
    MMAP_FD_1,
    MMAP_FD_2,
    AHWB_BLOB,
};
typedef struct __VirtualAddressInfo {
    hidl_mem_type type;  // 0:(ashmem), 1,2:(mmap_fd), 3: (hardware_buffer_blob)
    char* addr;
    int32_t size;
    AHardwareBuffer* hwBuffer;
} VirtualAddressInfo;

typedef struct __ModelInfo {
    std::vector<VirtualAddressInfo> vecVirtualAddressOnPools;
    std::vector<sp<IMemory>> vecIMemoryOnAshmems;

    std::unique_ptr<uint8_t[]> operandValues;
    std::vector<NNSubOperationList> vecOpGroup;
    std::vector<int32_t> modelInputIndexes;
    std::vector<int32_t> modelOutputIndexes;
    std::map<int32_t, int32_t> mapOperandIdFromAToE;
    std::map<int32_t, int32_t> mapOperationIdFromAToE;
    std::vector<int32_t> inputConsumers;
    std::map<void*, int32_t> mapOperationToOperationId;
    std::vector<int32_t> vecCustomOperationIds;

    std::vector<std::unique_ptr<int32_t[]>> internalBuffers;

    // Used for converting weight/bias from NHWC to NCHW
    std::vector<bool> vecConverted;
    std::vector<bool> vecIsNCHW;
} ModelInfo;

typedef struct __Conv2DParams {
    int32_t kernelSize;
    int32_t paddingSize;
    int32_t strideSize;
    uint32_t zeroPoint;
} Conv2DParams;

typedef struct __Pool2DParams {
    int32_t kernelSize;
    int32_t paddingSize;
    int32_t strideSize;
    uint32_t zeroPoint;
} Pool2DParams;

typedef struct __ConcatParams {
    int32_t axis;
} ConcatParams;

typedef struct __Pool2DInputParams {
    int32_t paddingLeft, paddingRight, paddingTop, paddingBottom;
    int32_t paddingImplicit;
    int32_t strideWidth, strideHeight;
    int32_t filterWidth, filterHeight;
    int32_t activation;
    bool NCHWDataLayout;
}Pool2DInputParams;

HwPreference getHwPreference(const ModelInfo& modelInfo);
const char* getAndroidNNOperandTypeName(V1_3::OperandType type);
void copyName(const char* targetName, char*& name, int32_t& length);
void releaseName(int8_t*& name);
void getMatchedEdenOperandName(int32_t nameIndex, int8_t*& name, int32_t& length);
void getMatchedEdenOperationName(int32_t nameIndex, int8_t*& name, int32_t& length);
void getEdenCustomOperationName(int32_t nameIndex, int8_t*& name, int32_t& length);
void getEdenCustomOperationNameForNCP(int8_t*& name, int32_t& length);
void getOptionsName(uint32_t type, char*& name, int32_t& length);
void getModelNameForNCP(int8_t*& name, int32_t& length);

void getEdenDataType(V1_3::OperandType androidDataType, EdenDataType& type, EdenDataTypeSize& size);
DATA_TYPE getDataType(V1_3::OperandType androidDataType);
DATA_TYPE getDataTypeFromEden(EdenDataType edenDataType);

void getEdenDimensions(const hidl_vec<uint32_t>& androidDimensions, int32_t* edenDimensions, int32_t& numOfDims);
void reduceDimensions(const V1_3::Operation& androidOperation, const V1_3::Model& model, const ModelInfo* modelInfo,
                      const int32_t& androidOperandIdx, const int32_t& edenInputOperandId, EdenModel* edenModel);
int32_t getActivationFn(int32_t type);
int32_t getMatchedEdenOperationType(V1_3::OperationType androidOpType);

EdenOperand* getEdenOperand(EdenModel* edenModel, int32_t edenOperandIdx);
EdenOperation* getEdenOperation(EdenModel* edenModel, int32_t edenOperationIdx);

template <typename T> T getValue(const V1_3::Model& model, const V1_3::Operation& androidOperation, const int op_id,
                                const ModelInfo* modelInfo = nullptr);
template <typename T> T getPtr(const V1_3::Model& model, const V1_3::Operation& androidOperation, const int op_id,
                               sp<IMemory>& buffer_addr, const ModelInfo* modelInfo = nullptr);

int32_t prepareModelInfo(const V1_3::Model& model, ModelInfo& modelInfo);
int32_t getVirtualAddressOnPools(const V1_3::Model& model, std::vector<VirtualAddressInfo>& vecVirtualAddressOnPools,
                                 std::vector<sp<IMemory>>& vecIMemoryOnAshmems);
int32_t getVirtualAddressOnPool(const hidl_memory& memory, bool needToWrite, VirtualAddressInfo& vInfo,
                                sp<IMemory>& spIMemoryOnAshmem);

int32_t createEmptyEdenModel(EdenModel *&edenModel,
                             EdenOperation *&edenOperations,
                             int32_t numOfEdenOperations);
int32_t createEdenOperand(ModelInfo& modelInfo, V1_3::Operand androidOperand, EdenOperand*& edenOperand, int32_t edenOpType, bool isNCHW);
int32_t createEdenOperation(const V1_3::Model& model, ModelInfo& modelInfo, const V1_3::Operation& androidOperation, EdenOperation*& edenOperation,
        EdenModel* edenModel);
void createEmptyEdenOperand(EdenOperand** edenOperand);
void createEdenCustomOperandForNcpBinary(void* bufferForNCP, int32_t bufferSizeForNCP, EdenOperand** edenOperand);
void createEdenCustomOperandForNcpName(EdenOperand** edenOperand);

int32_t getEdenOperandIdx(const V1_3::Model& model, ModelInfo& modelInfo, EdenModel* edenModel, int32_t androidOperandIdx,
                          int32_t edenOpType, bool isNCHW);

int32_t keepOperandIdMap(ModelInfo& modelInfo, int32_t androidOperandIdx, int32_t edenOperandIdx);
int32_t keepOperationIdMap(ModelInfo& modelInfo, int32_t androidOperationIdx, int32_t edenOperationIdx);
int32_t keepOperationToOperationIdMap(ModelInfo& modelInfo, void* androidOperation, int32_t androidOperationIdx);
int32_t keepInputConsumers(const V1_3::Model& model, ModelInfo& modelInfo, int32_t androidOperandIdx, int32_t androidOperationIdx);
int32_t keepCustomOperationId(ModelInfo& modelInfo, int32_t edenOperationIdx);

bool createEdenOperandForConfig(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configAdd(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configAveragePool2d(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configConcatenation(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configConv2d(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configFullyConnected(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configDepthwiseConv2d(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configDepthToSpace(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand**  configOperand);
void configL2Normalization(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configL2Pool2d(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configLocalResponseNormalization(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configMaxpool2d(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configMul(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configSoftmax(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configSpaceToDepth(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configStridedSlice(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configLshProjection(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configReshape(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configResizeBilinear(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configLSTM(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configPad(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configTranspose(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configRNN(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configSVDF(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configDiv(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configSub(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configBatchToSpaceND(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configSpaceToBatchND(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configGenerateProposals(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configResizeNearestNeighbor(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configRoiAlign(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configBidirectionalSequenceLstm(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configUnidirectionalSequenceLstm(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configGroupConv2d(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configArgMax(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configDeConv2d(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configBidriectionalRNN(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configUnidirectionalRNN(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configTopKV2(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configTFLiteRoiPool(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configArgMin(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configBoxWithNmsLimit(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configGather(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configSplit(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configExpandDims(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configChannelShuffle(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configDetectionPostprocessing(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configInstanceNormalization(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configRandomMultinomial(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configLogSoftmax(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void configHeatmapMaxKeypoint(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, EdenOperand** configOperand);
void Initialize_EdenOperand(const V1_3::Model& model, const V1_3::Operation& androidOperation, EdenOperand* newOperandForOptions, int32_t operationSize, size_t numOfInputs);
bool Is_unkownDims_unknownRank(const V1_3::Model& model, const V1_3::Operation& androidOperation);

void readConv2DParams(const V1_3::Model& model, const V1_3::Operation& androidOperation, Conv2DParams& conv2DParams);
void showConv2DParams(const Conv2DParams& conv2DParams);

void readPool2DParams(const V1_3::Model& model, const V1_3::Operation& androidOperation, Pool2DParams& pool2DParams);
void showPool2DParams(const Pool2DParams& pool2DParams);

void readConcatParams(const V1_3::Model& model, const V1_3::Operation& androidOperation, ConcatParams& concatParams);
void showConcatParams(const ConcatParams& concatParams);

int32_t readOperandType(const V1_3::Model& model, const V1_3::Operation& androidOperation, int32_t inputIndex);

void getPadding(int32_t inputSize, int32_t stride, int32_t filterSize, int32_t paddingImplicit, int32_t* paddingHead, int32_t* paddingTail, bool isTransposeConv = false);

void getPoolInputs(const V1_3::Model& model, const V1_3::Operation& androidOperation, const ModelInfo* modelInfo, size_t numOfInputs, Pool2DInputParams *Pool2dInputs);
void getPaddingForPool(int32_t inputSize, Pool2DInputParams * Pool2dInputs, Pad_axis axis_type, bool isTransposeConv = false);


int32_t loadParamsForEdenOperation(const int32_t edenOpType, const bool hasOptions, const V1_3::Operation& androidOperation,
                                   int32_t& numOfInputTensors, int32_t& numOfInputs, int32_t& numOfOutputs, std::vector<bool>& needToConvertOperands);
int32_t getInputOutputForEdenOperation(const V1_3::Model& model, ModelInfo& modelInfo, const V1_3::Operation& androidOperation, int32_t configOperandId,
        EdenOperation* edenOperation, EdenModel* edenModel);

void setEdenOperandNamesForNull(EdenModel* edenModel);
void setEdenOperandNamesForInput(EdenModel* edenModel, EdenOperation* edenOperation);
void setEdenOperandNamesForOutput(EdenModel* edenModel, EdenOperation* edenOperation);
void setEdenOperandNames(EdenModel* edenModel, EdenOperation* edenOperation);
void setEdenOperandNamesOnInputs(EdenModel* edenModel, EdenOperation* edenOperation, std::vector<int32_t>& vecNameIndexes, bool bForceUpdate);
int32_t updateEdenOperandNames(EdenModel* edenModel, int32_t* operandIndexes, int32_t numOfIndexes, std::vector<int32_t>& vecNameIndexes, bool bForceUpdate);

void showDimensions(std::vector<int32_t>& inputIndexes, EdenModel* edenModel);

bool loadDefaultValuesForOptionalInputs(const V1_3::Model& /*model*/, int32_t indexNumber, const V1_3::Operation& androidOperation, char* dstAddr, int32_t length);
void loadDefaultValuesForOptionalInputsForLshProjection(int32_t indexNumber, const V1_3::Operation& androidOperation, void* dstAddr, int32_t length);

bool verifyActivation(const V1_3::Model& model);
bool verifyDimension(const V1_3::Request& request, const V1_3::Model& model);
void showRequest(const V1_3::Request& request);
inline bool isValidAxis(const int32_t axis, const int32_t rank) {
    return (axis <= rank - 1 && axis + rank >= 0);
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_EDENMODELCONVERTLIB_H_
