#ifndef __NPUC_TYPE_H__
#define __NPUC_TYPE_H__

#define ST_MAKALU_STR "makalu"
#define ST_2020_STR "2020"
#define ST_NEUS_STR "neus"
#define ST_KITT_STR "kitt"

namespace NPUC
{

/**
 * @brief SOC type (Hardware type)
 * This is SOC types, for example, MAKALU, 2020, NEUS, KITT, etc.
 */
enum SOCType
{
  ST_MAKALU = 0,
  ST_2020 = 1,
  ST_NEUS = 2,
  ST_KITT = 3
};

enum SOCRev
{
  ENUM_EVT0 = 0,
  ENUM_EVT1 = 1
};

/**
 * @brief NN Operation(Operator) type
 * This is a kind of Tensorflow-like operation types.
 */
enum OpType
{
  ADD = 0,
  AVERAGE_POOL_2D = 1,
  CONCATENATION = 2,
  CONV_2D = 3,
  DEPTHWISE_CONV_2D = 4,

  DEPTH_TO_SPACE = 5,
  DEQUANTIZE = 6,
  EMBEDDING_LOOKUP = 7,
  FLOOR = 8,
  FULLY_CONNECTED = 9,

  HASHTABLE_LOOKUP = 10,
  L2_NORMALIZATION = 11,
  L2_POOL_2D = 12,
  LOCAL_RESPONSE_NORMALIZATION = 13,
  LOGISTIC = 14,

  LSH_PROJECTION = 15,
  LSTM = 16,
  MAX_POOL_2D = 17,
  MUL = 18,
  RELU = 19,

  RELU1 = 20,
  RELU6 = 21,
  RESHAPE = 22,
  RESIZE_BILINEAR = 23,
  RNN = 24,

  SOFTMAX = 25,
  SPACE_TO_DEPTH = 26,
  SVDF = 27,
  TANH = 28,
  BATCH_TO_SPACE_ND = 29,

  DIV = 30,
  MEAN = 31,
  CUSTOM = 32,
  SPACE_TO_BATCH_ND = 33,
  SQUEEZE = 34,

  STRIDED_SLICE = 35,
  SUB = 36,
  TRANSPOSE = 37,
  PRELU = 38,
  ELEMENTWISE_MAX = 39,

  ARGMAX = 40,
  SCALE = 41,
  CROP = 42,
  FLATTEN = 43,
  PERMUTE = 44,

  SLICE = 45,
  PRIORBOX = 46,
  POWER = 47,
  PAD = 48,
  DECONV_2D = 49,

  DETECTION = 50,
};

/**
 * @brief NPUC Buffer struct
 * This is used to pass input data (bias, kernel, and ncp binary)
 */
typedef struct __NCPBuffer
{
  unsigned char *addr = 0; /* buffer address */
  unsigned int size = 0;   /* buffer size */
} NCPBuffer;

/**
 * @brief Compiler type (Ondevice or Offline compiler)
 */
enum CompilerType
{
  COMPILER_OFFLINE = 0,
  COMPILER_ONDEVICE = 1
};

/**
 * @brief Dataformat (NHWC or NCHW)
 */
enum DataFormat
{
  NCHW = 0,
  NHWC = 1
};

/**
 * @brief Quantization mode (Symmetric or Asymmetric)
 * @details
 * SYM: symmetric (S08: input/weight/output)
 * ASY: asymmetric (U08: input/weight/output)
 * SYM16: symmetric (S16: input/weight/output)
 * QUASI_SYM: a special asymmetric case that handles it in a symmetric way (S08: input/output, U08:
 * weight)
 * PERF_SYM: a special symmetric case that has with only U08 input to improve performance
 * (U08:input, S08: weight/output)
 */
enum QuantizationMode
{
  SYM = 0,
  ASYM = 1,
  SYM16 = 2,
  QUASI_SYM = 3,
  PERF_SYM = 4,
};

/**
 * @brief Specify the activation to invoke on the result
 * @details
 * NNAPI (Ondevice compiler) passes activation code which is fused in layers (for example, conv or
 * pool)
 */
enum FusedActivation
{
  FUSED_ACT_NONE = 0,
  FUSED_ACT_RELU = 1,
  FUSED_ACT_RELU1 = 2,
  FUSED_ACT_RELU6 = 3,
  FUSED_ACT_TANH = 4,
  FUSED_ACT_LOGISTIC = 5,
};

/**
 * @brief Specify failure/error type
 */
enum
{
  // general
  RET_OK,    // return success
  RET_ERROR, // return error

  INVALID_PARAMS,
  NULLPTR_ERROR,

  // TODO add specific errors and failures
  UNSUPPORTED_OPERATION,
  UNSUPPORTED_FRAMEWORK,

  FAIL_ON_FRONTEND,
  FAIL_ON_NNMODEL,
  FAIL_ON_GRAPHIR,
  FAIL_ON_POLICYMAKER,
  FAIL_ON_CODEGEN,
  FAIL_ON_BACKEND,
};

} // namespace NPUC
#endif // __NPUC_TYPE_H__
