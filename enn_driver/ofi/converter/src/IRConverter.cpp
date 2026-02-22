/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "IRConverter"

#include "IRConverter.h"
#include <android-base/logging.h>
#include <android/log.h>
#include <log/log.h>
#include <fstream>
#include <thread>
#include <exception>
// #include "ValidateHal.h"

class DbgException : public std::exception {
  public:
    DbgException(std::string msg) : mMsg(msg) {}
    const char *what() const noexcept override { return mMsg.c_str(); }
    std::string mMsg;
};
#define DISABLE_ALL_QUANT
#define NN_DEBUG

enum DebugLevel {
    L0,
    L1,
    L2,
    L3,
    L4,
};

unsigned int debugMask = ((1 << (L1 + 1)) - 1);

#ifdef NN_DEBUG
#define VLOG(l, x, ...)                                                                                                \
    do {                                                                                                               \
        if (debugMask & (1 << l)) {                                                                                    \
            ALOGI("[%s] " x, __FUNCTION__, ##__VA_ARGS__);                                                             \
        }                                                                                                              \
    } while (0)

#define VLOGDIMS(l, d, header)                                                                                         \
    do {                                                                                                               \
        auto size = (d).size();                                                                                        \
        VLOG(l,                                                                                                        \
             "%s: vectors {%d, %d, %d, %d}",                                                                           \
             header,                                                                                                   \
             (int)((d)[0]),                                                                                            \
             size > 1 ? (int)((d)[1]) : 0,                                                                             \
             size > 2 ? (int)((d)[2]) : 0,                                                                             \
             size > 3 ? (int)((d)[3]) : 0);                                                                            \
    } while (0)

#define dumpOperand(index)                                                                                             \
    do {                                                                                                               \
        const auto op = mModel.main.operands[index];                                                                        \
        VLOG(L3, "---------------------------------------------");                                                     \
        VLOG(L3, "Operand index: %d", index);                                                                          \
        VLOG(L3, "%s", toString(op).c_str());                                                                          \
        VLOG(L3, "---------------------------------------------");                                                     \
    } while (0)

#define dumpOperation(operation)                                                                                       \
    do {                                                                                                               \
        VLOG(L3, "---------------------------------------------");                                                     \
        VLOG(L3, "Operation:");                                                                                        \
        VLOG(L3, "%s", toString(operation).c_str());                                                                   \
        VLOG(L3, "---------------------------------------------");                                                     \
    } while (0)

#define dumpOperationSupport(operation, support)                                                                       \
    do {                                                                                                               \
        VLOG(L3, "---------------------------------------------");                                                     \
        VLOG(L3, "Operation support: %s", support ? "True" : "False");                                                 \
        VLOG(L3, "%s", toString(operation).c_str());                                                                   \
        VLOG(L3, "---------------------------------------------");                                                     \
    } while (0)

#else
#define VLOG(...)
#define VLOGDIMS(l, d, header)
#define dumpOperand(...)
#define dumpOperation(operation)
#define dumpOperationSupport(operation, support)
#endif

#define WRONG_DIM (-1)

#define nnAssert(v)                                                                                                    \
    do {                                                                                                               \
        if (!(v)) {                                                                                                    \
            LOG(ERROR) << "nnAssert failed at " << __FILE__ << ":" << __LINE__ << " - '" << #v << "'\n";               \
            mErrorMessage = "nnAssert occurred:" + std::to_string((int)(__LINE__));                                    \
            throw DbgException(mErrorMessage);                                                                         \
        }                                                                                                              \
    } while (0)

#define nnAssertWM(v, msg)                                                                                             \
    do {                                                                                                               \
        if (!(v)) {                                                                                                    \
            LOG(ERROR) << "nnAssert failed at " << __FILE__ << ":" << __LINE__ << " - '" << #v << msg << "'\n";        \
            mErrorMessage = "nnAssert occurred:" + std::to_string((int)(__LINE__)) + " - " + msg;                      \
            throw DbgException(mErrorMessage);                                                                         \
        }                                                                                                              \
    } while (0)

namespace android {
namespace nn {
namespace eden_driver {
namespace ofi {

// using namespace android::nn;

enum PaddingScheme {
    kPaddingUnknown = 0,
    /**
     * SAME padding.
     * Padding on both ends are the "same":
     *     padding_to_beginning =  total_padding / 2
     *     padding_to_end       = (total_padding + 1)/2.
     * i.e., for even number of padding, padding to both ends are exactly
     * the same; for odd number of padding, padding to the ending is bigger
     * than the padding to the beginning by 1.
     *
     * total_padding is a function of input, stride and filter size.
     * It could be computed as follows:
     *    out_size = (input + stride - 1) / stride;
     *    needed_input = (out_size - 1) * stride + filter_size
     *    total_padding = max(0, needed_input - output_size)
     *  The computation is the same for the horizontal and vertical directions.
     */
    kPaddingSame = 1,
    /**
     * VALID padding.
     * No padding. When the input size is not evenly divisible by
     * the filter size, the input at the end that could not fill
     * the whole filter tile will simply be ignored.
     */
    kPaddingValid = 2,
};

void calculateExplicitPadding(int32_t in_size,
                              int32_t stride,
                              int32_t filter_size,
                              int32_t padding_implicit,
                              int32_t *padding_head,
                              int32_t *padding_tail) {
    *padding_head = 0;
    *padding_tail = 0;

    if (padding_implicit == kPaddingSame) {
        int32_t out_size = (in_size + stride - 1) / stride;
        int32_t tmp = (out_size - 1) * stride + filter_size;
        if (tmp > in_size) {
            *padding_head = (tmp - in_size) / 2;
            *padding_tail = (tmp - in_size) - *padding_head;
        }
    }
}

int32_t
computeOutSize(int32_t imageSize, int32_t filterSize, int32_t stride, int32_t paddingHead, int32_t paddingTail) {
    return (imageSize - filterSize + stride + paddingHead + paddingTail) / stride;
}

static inline size_t sizeOf(const TensorDims &dims) {
    size_t ret = dims[0];
    for (int i = 1; i < dims.size(); ++i)
        ret *= dims[i];
    return ret;
}

// shape is nchw, dims depends on layout
TensorDims dimsToShape(const std::vector<uint32_t> &dims, Layout layout) {
    VLOG(L3, "layout: %d", static_cast<int>(layout));
    VLOGDIMS(L3, dims, "dims");
    TensorDims shape;
    uint32_t n, c, h, w;
    // 4-D
    switch (layout) {
    case NCHW:
    case OIHW:
        n = dims[0];
        c = dims[1];
        h = dims[2];
        w = dims[3];
        shape = {n, c, h, w};
        break;
    case NHWC:
        n = dims[0];
        h = dims[1];
        w = dims[2];
        c = dims[3];
        shape = {n, c, h, w};
        break;
#if 0
        case IHWO:
            n = dims[3];
            c = dims[0];
            h = dims[1];
            w = dims[2];
            shape = {n, c, h, w};
            break;
#endif
    case C:
        n = dims[0];
        shape = {n};
        break;
    case NC:
        n = dims[0];
        c = dims[1];
        shape = {n, c};
        break;
    default: VLOG(L1, "unsupported layout %d", layout);
    }

    VLOGDIMS(L3, shape, "shape");
    return shape;
}

unsigned short float2half(unsigned f) {
    unsigned f_exp, f_sig;
    unsigned short h_sgn, h_exp, h_sig;

    h_sgn = (unsigned short)((f & 0x80000000u) >> 16);
    f_exp = (f & 0x7f800000u);

    /* Exponent overflow/NaN converts to signed inf/NaN */
    if (f_exp >= 0x47800000u) {
        if (f_exp == 0x7f800000u) {
            /* Inf or NaN */
            f_sig = (f & 0x007fffffu);
            if (f_sig != 0) {
                /* NaN - propagate the flag in the significand... */
                unsigned short ret = (unsigned short)(0x7c00u + (f_sig >> 13));
                /* ...but make sure it stays a NaN */
                if (ret == 0x7c00u) {
                    ret++;
                }
                return h_sgn + ret;
            } else {
                /* signed inf */
                return (unsigned short)(h_sgn + 0x7c00u);
            }
        } else {
            /* overflow to signed inf */
#if NPY_HALF_GENERATE_OVERFLOW
            npy_set_floatstatus_overflow();
#endif
            return (unsigned short)(h_sgn + 0x7c00u);
        }
    }

    /* Exponent underflow converts to a subnormal half or signed zero */
    if (f_exp <= 0x38000000u) {
        /*
         * Signed zeros, subnormal floats, and floats with small
         * exponents all convert to signed zero halfs.
         */
        if (f_exp < 0x33000000u) {
#if NPY_HALF_GENERATE_UNDERFLOW
            /* If f != 0, it underflowed to 0 */
            if ((f & 0x7fffffff) != 0) {
                npy_set_floatstatus_underflow();
            }
#endif
            return h_sgn;
        }
        /* Make the subnormal significand */
        f_exp >>= 23;
        f_sig = (0x00800000u + (f & 0x007fffffu));
#if NPY_HALF_GENERATE_UNDERFLOW
        /* If it's not exactly represented, it underflowed */
        if ((f_sig & (((unsigned)1 << (126 - f_exp)) - 1)) != 0) {
            npy_set_floatstatus_underflow();
        }
#endif
        f_sig >>= (113 - f_exp);
        /* Handle rounding by adding 1 to the bit beyond half precision */
#if NPY_HALF_ROUND_TIES_TO_EVEN
        /*
         * If the last bit in the half significand is 0 (already even), and
         * the remaining bit pattern is 1000...0, then we do not add one
         * to the bit after the half significand.  In all other cases, we do.
         */
        if ((f_sig & 0x00003fffu) != 0x00001000u) {
            f_sig += 0x00001000u;
        }
#else
        f_sig += 0x00001000u;
#endif
        h_sig = (unsigned short)(f_sig >> 13);
        /*
         * If the rounding causes a bit to spill into h_exp, it will
         * increment h_exp from zero to one and h_sig will be zero.
         * This is the correct result.
         */
        return (unsigned short)(h_sgn + h_sig);
    }

    /* Regular case with no overflow or underflow */
    h_exp = (unsigned short)((f_exp - 0x38000000u) >> 13);
    /* Handle rounding by adding 1 to the bit beyond half precision */
    f_sig = (f & 0x007fffffu);
#if NPY_HALF_ROUND_TIES_TO_EVEN
    /*
     * If the last bit in the half significand is 0 (already even), and
     * the remaining bit pattern is 1000...0, then we do not add one
     * to the bit after the half significand.  In all other cases, we do.
     */
    if ((f_sig & 0x00003fffu) != 0x00001000u) {
        f_sig += 0x00001000u;
    }
#else
    f_sig += 0x00001000u;
#endif
    h_sig = (unsigned short)(f_sig >> 13);
    /*
     * If the rounding causes a bit to spill into h_exp, it will
     * increment h_exp by one and h_sig will be zero.  This is the
     * correct result.  h_exp may increment to 15, at greatest, in
     * which case the result overflows to a signed inf.
     */
#if NPY_HALF_GENERATE_OVERFLOW
    h_sig += h_exp;
    if (h_sig == 0x7c00u) {
        npy_set_floatstatus_overflow();
    }
    return h_sgn + h_sig;
#else
    return h_sgn + h_exp + h_sig;
#endif
}
void floattofp16(short *dst, float *src, unsigned nelem) {
    unsigned i;
    unsigned short *_dst = (unsigned short *)dst;
    unsigned *_src = (unsigned *)src;

    for (i = 0; i < nelem; i++)
        _dst[i] = float2half(_src[i]);
}

// Function to convert F32 into F16
// F32: exp_bias:127 SEEEEEEE EMMMMMMM MMMMMMMM MMMMMMMM.
// F16: exp_bias:15  SEEEEEMM MMMMMMMM
#define EXP_MASK_F32 0x7F800000U
#define EXP_MASK_F16 0x7C00U

// small helper function to represent uint32_t value as float32
float asfloat(uint32_t v) { return *reinterpret_cast<float *>(&v); }

// Function to convert F32 into F16
float f16tof32(short x) {
    // this is storage for output result
    uint32_t u = x;

    // get sign in 32bit format
    uint32_t s = ((u & 0x8000) << 16);

    // check for NAN and INF
    if ((u & EXP_MASK_F16) == EXP_MASK_F16) {
        // keep mantissa only
        u &= 0x03FF;

        // check if it is NAN and raise 10 bit to be align with intrin
        if (u) {
            u |= 0x0200;
        }

        u <<= (23 - 10);
        u |= EXP_MASK_F32;
        u |= s;
    } else if ((x & EXP_MASK_F16) == 0) {  // check for zero and denormals. both are converted to zero
        u = s;
    } else {
        // abs
        u = (u & 0x7FFF);

        // shift mantissa and exp from f16 to f32 position
        u <<= (23 - 10);

        // new bias for exp (f16 bias is 15 and f32 bias is 127)
        u += ((127 - 15) << 23);

        // add sign
        u |= s;
    }

    // finaly represent result as float and return
    return *reinterpret_cast<float *>(&u);
}

// This function convert f32 to f16 with rounding to nearest value to minimize error
// the denormal values are converted to 0.
short f32tof16(float x) {
    // create minimal positive normal f16 value in f32 format
    // exp:-14,mantissa:0 -> 2^-14 * 1.0
    static float min16 = asfloat((127 - 14) << 23);

    // create maximal positive normal f16 value in f32 and f16 formats
    // exp:15,mantissa:11111 -> 2^15 * 1.(11111)
    static float max16 = asfloat(((127 + 15) << 23) | 0x007FE000);
    static uint32_t max16f16 = ((15 + 15) << 10) | 0x3FF;

    // define and declare variable for intermidiate and output result
    // the union is used to simplify representation changing
    union {
        float f;
        uint32_t u;
    } v;
    v.f = x;

    // get sign in 16bit format
    uint32_t s = (v.u >> 16) & 0x8000;  // sign 16:  00000000 00000000 10000000 00000000

    // make it abs
    v.u &= 0x7FFFFFFF;  // abs mask: 01111111 11111111 11111111 11111111

    // check NAN and INF
    if ((v.u & EXP_MASK_F32) == EXP_MASK_F32) {
        if (v.u & 0x007FFFFF) {
            return s | (v.u >> (23 - 10)) | 0x0200;  // return NAN f16
        } else {
            return s | (v.u >> (23 - 10));  // return INF f16
        }
    }

    // to make f32 round to nearest f16
    // create halfULP for f16 and add it to origin value
    float halfULP = asfloat(v.u & EXP_MASK_F32) * asfloat((127 - 11) << 23);
    v.f += halfULP;

    // if input value is not fit normalized f16 then return 0
    // denormals are not covered by this code and just converted to 0
    if (v.f < min16 * 0.5F) {
        return s;
    }

    // if input value between min16/2 and min16 then return min16
    if (v.f < min16) {
        return s | (1 << 10);
    }

    // if input value more than maximal allowed value for f16
    // then return this maximal value
    if (v.f >= max16) {
        return max16f16 | s;
    }

    // change exp bias from 127 to 15
    v.u -= ((127 - 15) << 23);

    // round to f16
    v.u >>= (23 - 10);

    return v.u | s;
}

void f16tof32Arrays(float *dst, const short *src, uint32_t &nelem, float scale = 1, float bias = 0) {
    VLOG(L2, "convert f16tof32Arrays...");
    const short *_src = reinterpret_cast<const short *>(src);

    for (uint32_t i = 0; i < nelem; i++) {
        dst[i] = f16tof32(_src[i]) * scale + bias;
    }
}

void f32tof16Arrays(short *dst, const float *src, uint32_t &nelem, float scale = 1, float bias = 0) {
    VLOG(L2, "convert f32tof16Arrays...");
    for (uint32_t i = 0; i < nelem; i++) {
        dst[i] = f32tof16(src[i] * scale + bias);
        // VLOG(L1, "element no: %d", i);
    }
}

int sizeOfData(V1_3::OperandType type, std::vector<uint32_t> dims) {
    int size;
    switch (type) {
    case V1_3::OperandType::FLOAT32: size = 4; break;
    case V1_3::OperandType::TENSOR_FLOAT32: size = 4; break;
    case V1_3::OperandType::TENSOR_INT32: size = 4; break;
    case V1_3::OperandType::TENSOR_QUANT8_ASYMM:
    case V1_3::OperandType::INT32: size = 1; break;

    default: size = 0;
    }
    for (auto d : dims)
        size *= d;

    return size;
}

inline size_t getSizeFromInts(int lower, int higher) {
    return (uint32_t)(lower) + ((uint64_t)(uint32_t)(higher) << 32);
}

template <typename T> T getScalarData(const RunTimeOperandInfo &info) {
    // TODO: Check buffer is at least as long as size of data.
    T *data = reinterpret_cast<T *>(info.buffer);
    return data[0];
}

// TODO: short term, make share memory mapping and updating a utility function.
// TODO: long term, implement mmap_fd as a hidl IMemory service.
bool RunTimePoolInfo::set(const hidl_memory &hidlMemory) {
    this->hidlMemory = hidlMemory;
    auto memType = hidlMemory.name();
    if (memType == "ashmem") {
        memory = mapMemory(hidlMemory);
        if (memory == nullptr) {
            LOG(ERROR) << "Can't map shared memory.";
            return false;
        }
        memory->update();
        buffer = reinterpret_cast<uint8_t *>(static_cast<void *>(memory->getPointer()));
        if (buffer == nullptr) {
            LOG(ERROR) << "Can't access shared memory.";
            return false;
        }
        return true;
    } else if (memType == "mmap_fd") {
        size_t size = hidlMemory.size();
        int fd = hidlMemory.handle()->data[0];
        int prot = hidlMemory.handle()->data[1];
        size_t offset = getSizeFromInts(hidlMemory.handle()->data[2], hidlMemory.handle()->data[3]);
        buffer = static_cast<uint8_t *>(mmap(nullptr, size, prot, MAP_SHARED, fd, offset));
        if (buffer == MAP_FAILED) {
            LOG(ERROR) << "Can't mmap the file descriptor.";
            return false;
        }
        return true;
    } else {
        LOG(ERROR) << "unsupported hidl_memory type";
        return false;
    }
}

// Making sure the output data are correctly updated after execution.
bool RunTimePoolInfo::update() {
    auto memType = hidlMemory.name();
    if (memType == "ashmem") {
        memory->commit();
        return true;
    } else if (memType == "mmap_fd") {
        int prot = hidlMemory.handle()->data[1];
        if (prot & PROT_WRITE) {
            size_t size = hidlMemory.size();
            return msync(buffer, size, MS_SYNC) == 0;
        }
    }
    // No-op for other types of memory.
    return true;
}

bool setRunTimePoolInfosFromHidlMemories(std::vector<RunTimePoolInfo> *poolInfos, const hidl_vec<hidl_memory> &pools) {
    poolInfos->resize(pools.size());
    for (size_t i = 0; i < pools.size(); i++) {
        auto &poolInfo = (*poolInfos)[i];
        if (!poolInfo.set(pools[i])) {
            LOG(ERROR) << "Could not map pool";
            return false;
        }
    }
    return true;
}

// Updates the RunTimeOperandInfo with the newly calculated shape.
// Allocate the buffer if we need to.

// This function is not used now. But leave it commented because it looks like useful on near future. Colin.
/*
static bool setInfoAndAllocateIfNeeded(RunTimeOperandInfo* info, const Shape& shape) {
    // For user-provided model output operands, the parameters must match the Shape
    // calculated from the preparation step.
    if (info->lifetime == V1_3::OperandLifeTime::SUBGRAPH_OUTPUT) {
        if (info->type != shape.type || info->dimensions != shape.dimensions) {
            LOG(ERROR) << "Invalid type or dimensions for model output";
            return false;
        }
        if (info->type == V1_3::OperandType::TENSOR_QUANT8_ASYMM &&
            (info->scale != shape.scale || info->zeroPoint != shape.offset)) {
            LOG(ERROR) << "Invalid scale or zeroPoint for model output";
            return false;
        }
    }
    info->type = shape.type;
    info->dimensions = shape.dimensions;
    info->scale = shape.scale;
    info->zeroPoint = shape.offset;
    if (info->lifetime == V1_3::OperandLifeTime::TEMPORARY_VARIABLE && info->buffer == nullptr) {
        uint32_t length = sizeOfData(info->type, info->dimensions);
        info->buffer = new uint8_t[length];
        if (info->buffer == nullptr) {
            return false;
        }
    }
    return true;
}
*/

// OutputPort handleFusion(const V1_3::Model &model, const V1_3::Operation &op, OutputPort &out, const int
// fusionIndex);

/*
template <typename T>
TensorDims toDims(const vec<T> &dims)
{
    TensorDims td;
    for (auto d: dims) td.push_back(d);
    return td;
}
*/
uint32_t getNumberOfElements(const vec<uint32_t> &dims) {
    uint32_t count = 1;
    for (size_t i = 0; i < dims.size(); i++) {
        count *= dims[i];
    }
    return count;
}

TensorDims toDims(const hidl_vec<uint32_t> &dims) {
    TensorDims td;
    for (size_t i = 0; i < dims.size(); i++) {
        td.push_back(dims[i]);
    }
    return td;
}

template <typename T> size_t product(const vec<T> &dims) {
    size_t rc = 1;
    for (auto d : dims)
        rc *= d;
    return rc;
}

TensorDims permuteDims(const TensorDims &src, const vec<unsigned int> &order) {
    TensorDims ret;
    ret.resize(order.size());

    for (int i = 0; i < src.size(); i++) {
        ret[i] = src[order[i]];
    }
    return ret;
}

// IRBlob::Ptr Permute(IRBlob::Ptr ptr, const vec<unsigned int> &order)
IRBlob::Ptr Permute(IRBlob::Ptr ptr, const vec<unsigned int> &order) {
    VLOG(L2, "Permute");
    auto orig_dims = ptr->getTensorDesc().getDims();
    auto dims = permuteDims(orig_dims, order);
    ptr->getTensorDesc().setDims(dims);

    return ptr;
}

#define PARAM_I32(i) ParseOperationInput<int32_t>(mModel, operation, i)
#define PARAM_FP(i) ParseOperationInput<float>(mModel, operation, i)
#define PARAM_BOOL(i) ParseOperationInput<bool>(mModel, operation, i)

template <typename T> struct printHelper {
    static void print(const T &value, const char *Obj) {
        (void)value;
        (void)Obj;
    }
};

template <> struct printHelper<int32_t> {
    static void print(const int32_t &value, const char *operand) { VLOG(L3, "Operand: value: %d, %s", value, operand); }
};

template <> struct printHelper<float> {
    static void print(const float &value, const char *operand) { VLOG(L3, "Operand: value: %f, %s", value, operand); }
};

template <typename T>
T IRConverter::ParseOperationInput(const V1_3::Model &model, const V1_3::Operation &operation, uint32_t index) {
    uint32_t inputIndex = operation.inputs[index];
    const auto operand = mModel.main.operands[inputIndex];
    const auto value = GetConstOperand<T>(model, inputIndex);
    VLOG(L3, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    VLOG(L3, "Operation input index: %d, operand index: %d", index, inputIndex);
    VLOG(L3, "Operation: %s", toString(operation).c_str());
    // VLOG(L1, "Operand: value: %d, %s", alue, toString(operand).c_str());
    printHelper<T>::print(value, toString(operand).c_str());
    VLOG(L3, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

    return value;
}

OutputPort IRConverter::handleFusion(const OutputPort &out, int32_t fusedOp, QuantizationParams &act_qtprms) {
    VLOG(L2, "fusedOp: %d", fusedOp);
    if (fusedOp == (int32_t)FusedActivationFunc::RELU) {
        VLOG(L2, "fusedOp is RELU");
        return ReLU(out, act_qtprms, mBuilderCtx);
    } else if (fusedOp == (int32_t)FusedActivationFunc::RELU1) {
        VLOG(L2, "fusedOp is RELU1");
        return ReLU1(out, act_qtprms, mBuilderCtx);
    } else if (fusedOp == (int32_t)FusedActivationFunc::RELU6) {
        VLOG(L2, "fusedOp is RELU6");
        return ReLU6(out, act_qtprms, mBuilderCtx);
    } else if (fusedOp == (int32_t)FusedActivationFunc::NONE) {
        VLOG(L2, "fusedOp is NONE");
        return out;
    }

    VLOG(L1, "Not defined ActivationFunc");
    return out;
}

template <typename T> T IRConverter::GetConstFromBuffer(const uint8_t *buf, uint32_t len) {
    VLOG(L3, "buf: %p, len: %d", buf, len);
    if (len != sizeof(T)) {
        VLOG(L1, "fix me: typeid(T).name() should be %zu bytes", sizeof(T));
        // fix me if buffer is of type float and if float and OperandLifeTime::CONSTANT_REFERENCE
        nnAssert(false);
    }
    return *(T *)(buf);
}

template <typename T> std::vector<T> IRConverter::GetConstVecFromBuffer(const uint8_t *buf, uint32_t len) {
    int n = len / sizeof(T);
    if (n * sizeof(T) != len) {
        VLOG(L1, "typeid(T).name() should be  multiples of %zu bytes", sizeof(T));
        nnAssert(false);
    }

    std::vector<T> ret;

    for (int i = 0; i < n; i++) {
        ret.push_back(*(T *)buf);
        buf += sizeof(T);
    }

    return ret;
}

const uint8_t *IRConverter::GetOperandMemory(const V1_3::Model &model, uint32_t index, uint32_t &len_out) {
    const auto op = model.main.operands[index];
    len_out = op.location.length;
    if (op.lifetime == V1_3::OperandLifeTime::CONSTANT_COPY) {
        if (op.location.poolIndex != 0) {
            ALOGE("CONSTANT_COPY expects poolIndex to be 0");
            nnAssert(false);
            // return &model.operandValues[op.location.offset];
        }
        VLOG(L2, "operand lifetime OperandLifeTime::CONSTANT_COPY");
        return (const_cast<uint8_t *>(&model.operandValues[op.location.offset]));
        // to.numberOfUsesLeft = 0;
    } else if (op.lifetime == V1_3::OperandLifeTime::CONSTANT_REFERENCE) {
        // auto pool = model.pools[op.location.poolIndex];
        // return (pool + op.location.offset);
        VLOG(L2, "operand lifetime OperandLifeTime::CONSTANT_REFERENCE");
        auto poolIndex = op.location.poolIndex;
        // nnAssert(poolIndex < mPoolInfos.size()); //aks fix me
        auto &r = mPoolInfos[poolIndex];
        return (const_cast<uint8_t *>(r.buffer + op.location.offset));
        // to.numberOfUsesLeft = 0;
    } else if (op.lifetime == V1_3::OperandLifeTime::SUBGRAPH_INPUT || op.lifetime == V1_3::OperandLifeTime::SUBGRAPH_OUTPUT ||
               op.lifetime == V1_3::OperandLifeTime::NO_VALUE) {
        // return const_cast<uint8_t*>(op.buffer);
        VLOG(L2, "operand lifetime OperandLifeTime::SUBGRAPH_INPUT||SUBGRAPH_OUTPUT||NO_VALUE");
        len_out = sizeOfData(op.type, op.dimensions);
        return nullptr;
    } else if (op.lifetime == V1_3::OperandLifeTime::TEMPORARY_VARIABLE) {
        // return const_cast<uint8_t*>(op.buffer);
        VLOG(L2, "operand lifetime OperandLifeTime::TEMPORARY_VARIABLE");
        VLOG(L2, "operand is expected to be const, but lifetime is %d", op.lifetime);
        len_out = sizeOfData(op.type, op.dimensions);
        // nnAssert(false);
        return nullptr;
    }

    ALOGE("operand is expected to be const, but lifetime is %d", op.lifetime);
    nnAssert(false);  // temp fix since some time const operand set as TEMPORARY_VARIABLE
    return nullptr;
}

template <typename T> T IRConverter::GetConstOperand(const V1_3::Model &model, uint32_t index) {
    dumpOperand(index);
    uint32_t len;
    const uint8_t *buf = GetOperandMemory(model, index, len);
    return GetConstFromBuffer<T>(buf, len);
}

template <typename T> std::vector<T> IRConverter::GetConstVecOperand(const V1_3::Model &model, uint32_t index) {
    dumpOperand(index);
    uint32_t len;
    const uint8_t *buf = GetOperandMemory(model, index, len);
    return GetConstVecFromBuffer<T>(buf, len);
}

OutputPort IRConverter::getPort(int index, bool toNCHW) {
    VLOG(L2, "getPort");
    if (isConst(index)) {
        VLOG(L2, "index is a const!");
        nnAssert(false);
    }
    const auto op = mModel.main.operands[index];

    if (op.lifetime == V1_3::OperandLifeTime::SUBGRAPH_INPUT) {
        VLOG(L2, "Model input operand");
        std::ostringstream operandName;
        operandName << "input" << index;

        VLOG(L3, "Model input operand 1: %zu", op.dimensions.size());
        vec<unsigned int> order;
        if (op.dimensions.size() == 4) {
            if (toNCHW) {
                order = {0, 3, 1, 2};  // nhwc -> nchw
            } else {
                order = {0, 1, 2, 3};  // no permutation needed. but keep it simple.
            }
        } else if (op.dimensions.size() == 2)
            order = {0, 1};
        else {
            for (unsigned int dimid = 0; dimid < op.dimensions.size(); dimid++) {
                order.push_back(dimid);
            }
        }

        for (auto &&d : op.dimensions) {
            VLOG(L3, "d: %u", d);
        }
        for (auto &&it = op.dimensions.begin(); it != op.dimensions.end(); ++it) {
            VLOG(L3, "it: %u", *it);
        }
        auto operandInfo =
            mNet.createInput(operandName.str(), permuteDims(toDims(op.dimensions), order));  // NHWC -> NCHW
        // auto operandInfo = mNet.createInput(operandName.str(), toDims(op.dimensions)); // NHWC
        VLOG(L2, "Model input operand 3");
        mPorts[index] = operandInfo->getInputData();
        // mPorts[index]->setLayout(NHWC); // mPorts[i]->name
        // mPorts[index]->setPrecision(InferenceEngine::Precision::FP16);
        // mPorts[index]->setPrecision(InferenceEngine::Precision::FP32);
        // TODO: workaround 3-D
        int dims_size = op.dimensions.size();

        VLOG(L3, "mPorts[%d] %s dims size %d", index, mPorts[index]->getName().c_str(), dims_size);

        auto dims = permuteDims(toDims(op.dimensions), order);
        // auto dims = toDims(op.dimensions);
        for (auto i = 0; i < dims.size(); i++)
            VLOG(L3, "input dims[%d] = %d & set input dims[%d] = %zu", i, op.dimensions[i], i, dims[i]);

        switch (dims_size) {
        case 2: mPorts[index]->setLayout(NC); break;
        case 4:
            // mPorts[index]->setLayout(NHWC);
            mPorts[index]->setLayout(NCHW);
            break;
        case 1: mPorts[index]->setLayout(C); break;
        default:
            mPorts[index]->setLayout(ANY);
            break;
            // VLOG(L1, "unsupported dims size %d", dims_size);
            // nnAssert(false);
        }

        return mPorts[index];
    }
    if (op.lifetime == V1_3::OperandLifeTime::SUBGRAPH_OUTPUT) {
        VLOG(L1, "Model output expected as input, not possible");
        nnAssert(false);
    }
    if (op.lifetime == V1_3::OperandLifeTime::NO_VALUE) {
        VLOG(L1, "port is expected to be allocated for this as output from other layer");
        nnAssert(false);
    }
    if (op.lifetime == V1_3::OperandLifeTime::TEMPORARY_VARIABLE) {
        VLOG(L2, "getport OperandLifeTime::TEMPORARY_VARIABLE\n");
        if (!mPorts[index])
            nnAssert(false);
        VLOG(L3, "mPorts[%d] already allocated\n", index);
        return mPorts[index];
        // to.buffer = nullptr;
        // to.length = sizeOfData(to.type, to.dimensions);
        // nnAssert(true);
    }

    return nullptr;
}

// uint8_t* buffer;
// The length of the buffer.
// uint32_t length;

bool IRConverter::initializeRunTimeOperandInfo() {
    // initialize runtime operand info from model.
    const size_t count = mModel.main.operands.size();
    mOperands.resize(count);
    mPorts.resize(count);
    // TensorDims dims;

    // Start by setting the runtime info to what's in the model.
    for (size_t i = 0; i < count; i++) {
        const V1_3::Operand &from = mModel.main.operands[i];
        RunTimeOperandInfo &to = mOperands[i];
        //        OutputPort& port = mPorts[i];  //std::shared_ptr<Data>
        to.dimensions.resize(from.dimensions.size());
        for (size_t j = 0; j < from.dimensions.size(); j++) {
            to.dimensions[j] = from.dimensions[j];
            // dims[j] = (size_t)from.dimensions[j];
        }

        to.scale = from.scale;
        to.zeroPoint = from.zeroPoint;

#ifdef NN_DEBUG
        VLOG(L3, "operand type = %d", from.type);
        VLOG(L3, "operand dimensions.size() = %zu", from.dimensions.size());
        VLOG(L3, "operand numberOfConsumers = %d", from.numberOfConsumers);
        VLOG(L3, "operand scale = %f", from.scale);
        VLOG(L3, "operand zeroPoint = %d", from.zeroPoint);
        VLOG(L3, "operand lifetime = %d", from.lifetime);
        VLOG(L3, "operand location.poolIndex = %d", from.location.poolIndex);
        VLOG(L3, "operand location.offset = %d", from.location.offset);
        VLOG(L3, "operand location.length = %d", from.location.length);
#endif
        switch (from.type) {
        case V1_3::OperandType::TENSOR_FLOAT32:
        case V1_3::OperandType::FLOAT32:
            // nnAssert(to.scale == 0);
            to.type = V1_3::OperandType::TENSOR_FLOAT32;
            VLOG(L3, "OperandType = %d\n", from.type);
            // port->setPrecision(InferenceEngine::Precision::FP32);
            break;
        case V1_3::OperandType::INT32:
        case V1_3::OperandType::UINT32: nnAssert(to.scale == 0); [[fallthrough]];
        case V1_3::OperandType::TENSOR_INT32:
            to.type = V1_3::OperandType::TENSOR_INT32;
            // port->setPrecision(InferenceEngine::Precision::I32);
            VLOG(L3, "OperandType::TENSOR_INT32 and operand scale value = %.1f", to.scale);
            break;
        case V1_3::OperandType::TENSOR_QUANT8_ASYMM:
            to.type = V1_3::OperandType::TENSOR_QUANT8_ASYMM;
            nnAssert(to.scale != 0);
            break;
        case V1_3::OperandType::BOOL: break;
        default:
            VLOG(L1, "wrong operand type %d", from.type);
            mErrorMessage = "wrong operand type : " + std::to_string((int)from.type);
            return false;
        }
        to.length = from.location.length;
        to.lifetime = from.lifetime;
        switch (from.lifetime) {
        case V1_3::OperandLifeTime::TEMPORARY_VARIABLE:
            to.buffer = nullptr;
            to.length = sizeOfData(to.type, to.dimensions);
            to.numberOfUsesLeft = from.numberOfConsumers;
            break;
        case V1_3::OperandLifeTime::CONSTANT_COPY:
            to.buffer = const_cast<uint8_t *>(&mModel.operandValues[from.location.offset]);
            to.numberOfUsesLeft = 0;
            break;
        case V1_3::OperandLifeTime::CONSTANT_REFERENCE: {
            auto poolIndex = from.location.poolIndex;
            nnAssert(poolIndex < mPoolInfos.size());
            auto &r = mPoolInfos[poolIndex];
            to.buffer = r.buffer + from.location.offset;
            to.numberOfUsesLeft = 0;
            break;
        }
        case V1_3::OperandLifeTime::SUBGRAPH_INPUT:
        case V1_3::OperandLifeTime::SUBGRAPH_OUTPUT:
        case V1_3::OperandLifeTime::NO_VALUE:
            to.buffer = nullptr;
            // to.length = sizeOfData(to.type, to.dimensions);
            to.numberOfUsesLeft = 0;
            break;
        default:
            mErrorMessage = "wrong operand lifetime : " + std::to_string((int)from.lifetime);
            return false;
            break;
        }
    }
    return true;
}

bool IRConverter::parse() {
    VLOG(L2, "initialize");
    bool success = false;
    try {
        mBuilderCtx.layer_count = 0;
        mNet.setPrecision(mBuilderCtx.layer_precision);
        if (isSupportedModelByDSPC(mModel) == false) {
            VLOG(L1, "This model is not supported by DSP");
            return false;
        }
        if (isSupportedModelByNPUC(mModel)) {
            VLOG(L1, "This model supported by NPUC.");
            return false;
        }

        success = setRunTimePoolInfosFromHidlMemories(&mPoolInfos, mModel.pools);
        if (!success) {
            mErrorMessage = "setRunTimePoolInfosFromHidlMemories failed.";
            VLOG(L1, "setRunTimePoolInfosFromHidlMemories failed.");
            return false;
        }

        // Check operation supoorted or not, user may not call getOpertionSupported()
        for (const auto &operation : mModel.main.operations) {
            success = isOperationSupported(operation, mModel);
            dumpOperationSupport(operation, success);
            if (!success) {
                VLOG(L1, "get unsupported operation in initialize()");
                return false;
            }
        }

        success = initializeRunTimeOperandInfo();
        if (!success) {
            VLOG(L1, "initializeRunTimeOperandInfo failed.");
            return false;
        }

        for (const auto &operation : mModel.main.operations) {
            VLOG(L2, "get operation %d ready to add", operation.type);
            dumpOperation(operation);
            switch (operation.type) {
            case V1_3::OperationType::CONV_2D: success = operationConv2D(operation); break;
            case V1_3::OperationType::DEPTHWISE_CONV_2D: success = operationDepthwiseConv2D(operation); break;
            case V1_3::OperationType::MAX_POOL_2D: success = operationMaxPool2D(operation); break;
            case V1_3::OperationType::AVERAGE_POOL_2D: success = operationAveragePool2D(operation); break;
            case V1_3::OperationType::RELU: success = operationRELU(operation); break;
            case V1_3::OperationType::RELU1: success = operationRELU1(operation); break;
            case V1_3::OperationType::RELU6: success = operationRELU6(operation); break;
            case V1_3::OperationType::LOGISTIC: success = operationLogisticSigmoid(operation); break;
            case V1_3::OperationType::TANH: success = operationTANH(operation); break;
            case V1_3::OperationType::CONCATENATION: success = operationConCat(operation); break;
            case V1_3::OperationType::SOFTMAX: success = operationSoftmax(operation); break;
            case V1_3::OperationType::LOCAL_RESPONSE_NORMALIZATION: success = operationLRN(operation); break;
            case V1_3::OperationType::FULLY_CONNECTED: success = operationFullyConnected(operation); break;
            case V1_3::OperationType::L2_NORMALIZATION: success = operationL2Normalization(operation); break;
            case V1_3::OperationType::RESHAPE: success = operationReshape(operation); break;
            case V1_3::OperationType::ADD: success = operationAdd(operation); break;
            default:
                mErrorMessage = "unsupported operation : " + std::to_string((int)(operation.type));
                VLOG(L1, "unsupported operation %d", operation.type);
                return false;
            }
            if (success == false) {
                VLOG(L1, "failed to convert operation %d", operation.type);
                return false;
            }
            VLOG(L2, "convert operation %d success", operation.type);
        }

        initializeInput();
        finalizeOutput();
        mNet.buildNetwork();

        // initialize IE operation input/output ports
        //    convertModel(mNet);

        // debug graph
        // mNet.buildNetwork(mBuilderCtx);
        // std::fstream dot;
        // std::string graphfile("/data/local/graphfile");
        // dot.open("/data/local/graph.dot", std::ios::out);
        // mNet.save(graphfile);
        // mNet.crateDotFile(dot);
        // dot.close();

        // VLOG(L1, "initialize ExecuteNetwork for device %s",
        //      InferenceEngine::TargetDeviceInfo::name(mTargetDevice));
        // enginePtr = new ExecuteNetwork(mNet, mTargetDevice);
        // enginePtr->prepareInput();
        // enginePtr->loadNetwork();
    } catch (const std::exception &error) {
        VLOG(L1, "Convert exception : %s", error.what());
        return false;
    } catch (...) {
        VLOG(L1, "Convert exception : unknown exception");
        return false;
    }

    return true;
}

void IRConverter::deinitialize() {
    VLOG(L2, "deinitialize");

    // for (const auto& operand : mOperands) {
    /*        for (const auto& buf : operand.buffer) {
                VLOG(L1, "free buffer %p of operand %p", buf, &operand);
                if (buf != nullptr)
                delete buf;
            }*/
    // VLOG(L1, "free buffer %p of operand %p", operand.buffer, &operand);
    // if (operand.buffer)
    //    delete operand.buffer;
    // }
}

template <typename T> T getOperandConstVal(const V1_3::Model &model, const V1_3::Operand &operand) {
    const T *data = reinterpret_cast<const T *>(&model.operandValues[operand.location.offset]);
    return data[0];
}

bool IRConverter::isOperationSupported(const V1_3::Operation &operation, const V1_3::Model &model) {
    VLOG(L2, "Check operation %d", operation.type);

#define VLOG_CHECKFAIL(fail) VLOG(L1, "Check failed: %s", fail)

    const auto input0 = model.main.operands[operation.inputs[0]];

    switch (operation.type) {
    case V1_3::OperationType::CONV_2D: {
        const auto &input1 = model.main.operands[operation.inputs[1]];
        auto input0_c = input0.dimensions[3];  // NHW'C'
        if (isNCHWLayoutSupportLayer(operation)) {
            input0_c = input0.dimensions[1];  // N'C'HW
        }
        // filter in == channel
        if (input0_c != input1.dimensions[3]) {
            VLOG_CHECKFAIL("filter in not equals channel");
            mErrorMessage = "filter in not equals channel_CONV2D";
            return false;
        }
        break;
        // continue to check actication.
    }

    case V1_3::OperationType::DEPTHWISE_CONV_2D: {
        const auto &input1 = model.main.operands[operation.inputs[1]];
        auto input0_c = input0.dimensions[3];  // NHW'C'
        if (isNCHWLayoutSupportLayer(operation)) {
            input0_c = input0.dimensions[1];  // N'C'HW
        }
        // channels_out must be channels * depth_mul
        if ((input1.dimensions[3] % input0_c) != 0) {
            VLOG_CHECKFAIL("dims not in group");
            mErrorMessage = "dims not in group DEPTHWISE_CONV_2D";
            return false;
        }
        break;
    }

    case V1_3::OperationType::SOFTMAX: {
        uint32_t len;
        float beta = 0.0f;
        auto pBeta = reinterpret_cast<const float *>(GetOperandMemory(model, operation.inputs[1], len));
        if (pBeta != nullptr) {
            beta = *pBeta;
        }
        // beta need = 1.0f
        // if (beta != 1.0f) {
        if (beta <= 0.0f) {
            VLOG_CHECKFAIL("beta must be positive for softmax");
            mErrorMessage = "beta must be positive for softmax";
            return false;
        }
        break;
    }

    case V1_3::OperationType::AVERAGE_POOL_2D:
    case V1_3::OperationType::MAX_POOL_2D:
    case V1_3::OperationType::FULLY_CONNECTED: {
        break;
    }

    case V1_3::OperationType::RELU:
    case V1_3::OperationType::RELU1:
    case V1_3::OperationType::RELU6: break;
    case V1_3::OperationType::LOGISTIC:
    case V1_3::OperationType::TANH:
    case V1_3::OperationType::LOCAL_RESPONSE_NORMALIZATION:
    case V1_3::OperationType::L2_NORMALIZATION:
        mErrorMessage = "unsupport operation" + std::to_string((int)(operation.type));
        return false;  // colin : Not used by AI Benchmark3 models
        break;
    case V1_3::OperationType::CONCATENATION:
    case V1_3::OperationType::RESHAPE: break;

    case V1_3::OperationType::ADD: {
        const auto &input1 = model.main.operands[operation.inputs[1]];
        if (input0.dimensions != input1.dimensions) {
            mErrorMessage = "dims not match for ADD. Broadcasting was not supported.";
            VLOG_CHECKFAIL("dims not match for ADD. Broadcasting was not supported.");
            return false;
        }
        break;
    }
    default:
        VLOG(L1, "unsupport operation %d", operation.type);
        mErrorMessage = "Unsupported Operation " + std::to_string((int)operation.type);
        return false;
    }
    VLOG(L2, "Operation %d supported by driver", operation.type);

    return true;
}
bool IRConverter::isQ8A(int index) {
    VLOG(L3, "---------------------------------------------");
    VLOG(L3, "Operand index: %d", index);
    const auto op = mModel.main.operands[index];
    VLOG(L3, " %s", toString(op).c_str());
    bool ret = (op.type == V1_3::OperandType::TENSOR_QUANT8_ASYMM);
    VLOG(L3, "%s", ret ? "Q8A Tensor" : "Non-Q8A Tensor");
    VLOG(L3, "---------------------------------------------");
    return ret;
}

bool IRConverter::isConst(int index) {
    VLOG(L3, "---------------------------------------------");
    VLOG(L3, "Operand index: %d", index);
    const auto op = mModel.main.operands[index];
    VLOG(L3, " %s", toString(op).c_str());
    bool ret = (op.lifetime == V1_3::OperandLifeTime::CONSTANT_COPY || op.lifetime == V1_3::OperandLifeTime::CONSTANT_REFERENCE);
    VLOG(L3, "%s", ret ? "Const" : "Non-Const");
    VLOG(L3, "---------------------------------------------");
    return ret;
}

bool IRConverter::operationAdd(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::ADD");
    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.inputs[1]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    OutputPort out;
    bool isIn0Const = isConst(operation.inputs[0]);
    bool isIn1Const = isConst(operation.inputs[1]);
    VLOG(L3, "isIn0Const = %d isIn1Const = %d \n", isIn0Const, isIn1Const);

    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[1]].scale, mModel.main.operands[operation.inputs[1]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    if (isIn0Const || isIn1Const) {
        if (isIn0Const && isIn1Const) {
            VLOG(L1, "adding 2 constants, we can do it now and put const as output");
            nnAssert(true);
        }
        // this will use ScaleShift
        if (isIn0Const)  // if op.inputs[1] is a Model input
            out = AddConst(
                mNet, getPort(operation.inputs[1]), GetConstOperandAsTensor(operation.inputs[0]), qtprms, mBuilderCtx);
        else  // isIn1Const is const //op.inputs[0] is a Model input
            out = AddConst(
                mNet, getPort(operation.inputs[0]), GetConstOperandAsTensor(operation.inputs[1]), qtprms, mBuilderCtx);
    } else {  // both inputs[0] & inputs[1] are model inputs
        // out = getPort(operation.inputs[0]) + getPort(operation.inputs[1]);
        out = Add(getPort(operation.inputs[0]), getPort(operation.inputs[1]), qtprms, mBuilderCtx);
    }
    // check fusion
    VLOG(L2, "check fusion parameter = %d\n", PARAM_I32(2));

    QuantizationParams act_qtprms;

    act_qtprms.inputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    act_qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = handleFusion(out, PARAM_I32(2), act_qtprms);

    VLOG(L3,
         "add mPorts[%d]->name %s + mPorts[%d]->name %s  = mPorts[%d]->name %s \n",
         operation.inputs[0],
         isIn0Const ? "isIn0Const" : mPorts[operation.inputs[0]]->getName().c_str(),
         operation.inputs[1],
         isIn1Const ? "isIn1Const" : mPorts[operation.inputs[1]]->getName().c_str(),
         operation.outputs[0],
         mPorts[operation.outputs[0]]->getName().c_str());

    return true;
}

bool IRConverter::operationAveragePool2D(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::AVERAGE_POOL_2D");
    /**
     * Performs a 2-D average pooling operation.
     *
     * The output dimensions are functions of the filter dimensions, stride, and
     * padding.
     *
     * The values in the output tensor are computed as:
     *
     *     output[batch, row, col, channel] =
     *         sum_{i, j}(input[batch, row + i, col + j, channel]) / sum(1)
     *
     * Supported tensor {@link OperandType}:
     * * {@link OperandType::TENSOR_FLOAT32}
     * * {@link OperandType::TENSOR_QUANT8_ASYMM}
     *
     * Supported tensor rank: 4, with "NHWC" (i.e., Num_samples, Height, Width,
     * and Channels) data layout.
     *
     * Both explicit padding and implicit padding are supported.
     *
     * Inputs (explicit padding):
     * * 0: A 4-D tensor, of shape [batches, height, width, depth], specifying
     *      the input.
     * * 1: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the left, in the ‘width’ dimension.
     * * 2: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the right, in the ‘width’ dimension.
     * * 3: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the top, in the ‘height’ dimension.
     * * 4: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the bottom, in the ‘height’ dimension.
     * * 5: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘width’ dimension.
     * * 6: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘height’ dimension.
     * * 7: An {@link OperandType::INT32} scalar, specifying the filter
     *      width.
     * * 8: An {@link OperandType::INT32} scalar, specifying the filter
     *      height.
     * * 9: An {@link OperandType::INT32} scalar, and has to be one of the
     *      {@link FusedActivationFunc} values. Specifies the activation to
     *      invoke on the result.
     *
     * Inputs (implicit padding):
     * * 0: A 4-D tensor, of shape [batches, height, width, depth], specifying
     *      the input.
     * * 1: An {@link OperandType::INT32} scalar, specifying the implicit
     *      padding scheme, has to be one of the
     *      following values: {0 (NONE), 1 (SAME), 2 (VALID)}.
     * * 2: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘width’ dimension.
     * * 3: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘height’ dimension.
     * * 4: An {@link OperandType::INT32} scalar, specifying the filter
     *      width.
     * * 5: An {@link OperandType::INT32} scalar, specifying the filter
     *      height.
     * * 6: An {@link OperandType::INT32} scalar, and has to be one of the
     *      {@link FusedActivationFunc} values. Specifies the activation to
     *      invoke on the result.
     *
     * Outputs:
     * * 0: The output 4-D tensor, of shape
            [batches, out_height, out_width, depth].
     */

    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    bool toNCHW = !(isNCHWLayoutSupportLayer(operation));
    auto input = getPort(operation.inputs[0], toNCHW);
    const auto indims = input->getTensorDesc().getDims();

    Point2D pad_start = {0, 0};
    Point2D pad_end = {0, 0};
    Point2D stride = {1, 1};
    Point2D kernel = {1, 1};
    std::string padType;
    int fusion_index = -1;
    Layout layout = isNCHWLayoutSupportLayer(operation) ? Layout::NCHW : Layout::NHWC;

    if (isExplicitPadding(operation)) {
        padType = "explicit";
        pad_start = {PARAM_I32(1), PARAM_I32(3)};
        pad_end = {PARAM_I32(2), PARAM_I32(4)};
        stride = {PARAM_I32(5), PARAM_I32(6)};
        kernel = {PARAM_I32(7), PARAM_I32(8)};
        fusion_index = 9;
    } else {  // implicit padding
        const auto pad_type = PARAM_I32(1);
        int stride_width = PARAM_I32(2);
        int stride_height = PARAM_I32(3);
        int filter_width = PARAM_I32(4);
        int filter_height = PARAM_I32(5);
        fusion_index = 6;
        stride = {stride_width, stride_height};
        kernel = {filter_width, filter_height};

        int input_width = indims[3];
        int input_height = indims[2];

        int padding_left, padding_right;
        int padding_top, padding_bottom;

        if (pad_type == kPaddingSame) {
            /**
             * SAME padding.
             * Padding on both ends are the "same":
             *     padding_to_beginning =  total_padding / 2
             *     padding_to_end       = (total_padding + 1)/2.
             * i.e., for even number of padding, padding to both ends are exactly
             * the same; for odd number of padding, padding to the ending is bigger
             * than the padding to the beginning by 1.
             *
             * total_padding is a function of input, stride and filter size.
             * It could be computed as follows:
             *    out_size = (input + stride - 1) / stride;
             *    needed_input = (out_size - 1) * stride + filter_size
             *    total_padding = max(0, needed_input - output_size)
             *  The computation is the same for the horizontal and vertical directions.
             */

            calculateExplicitPadding(
                input_width, stride_width, filter_width, pad_type /*padding_implicit*/, &padding_left, &padding_right);
            calculateExplicitPadding(input_height,
                                     stride_height,
                                     filter_height,
                                     pad_type /*padding_implicit*/,
                                     &padding_top,
                                     &padding_bottom);

            pad_start = {padding_left, padding_top};
            pad_end = {padding_right, padding_bottom};
            padType = "same_upper";

        } else if (pad_type == kPaddingValid) {
            /**
             * VALID padding.
             * No padding. When the input size is not evenly divisible by
             * the filter size, the input at the end that could not fill
             * the whole filter tile will simply be ignored.
             */
            pad_start = {0, 0};
            pad_end = {0, 0};
            padType = "valid";
        }
    }

    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    auto out = Pooling(input,
                       kernel,
                       stride,
                       pad_start,
                       pad_end,
                       padType,
                       InferenceEngine::PoolingLayer::PoolType::AVG,
                       qtprms,
                       layout,
                       mBuilderCtx);

    QuantizationParams act_qtprms;

    act_qtprms.inputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    act_qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = handleFusion(out, PARAM_I32(fusion_index), act_qtprms);

    return true;
}

bool IRConverter::operationMaxPool2D(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::MAX_POOL_2D");
    /*
     *  * Inputs:
     * 0: A 4-D tensor, of shape [batches, height, width, depth], specifying the input.
     * 1: An INT32 value, specifying the padding on the left, in the ‘width’ dimension.
     * 2: An INT32 value, specifying the padding on the right,in the ‘width’ dimension.
     * 3: An INT32 value, specifying the padding on the top, in the ‘height’ dimension.
     * 4: An INT32 value, specifying the padding on the bottom, in the ‘height’ dimension.
     * 5: An INT32 value, specifying the output stride in the ‘width’ dimension.
     * 6: An INT32 value, specifying the output stride in the ‘height’ dimension.
     * 7: An INT32 value, specifying the filter width.
     * 8: An INT32 value, specifying the filter height.
     * 9: An INT32 value, and has to be one of the {@link FusedActivationFunc} values.
     *    Specifies the activation to invoke on the result of each addition.
     */

    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    bool toNCHW = !(isNCHWLayoutSupportLayer(operation));
    auto input = getPort(operation.inputs[0], toNCHW);
    const auto indims = input->getTensorDesc().getDims();

    Point2D pad_start = {0, 0};
    Point2D pad_end = {0, 0};
    Point2D stride = {1, 1};
    Point2D kernel = {1, 1};
    std::string padType;
    int fusion_index = -1;
    Layout layout = isNCHWLayoutSupportLayer(operation) ? Layout::NCHW : Layout::NHWC;

    if (isExplicitPadding(operation)) {
        padType = "explicit";
        pad_start = {PARAM_I32(1), PARAM_I32(3)};
        pad_end = {PARAM_I32(2), PARAM_I32(4)};
        stride = {PARAM_I32(5), PARAM_I32(6)};
        kernel = {PARAM_I32(7), PARAM_I32(8)};
        fusion_index = 9;
    } else {  // PAD SAME
        const auto pad_type = PARAM_I32(1);
        int stride_width = PARAM_I32(2);
        int stride_height = PARAM_I32(3);
        int filter_width = PARAM_I32(4);
        int filter_height = PARAM_I32(5);
        fusion_index = 6;
        stride = {stride_width, stride_height};
        kernel = {filter_width, filter_height};

        int input_width = indims[3];
        int input_height = indims[2];

        int padding_left, padding_right;
        int padding_top, padding_bottom;

        if (pad_type == kPaddingSame) {
            /**
             * SAME padding.
             * Padding on both ends are the "same":
             *     padding_to_beginning =  total_padding / 2
             *     padding_to_end       = (total_padding + 1)/2.
             * i.e., for even number of padding, padding to both ends are exactly
             * the same; for odd number of padding, padding to the ending is bigger
             * than the padding to the beginning by 1.
             *
             * total_padding is a function of input, stride and filter size.
             * It could be computed as follows:
             *    out_size = (input + stride - 1) / stride;
             *    needed_input = (out_size - 1) * stride + filter_size
             *    total_padding = max(0, needed_input - output_size)
             *  The computation is the same for the horizontal and vertical directions.
             */

            calculateExplicitPadding(
                input_width, stride_width, filter_width, pad_type /*padding_implicit*/, &padding_left, &padding_right);
            calculateExplicitPadding(input_height,
                                     stride_height,
                                     filter_height,
                                     pad_type /*padding_implicit*/,
                                     &padding_top,
                                     &padding_bottom);

            pad_start = {padding_left, padding_top};
            pad_end = {padding_right, padding_bottom};
            padType = "same_upper";

        } else if (pad_type == kPaddingValid) {
            /**
             * VALID padding.
             * No padding. When the input size is not evenly divisible by
             * the filter size, the input at the end that could not fill
             * the whole filter tile will simply be ignored.
             */
            pad_start = {0, 0};
            pad_end = {0, 0};
            padType = "valid";
        }
    }

    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    auto out = Pooling(input,
                       kernel,
                       stride,
                       pad_start,
                       pad_end,
                       padType,
                       InferenceEngine::PoolingLayer::PoolType::MAX,
                       qtprms,
                       layout,
                       mBuilderCtx);

    QuantizationParams act_qtprms;

    act_qtprms.inputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    act_qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = handleFusion(out, PARAM_I32(fusion_index), act_qtprms);

    return true;
}

bool IRConverter::operationConCat(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::CONCATENATION");
    /*
     * Inputs:
     * 0 ~ n-1: The list on n input tensors, of shape [D0, D1, ..., Daxis(i), ..., Dm]
     * n: An INT32 value, specifying the concatenation axis.
     */
    uint32_t axis;
    auto n = operation.inputs.size() - 1;
    for (size_t inIdx = 0; inIdx < n; inIdx++) {
        nnAssertWM(isQ8A(operation.inputs[inIdx]) == true, "IFM " + std::to_string(inIdx) + " is not Q8A Tensor");
    }
    std::vector<OutputPort> inputs;
    if (getPort(operation.inputs[0])->getLayout() == InferenceEngine::NCHW) {
        std::vector<uint32_t> axisMap = {0, 2, 3, 1};
        axis = axisMap[PARAM_I32(n)];
    } else
        axis = PARAM_I32(n);

    for (int i = 0; i < n; i++)
        inputs.push_back(getPort(operation.inputs[i]));

    QuantizationParams qtprms;
    for (size_t idx = 0; idx < n; idx++) {
        qtprms.inputs.push_back(
            {mModel.main.operands[operation.inputs[idx]].scale, mModel.main.operands[operation.inputs[idx]].zeroPoint});
    }
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    auto out = Concat(inputs, qtprms, mBuilderCtx, axis);
    mPorts[operation.outputs[0]] = out;

    return true;
}

bool IRConverter::operationConv2D(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::CONV_2D");

    /**
     * Performs an 2-D convolution operation.
     *
     * The CONV_2D op sweeps a 2-D filter that can mix channels together over a
     * batch of images, applying the filter to each window of each image of the
     * appropriate size.
     *
     * The output dimensions are functions of the filter dimensions, stride, and
     * padding.
     *
     * The values in the output tensor are computed as:
     *
     *     output[batch, row, col, channel] =
     *         sum_{i, j} (
     *             input[batch, row + i, col + j, k] *
     *             filter[channel, row + i, col + j, k] +
     *             bias[channel]
     *         )
     *
     * Supported tensor {@link OperandType}:
     * * {@link OperandType::TENSOR_FLOAT32}
     * * {@link OperandType::TENSOR_QUANT8_ASYMM}
     *
     * Supported tensor rank: 4, with "NHWC" data layout.
     *
     * Both explicit padding and implicit padding are supported.
     *
     * Inputs (explicit padding):
     * * 0: A 4-D tensor, of shape [batches, height, width, depth_in],
     *      specifying the input.
     * * 1: A 4-D tensor, of shape
     *      [depth_out, filter_height, filter_width, depth_in], specifying the
     *      filter.
     * * 2: A 1-D tensor, of shape [depth_out], specifying the bias.
     *      For input tensor of {@link OperandType::TENSOR_FLOAT32}, the bias
     *      should also be of {@link OperandType::TENSOR_FLOAT32}. For input
     *      tensor of {@link OperandType::TENSOR_QUANT8_ASYMM}, the bias
     *      should be of {@link OperandType::TENSOR_INT32}, with zeroPoint of
     *      0 and bias_scale == input_scale * filter_scale.
     * * 3: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the left, in the ‘width’ dimension.
     * * 4: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the right, in the ‘width’ dimension.
     * * 5: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the top, in the ‘height’ dimension.
     * * 6: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the bottom, in the ‘height’ dimension.
     * * 7: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘width’ dimension.
     * * 8: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘height’ dimension.
     * * 9: An {@link OperandType::INT32} scalar, and has to be one of the
     *      {@link FusedActivationFunc} values. Specifies the activation to
     *      invoke on the result.
     *
     * Inputs (implicit padding):
     * * 0: A 4-D tensor, of shape [batches, height, width, depth_in],
     *      specifying the input.
     * * 1: A 4-D tensor, of shape
     *      [depth_out, filter_height, filter_width, depth_in], specifying the
     *      filter.
     * * 2: A 1-D tensor, of shape [depth_out], specifying the bias. For input
     *      tensor of {@link OperandType::TENSOR_FLOAT32}, the bias should
     *      also be of {@link OperandType::TENSOR_FLOAT32}. For input tensor
     *      of {@link OperandType::TENSOR_QUANT8_ASYMM}, the bias should be
     *      of {@link OperandType::TENSOR_INT32}, with zeroPoint of 0 and
     *      bias_scale == input_scale * filter_scale.
     * * 3: An {@link OperandType::INT32} scalar, specifying the implicit
     *      padding scheme, has to be one of the
     *      following values: {0 (NONE), 1 (SAME), 2 (VALID)}.
     * * 4: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘width’ dimension.
     * * 5: An {@link OperandType::INT32} scalar, specifying the stride when
    *       walking through input in the ‘height’ dimension.
     * * 6: An {@link OperandType::INT32} scalar, and has to be one of the
     *      {@link FusedActivationFunc} values. Specifies the activation to
     *      invoke on the result.
     *
     * Outputs:
     * * 0: The output 4-D tensor, of shape
     *      [batches, out_height, out_width, depth_out]. For output tensor of
     *      {@link OperandType::TENSOR_QUANT8_ASYMM}, the following condition
     *      must be satisfied: output_scale > input_scale * filter_scale.
     *
    CONV_2D = 3,

    ***/
    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.inputs[1]) == true, "Weight is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    bool toNCHW = !(isNCHWLayoutSupportLayer(operation));
    auto input = getPort(operation.inputs[0], toNCHW);
    nnAssertWM(isConst(operation.inputs[1]) == true, "Weight/Bias as input was not implemented yet.");
    auto filter = GetConstOperandAsTensor(operation.inputs[1]);  // OIHW
    // auto filter = GetConstWeightsOperandAsTensor(operation.inputs[1]);
    nnAssertWM(isConst(operation.inputs[2]) == true, "Weight/Bias as input was not implemented yet.");
    auto bias = GetConstOperandAsTensor(operation.inputs[2]);

    const auto inputDims = input->getTensorDesc().getDims();
    const auto filterDims = filter->getTensorDesc().getDims();

    ConvolutionParams prms;
    // prms.weights = static_cast<IRBlob::Ptr>(filter); // permute OHWI to OIHW (0->0, 3->1, 1->2,
    // 2->3) const auto dims = prms.weights->getTensorDesc().getDims(); const auto indims =
    // input->getTensorDesc().getDims(); int  fusion_index = -1;

    // int batches = (int)inputDims[0];
    int in_channels = (int)inputDims[1];
    int input_height = (int)inputDims[2];
    int input_width = (int)inputDims[3];

    int filter_in = (int)filterDims[1];
    int filter_out = (int)filterDims[0];
    int filter_height = (int)filterDims[2];
    int filter_width = (int)filterDims[3];

    // int32_t padding_left, padding_right;
    // int32_t padding_top, padding_bottom;
    // int32_t stride_width, stride_height;

    int fusion_index = -1;

    if (isExplicitPadding(operation)) {
        prms.padType = "explicit";
        prms.pad_start = {PARAM_I32(3), PARAM_I32(5)};
        prms.pad_end = {PARAM_I32(4), PARAM_I32(6)};
        prms.stride = {PARAM_I32(7), PARAM_I32(8)};
        prms.kernel = {filter_width, filter_height};
        prms.num_output_planes = filter_out;  // depth out
        if (operation.inputs.size() == 13) {  // dilation, optional
            prms.dilations = {PARAM_I32(11), PARAM_I32(12)};
        }
        fusion_index = 9;
    } else {                                 // PAD SAME
        const auto pad_type = PARAM_I32(3);  // padding_implicit
        int stride_width = PARAM_I32(4);
        int stride_height = PARAM_I32(5);
        int padding_left, padding_right;
        int padding_top, padding_bottom;
        /*
            int input_height = indims[2];
            int input_width = indims[3];

            int filter_height = dims[2];
            int filter_width = dims[3];
        */
        if (pad_type == kPaddingSame) {
            /**
             * SAME padding.
             * Padding on both ends are the "same":
             *     padding_to_beginning =  total_padding / 2
             *     padding_to_end       = (total_padding + 1)/2.
             * i.e., for even number of padding, padding to both ends are exactly
             * the same; for odd number of padding, padding to the ending is bigger
             * than the padding to the beginning by 1.
             *
             * total_padding is a function of input, stride and filter size.
             * It could be computed as follows:
             *    out_size = (input + stride - 1) / stride;
             *    needed_input = (out_size - 1) * stride + filter_size
             *    total_padding = max(0, needed_input - output_size)
             *  The computation is the same for the horizontal and vertical directions.
             */

            calculateExplicitPadding(
                input_width, stride_width, filter_width, pad_type /*padding_implicit*/, &padding_left, &padding_right);
            calculateExplicitPadding(input_height,
                                     stride_height,
                                     filter_height,
                                     pad_type /*padding_implicit*/,
                                     &padding_top,
                                     &padding_bottom);

            prms.pad_start = {padding_left, padding_top};
            prms.pad_end = {padding_right, padding_bottom};
            prms.padType = "same_upper";

        } else if (pad_type == kPaddingValid) {
            /**
             * VALID padding.
             * No padding. When the input size is not evenly divisible by
             * the filter size, the input at the end that could not fill
             * the whole filter tile will simply be ignored.
             */
            prms.pad_start = {0, 0};
            prms.pad_end = {0, 0};
            prms.padType = "valid";
        }
        prms.stride = {stride_width, stride_height};
        prms.kernel = {filter_width, filter_height};
        prms.num_output_planes = filter_out;  // depth out
        if (operation.inputs.size() == 10) {  // dilation, optional
            prms.dilations = {PARAM_I32(8), PARAM_I32(9)};
        }
        fusion_index = 6;
    }
    prms.layout = isNCHWLayoutSupportLayer(operation) ? Layout::NCHW : NHWC;

    if (bias->size() != prms.num_output_planes) {
        VLOG(L1, "biases size mismatch filer's depth");
        nnAssert(false);
    }

    // input_size (validate)
    if (filter_in != in_channels) {
        VLOG(L1, "filter depth_in size mismatch input depth");
        nnAssert(false);
    }

    // Reshape CONV_2D or use GetConstWeightsOperandAsTensor()
    /*
        //filter_in same as in_channels
        TensorDims newDims = {(uint32_t)filter_in, (uint32_t)filter_out, (uint32_t)filter_height,
       (uint32_t)filter_width};

        TensorDesc td(mBuilderCtx.layer_precision, newDims, {{newDims[2], newDims[3], newDims[0],
       newDims[1]}, {2, 3, 0, 1}}); //working for CTS

        //check diff combination btw TF lite and IE
        //2310
        //TensorDesc td(mBuilderCtx.layer_precision, newDims, {{newDims[2], newDims[3], newDims[1],
       newDims[0]}, {2, 3, 1, 0}});

        //using data_type = typename
       InferenceEngine::PrecisionTrait<mBuilderCtx.layer_precision>::value_type; //fix this to use
       calculate data type at runtime

        if (mBuilderCtx.layer_precision == InferenceEngine::Precision::FP32) {
          InferenceEngine::TBlob<float>::Ptr dst_blob =
       std::make_shared<InferenceEngine::TBlob<float>>(td); dst_blob->allocate();

          for (size_t i = 0 ; i < filter->size(); i++) {
            dst_blob->buffer().as<float*>()[dst_blob->getTensorDesc().offset(i)] =
       filter->cbuffer().as<const float*>()[filter->getTensorDesc().offset(i)]; //set Layout::NCHW
       in td for src and dst
          }

          prms.weights = static_cast<IRBlob::Ptr>(dst_blob);
        }else {
          InferenceEngine::TBlob<short>::Ptr dst_blob =
       std::make_shared<InferenceEngine::TBlob<short>>(td); //short or uint16_t
          dst_blob->allocate();

          for (size_t i = 0 ; i < filter->size(); i++) {
            dst_blob->buffer().as<short*>()[dst_blob->getTensorDesc().offset(i)] =
       filter->cbuffer().as<const short*>()[filter->getTensorDesc().offset(i)]; //set Layout::NCHW
       in td for src and dst
          }

          prms.weights = static_cast<IRBlob::Ptr>(dst_blob);
        }

    */
    prms.weights = static_cast<IRBlob::Ptr>(filter);  // layout [filter_in, filter_out, filter_height, filter_width]
    const auto weightsDims = prms.weights->getTensorDesc().getDims();

    // auto out = Convolution(input, prms) + bias;
    prms.biases = static_cast<IRBlob::Ptr>(bias);

    QuantizationParams qtprms;
    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.weight.scale = mModel.main.operands[operation.inputs[1]].scale;
    qtprms.weight.zeroPoint = mModel.main.operands[operation.inputs[1]].zeroPoint;
    qtprms.bias.scale = mModel.main.operands[operation.inputs[2]].scale;
    qtprms.bias.zeroPoint = mModel.main.operands[operation.inputs[2]].zeroPoint;
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    auto out = Convolution(input, prms, qtprms, mBuilderCtx);

    if (fusion_index < 0) {
        VLOG(L1, "invalid fusion index");
        nnAssert(false);
    }

    QuantizationParams act_qtprms;

    act_qtprms.inputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    act_qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = handleFusion(out, PARAM_I32(fusion_index), act_qtprms);

    VLOG(L3, "----------------------------------------------");
    VLOGDIMS(L3, inputDims, "inputs dims");
    VLOGDIMS(L3, filterDims, "filter dims");
    VLOGDIMS(L3, weightsDims, "weights dims");
    VLOG(L3, "----------------------------------------------");

    return true;
}

bool IRConverter::operationDepthwiseConv2D(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::DEPTHWISE_CONV_2D");
    /**
     * Performs a depthwise 2-D convolution operation.
     *
     * Given an input tensor of shape [batches, height, width, depth_in] and a
     * filter tensor of shape [1, filter_height, filter_width, depth_out]
     * containing depth_out convolutional filters of depth 1, DEPTHWISE_CONV
     * applies a different filter to each input channel (expanding from 1
     * channel to channel_multiplier channels for each), then concatenates the
     * results together.
     *
     * The output has depth_out = depth_in * depth_multiplier channels.
     * The output dimensions are functions of the filter dimensions, stride, and
     * padding.
     *
     * The values in the output tensor are computed as:
     *
     *     output[b, i, j, k * channel_multiplier + q] =
     *         sum_{di, dj} (
     *             input[b, strides[1] * i + di, strides[2] * j + dj, k] *
     *             filter[1, di, dj, k * channel_multiplier + q]
     *         )
     *
     * Supported tensor {@link OperandType}:
     * * {@link OperandType::TENSOR_FLOAT32}
     * * {@link OperandType::TENSOR_QUANT8_ASYMM}
     *
     * Supported tensor rank: 4, with "NHWC" data layout.
     *
     * Both explicit padding and implicit padding are supported.
     *
     * Inputs (explicit padding):
     * * 0: A 4-D tensor, of shape [batches, height, width, depth_in],
     *      specifying the input.
     * * 1: A 4-D tensor, of shape [1, filter_height, filter_width, depth_out],
     *      specifying the filter.
     * * 2: A 1-D tensor, of shape [depth_out], specifying the bias. For input
     *      tensor of {@link OperandType::TENSOR_FLOAT32}, the bias should
     *      also be of {@link OperandType::TENSOR_FLOAT32}. For input tensor
     *      of {@link OperandType::TENSOR_QUANT8_ASYMM}, the bias should be
     *      of {@link OperandType::TENSOR_INT32}, with zeroPoint of 0 and
     *      bias_scale == input_scale * filter_scale.
     * * 3: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the left, in the ‘width’ dimension.
     * * 4: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the right, in the ‘width’ dimension.
     * * 5: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the top, in the ‘height’ dimension.
     * * 6: An {@link OperandType::INT32} scalar, specifying the padding on
     *      the bottom, in the ‘height’ dimension.
     * * 7: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘width’ dimension.
     * * 8: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘height’ dimension.
     * * 9: An {@link OperandType::INT32} scalar, specifying the depthwise
     *      multiplier.
     * * 10: An {@link OperandType::INT32} scalar, and has to be one of the
     *       {@link FusedActivationFunc} values. Specifies the activation to
     *       invoke on the result.
     *
     * Inputs (implicit padding):
     * * 0: A 4-D tensor, of shape [batches, height, width, depth_in],
     *      specifying the input.
     * * 1: A 4-D tensor, of shape [1, filter_height, filter_width, depth_out],
     *      specifying the filter.
     * * 2: A 1-D tensor, of shape [depth_out], specifying the bias. For input
     *      tensor of {@link OperandType::TENSOR_FLOAT32}, the bias should
     *      also be of {@link OperandType::TENSOR_FLOAT32}. For input tensor
     *      of {@link OperandType::TENSOR_QUANT8_ASYMM}, the bias should be
     *      of {@link OperandType::TENSOR_INT32}, with zeroPoint of 0 and
     *      bias_scale == input_scale * filter_scale.
     * * 3: An {@link OperandType::INT32} scalar, specifying the implicit
     *      padding scheme, has to be one of the
     *      following values: {0 (NONE), 1 (SAME), 2 (VALID)}.
     * * 4: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘width’ dimension.
     * * 5: An {@link OperandType::INT32} scalar, specifying the stride when
     *      walking through input in the ‘height’ dimension.
     * * 6: An {@link OperandType::INT32} scalar, specifying the depthwise
     *      multiplier.
     * * 7: An {@link OperandType::INT32} scalar, and has to be one of the
     *      {@link FusedActivationFunc} values. Specifies the activation to
     *      invoke on the result.
     *
     * Outputs:
     * * 0: The output 4-D tensor, of shape
     *      [batches, out_height, out_width, depth_out]. For output tensor of
     *      {@link OperandType::TENSOR_QUANT8_ASYMM}, the following condition
     *      must be satisfied: output_scale > input_scale * filter_scale.
     */

    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.inputs[1]) == true, "Weight is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    bool toNCHW = !(isNCHWLayoutSupportLayer(operation));
    auto input = getPort(operation.inputs[0], toNCHW);
    // auto filter = GetConstOperandAsTensor(operation.inputs[1]); //NCHW [1, depth_out,
    // filter_height, filter_width]
    nnAssertWM(isConst(operation.inputs[1]) == true, "Weight/Bias as input was not implemented yet.");
    auto filter = GetConstOperandAsTensor(operation.inputs[1]);  //[depth_out, 1, filter_height, filter_width] OIHW
    nnAssertWM(isConst(operation.inputs[2]) == true, "Weight/Bias as input was not implemented yet.");
    auto bias = GetConstOperandAsTensor(operation.inputs[2]);

    const auto inputDims = input->getTensorDesc().getDims();
    const auto filterDims = filter->getTensorDesc().getDims();

    ConvolutionParams prms;

    int batches = (int)inputDims[0];
    int in_channels = (int)inputDims[1];
    int input_height = (int)inputDims[2];
    int input_width = (int)inputDims[3];

    int filter_in = (int)filterDims[0];
    int filter_out = (int)filterDims[1];
    int filter_height = (int)filterDims[2];
    int filter_width = (int)filterDims[3];

    // int32_t padding_left, padding_right;
    // int32_t padding_top, padding_bottom;
    // int32_t stride_width, stride_height;

    int fusion_index = -1;
    int depth_multiplier = 0;

    if (isExplicitPadding(operation)) {
        prms.padType = "explicit";
        prms.pad_start = {PARAM_I32(3), PARAM_I32(5)};
        prms.pad_end = {PARAM_I32(4), PARAM_I32(6)};
        prms.stride = {PARAM_I32(7), PARAM_I32(8)};
        prms.kernel = {(int)filter_width, (int)filter_height};
        fusion_index = 10;
        depth_multiplier = PARAM_I32(9);
        prms.groups = in_channels * depth_multiplier;             // working
        prms.num_output_planes = in_channels * depth_multiplier;  // same as filter_out; //dims[0]; //depth out
        if (operation.inputs.size() == 14) {
            prms.dilations = {PARAM_I32(12), PARAM_I32(13)};
        }

    } else {  // implicit padding
        const auto pad_type = PARAM_I32(3);
        int stride_width = PARAM_I32(4);
        int stride_height = PARAM_I32(5);

        int padding_left, padding_right;
        int padding_top, padding_bottom;

        if (pad_type == kPaddingSame) {
            /**
             * SAME padding.
             * Padding on both ends are the "same":
             *     padding_to_beginning =  total_padding / 2
             *     padding_to_end       = (total_padding + 1)/2.
             * i.e., for even number of padding, padding to both ends are exactly
             * the same; for odd number of padding, padding to the ending is bigger
             * than the padding to the beginning by 1.
             *
             * total_padding is a function of input, stride and filter size.
             * It could be computed as follows:
             *    out_size = (input + stride - 1) / stride;
             *    needed_input = (out_size - 1) * stride + filter_size
             *    total_padding = max(0, needed_input - output_size)
             *  The computation is the same for the horizontal and vertical directions.
             */
            calculateExplicitPadding(
                input_width, stride_width, filter_width, pad_type /*padding_implicit*/, &padding_left, &padding_right);
            calculateExplicitPadding(input_height,
                                     stride_height,
                                     filter_height,
                                     pad_type /*padding_implicit*/,
                                     &padding_top,
                                     &padding_bottom);

            prms.pad_start = {padding_left, padding_top};
            prms.pad_end = {padding_right, padding_bottom};
            prms.padType = "same_upper";

        } else if (pad_type == kPaddingValid) {
            /**
             * VALID padding.
             * No padding. When the input size is not evenly divisible by
             * the filter size, the input at the end that could not fill
             * the whole filter tile will simply be ignored.
             */
            prms.pad_start = {0, 0};
            prms.pad_end = {0, 0};
            prms.padType = "valid";
        }
        prms.stride = {stride_width, stride_height};
        prms.kernel = {(int)filter_width, (int)filter_height};
        fusion_index = 7;
        depth_multiplier = PARAM_I32(6);
        prms.groups = in_channels * depth_multiplier;             // working
        prms.num_output_planes = in_channels * depth_multiplier;  // same as filter_out;//depth out
        if (operation.inputs.size() == 11) {
            prms.dilations = {PARAM_I32(9), PARAM_I32(10)};
        }
    }
    prms.layout = isNCHWLayoutSupportLayer(operation) ? Layout::NCHW : NHWC;

    /*
    TF filter: 4-D with shape [filter_height, filter_width, in_channels, channel_multiplier].
    reshape to org layout of TF [filter_height, filter_width, in_channels, channel_multiplier]
    then permute 2, 3, 0, 1
    prms.weights = static_cast<IRBlob::Ptr>(Permute(filter, {2, 3, 0, 1}));

    group is same as in_channels and the number of output features map as
    in_channels*depth_multiplier and permute weights to (in_chennels, channel_multiplier,
    filter_height, filter_width) where assuming TF original input filter shape as [filter_height,
    filter_width,  in_channels, channel_multiplier] to preapare in the layout expected by IE
    */

    /*
      //Reshape DEPTHWISE_CONV_2D
      //filter_out same as in_channels if depth_multiplier = 1
      TensorDims newDims = {(uint32_t)in_channels, (uint32_t)depth_multiplier,
      (uint32_t)filter_height, (uint32_t)filter_width}; //channel_multiplier == depth_multiplier
      working for CTS
      //TensorDims newDims = {(uint32_t)depth_multiplier, (uint32_t)in_channels,
      (uint32_t)filter_height, (uint32_t)filter_width};
      //TensorDims newDims = {1, in_channels*depth_multiplier, filter_height, filter_width};
      //original filter shape //channel_multiplier == depth_multiplier

      //TensorDesc td(mBuilderCtx.layer_precision, newDims, {{newDims[2], newDims[3], newDims[0],
      newDims[1]}, {2, 3, 0, 1}}); //working for CTS TensorDesc td(mBuilderCtx.layer_precision,
      newDims, {{newDims[1], newDims[0], newDims[2], newDims[3]}, {1, 0, 2, 3}});
      //TensorDesc td(InferenceEngine::Precision::FP16, newDims, {{newDims[3], newDims[2],
      newDims[1], newDims[0]}, {3, 2, 1, 0}});

      //using data_type = typename
      InferenceEngine::PrecisionTrait<mBuilderCtx.layer_precision>::value_type;

      if (mBuilderCtx.layer_precision == InferenceEngine::Precision::FP32) {
        InferenceEngine::TBlob<float>::Ptr dst_blob =
      std::make_shared<InferenceEngine::TBlob<float>>(td); dst_blob->allocate();

        for (size_t i = 0 ; i < filter->size(); i++) {
          dst_blob->buffer().as<float*>()[dst_blob->getTensorDesc().offset(i)] =
      filter->cbuffer().as<const float*>()[filter->getTensorDesc().offset(i)]; //set Layout::NCHW in
      td for src and dst
        }

        prms.weights = static_cast<IRBlob::Ptr>(dst_blob);
      }else {
        InferenceEngine::TBlob<short>::Ptr dst_blob =
      std::make_shared<InferenceEngine::TBlob<short>>(td); //short or uint16_t dst_blob->allocate();

        for (size_t i = 0 ; i < filter->size(); i++) {
          dst_blob->buffer().as<short*>()[dst_blob->getTensorDesc().offset(i)] =
      filter->cbuffer().as<const short*>()[filter->getTensorDesc().offset(i)]; //set Layout::NCHW in
      td for src and dst
        }

        prms.weights = static_cast<IRBlob::Ptr>(dst_blob);
      }
    */

    prms.weights = static_cast<IRBlob::Ptr>(filter);

    const auto weightDims = prms.weights->getTensorDesc().getDims();

    VLOG(L3,
         "batches %d, channels %d, input_height: %d, input_width %d",
         batches,
         in_channels,
         input_height,
         input_width);
    VLOG(L3,
         "filter_in %d, filter_out %d, filter_height: %d, filter_width %d",
         filter_in,
         filter_out,
         filter_height,
         filter_width);
    VLOG(L3, "depth multiplier %d", depth_multiplier);
    nnAssert(filter_out == in_channels * depth_multiplier);

    // auto out = Convolution(input, prms) + bias;
    prms.biases = static_cast<IRBlob::Ptr>(bias);

    QuantizationParams qtprms;
    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.weight.scale = mModel.main.operands[operation.inputs[1]].scale;
    qtprms.weight.zeroPoint = mModel.main.operands[operation.inputs[1]].zeroPoint;
    qtprms.bias.scale = mModel.main.operands[operation.inputs[2]].scale;
    qtprms.bias.zeroPoint = mModel.main.operands[operation.inputs[2]].zeroPoint;
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    auto out = Convolution(input, prms, qtprms, mBuilderCtx);

    if (fusion_index < 0) {
        VLOG(L1, "invalid fusion index");
        nnAssert(false);
    }
    QuantizationParams act_qtprms;

    act_qtprms.inputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    act_qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = handleFusion(out, PARAM_I32(fusion_index), act_qtprms);

    VLOG(L3, "----------------------------------------------");
    VLOGDIMS(L3, inputDims, "inputs dims");
    VLOGDIMS(L3, filterDims, "filter dims");
    VLOGDIMS(L3, weightDims, "weight dims");
    VLOG(L3, "----------------------------------------------");

    return true;
}

bool IRConverter::operationFullyConnected(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::FULLY_CONNECTED");
    /**
     * Denotes a fully (densely) connected layer, which connects all elements
     * in the input tensor with each element in the output tensor.
     *
     * This layer implements the operation:
     *
     *     outputs = activation(inputs * weights’ + bias)
     *
     * Supported tensor {@link OperandType}:
     * * {@link OperandType::TENSOR_FLOAT32}
     * * {@link OperandType::TENSOR_QUANT8_ASYMM}
     *
     * Supported tensor rank: up to 4.
     *
     * Inputs:
     * * 0: A tensor of at least rank 2, specifying the input. If rank is
     *      greater than 2, then it gets flattened to a 2-D Tensor. The
     *      (flattened) 2-D Tensor is reshaped (if necessary) to
     *      [batch_size, input_size], where "input_size" corresponds to the
     *      number of inputs to the layer, matching the second dimension of
     *      weights, and "batch_size" is calculated by dividing the number of
     *      elements by "input_size".
     * * 1: A 2-D tensor, specifying the weights, of shape
     *      [num_units, input_size], where "num_units" corresponds to the number
     *      of output nodes.
     * * 2: A 1-D tensor, of shape [num_units], specifying the bias. For input
     *      tensor of {@link OperandType::TENSOR_FLOAT32}, the bias should
     *      also be of {@link OperandType::TENSOR_FLOAT32}. For input tensor
     *      of {@link OperandType::TENSOR_QUANT8_ASYMM}, the bias should be
     *      of {@link OperandType::TENSOR_INT32}, with zeroPoint of 0 and
     *      bias_scale == input_scale * filter_scale.
     * * 3: An {@link OperandType::INT32} scalar, and has to be one of the
     *      {@link FusedActivationFunc} values. Specifies the activation to
     *      invoke on the result.
     *
     * Outputs:
     * * 0: The output tensor, of shape [batch_size, num_units]. For output
     *      tensor of {@link OperandType::TENSOR_QUANT8_ASYMM}, the following
     *      condition must be satisfied:
     *      output_scale > input_scale * filter_scale.

    FULLY_CONNECTED = 9,
     */

    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.inputs[1]) == true, "Weight is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    auto input = getPort(operation.inputs[0]);
    nnAssertWM(isConst(operation.inputs[1]) == true, "Weight/Bias as input was not implemented yet.");
    auto weights = GetConstOperandAsTensor(operation.inputs[1]);
    nnAssertWM(isConst(operation.inputs[2]) == true, "Weight/Bias as input was not implemented yet.");
    auto bias = GetConstOperandAsTensor(operation.inputs[2]);

    auto inputDims = input->getTensorDesc().getDims();
    for (auto i = 0; i < inputDims.size(); i++)
        VLOG(L3, "input dims[%d] = %zu ", i, inputDims[i]);
    auto weightsDims = weights->getTensorDesc().getDims();
    for (auto i = 0; i < weightsDims.size(); i++)
        VLOG(L3, "weights dims[%d] = %zu ", i, weightsDims[i]);

    auto biasDims = bias->getTensorDesc().getDims();

    // input is [batch_size, input_size], weights is [num_unit, input_size]
    // nnAssert(inputDims[1] == weightsDims[1]);
    nnAssert(inputDims.size() >= 2);
    nnAssert(weightsDims.size() == 2);
    uint32_t numInputElements = sizeOf(inputDims);

    if (inputDims.size() == 2 || inputDims.size() == 4) {
        nnAssertWM(inputDims[1] == numInputElements, "IFM of FC need flatten. DSP does not support reshape/flatten.");
    } else {
        nnAssertWM(false, "DSP can handle IFM dimension 2 or 4 only.");
    }

    uint32_t num_units = weightsDims[0];
    uint32_t input_size = weightsDims[1];
    uint32_t batch_size = numInputElements / input_size;
    nnAssertWM(batch_size == 1, "batch run of FullyConnected case is not supported.");
    nnAssert(biasDims[0] == num_units);
    nnAssert(input_size * batch_size == numInputElements);
#if 0
    if (inputDims.size() > 2) {
        // todo: could be we need to rotate the input weights to reflect the different layout of
        // input tensor when it is not 2D: NHWC vs NCHW in IE
        // Reshape
        // input = Reshape({inputDims[0], product(inputDims)/inputDims[0]}, input);

        TensorDims outDims = {
            (uint32_t)-1,
            numInputElements / batch_size};  // fix me: find correct outDims and if -1 is fine

        int strechDim = -1;
        auto numOutputElements = 1;  // shape
        for (auto i = 0; i < outDims.size(); i++) {
            VLOG(L1, "shape of output tensor outDims[%d] = %d ", i, outDims[i]);
            if ((int)outDims[i] < 0) {
                strechDim = i;  // strechdim
                VLOG(L1, "strechDim = %d", i);
                continue;
            }
            numOutputElements *= outDims[i];  // shape
        }
        if (strechDim >= 0) {
            auto strechValue = numInputElements / numOutputElements;
            outDims[strechDim] = (uint32_t)strechValue;
            numOutputElements *= strechValue;

            VLOG(L1, "numInputElements = %d, index = %d, outDims[index] = %d", numInputElements,
                 strechDim, outDims[strechDim]);
        }

        QuantizationParams qtprms;

        qtprms.inputs.push_back({mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
        qtprms.outputs.push_back({mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
        qtprms.weight.scale = mModel.main.operands[operation.inputs[1]].scale;
        qtprms.weight.zeroPoint = mModel.main.operands[operation.inputs[1]].zeroPoint;
        qtprms.bias.scale = mModel.main.operands[operation.inputs[2]].scale;
        qtprms.bias.zeroPoint = mModel.main.operands[operation.inputs[2]].zeroPoint;

        input = Reshape(outDims, input, qtprms, mBuilderCtx);

        /*
                //Reshape
                TensorDims newDims = {batch_size, input_n_elements/batch_size};

                auto precision = input->getPrecision();
                if (precision == InferenceEngine::Precision::FP16) {
                  TensorDesc td(InferenceEngine::Precision::FP16, newDims, {{newDims[0],
           newDims[1]}, {0, 1}});

                  InferenceEngine::TBlob<short>::Ptr dst_input_blob =
           std::make_shared<InferenceEngine::TBlob<short>>(td); dst_input_blob->allocate();

                  if (input->cbuffer() != nullptr) {
                      for (size_t i = 0 ; i < input->size(); i++) {
                        dst_input_blob->buffer().as<short*>()[dst_blob->getTensorDesc().offset(i)] =
           input->cbuffer().as<const short*>()[filter->getTensorDesc().offset(i)]; //set
           Layout::NCHW in td for src and dst
                      }
                  }
                }
                else if (precision == InferenceEngine::Precision::FP32) {
                  TensorDesc td(InferenceEngine::Precision::FP32, newDims, {{newDims[0],
           newDims[1]}, {0, 1}});

                  InferenceEngine::TBlob<float>::Ptr dst_input_blob =
           std::make_shared<InferenceEngine::TBlob<float>>(td);

                  if (input->cbuffer() != nullptr){
                      dst_input_blob->allocate();
                      for (size_t i = 0 ; i < input->size(); i++) {
                        dst_input_blob->buffer().as<float*>()[dst_blob->getTensorDesc().offset(i)] =
           input->cbuffer().as<const float*>()[filter->getTensorDesc().offset(i)]; //set
           Layout::NCHW in td for src and dst
                      }
                  }
                }

                input = static_cast<IRBlob::Ptr>(dst_input_blob);
        */
    }
#endif
    const auto newInputDims = input->getTensorDesc().getDims();

    /*
        //FIX ME : Work around since input size indims[0] != output nodes (wdims[0])
        auto dims = permuteDims(weights->getTensorDesc().getDims(), {0, 1});
        dims[0] = indims[0];
        weights->getTensorDesc().setDims(dims);
        //WA end
    */

    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    qtprms.weight.scale = mModel.main.operands[operation.inputs[1]].scale;
    qtprms.weight.zeroPoint = mModel.main.operands[operation.inputs[1]].zeroPoint;
    qtprms.bias.scale = mModel.main.operands[operation.inputs[2]].scale;
    qtprms.bias.zeroPoint = mModel.main.operands[operation.inputs[2]].zeroPoint;

    auto out = FullyConnected(input, weights, bias, qtprms, mBuilderCtx);
    // auto out = weights * input + bias;

    QuantizationParams act_qtprms;

    act_qtprms.inputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    act_qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    mPorts[operation.outputs[0]] = handleFusion(out, PARAM_I32(3), act_qtprms);

    VLOG(L3, "----------------------------------------------");
    VLOGDIMS(L3, inputDims, "inputs dims");
    VLOGDIMS(L3, newInputDims, "newInput dims");
    VLOGDIMS(L3, weightsDims, "weights dims");
    VLOG(L3, "----------------------------------------------");

    return true;
}

bool IRConverter::operationL2Normalization(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::L2_NORMALIZATION");
    /*
     * Inputs:
     * 0: A 4-D tensor, of shape [batches, height, width, depth], specifying the input.
     *
     * Ouputs:
     * 0: The output 4-D tensor, of shape [batches, out_height, out_width, depth].
     */
    // mPorts[operation.outputs[0]] = L2Normalization(getPort(operation.inputs[0]), true, false);
    mPorts[operation.outputs[0]] =
        L2Normalization(getPort(operation.inputs[0]), false, false, mBuilderCtx);  // passing accross false
    return true;
}

bool IRConverter::operationLRN(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::LOCAL_RESPONSE_NORMALIZATION");

    /*
     * * Inputs:
     * 0: A 4-D tensor, of shape [batches, height, width, depth], specifying the input.
     * 1: An INT32 value, specifying the radius of the normalization window.
     * 2: A FLOAT32 value, specifying the bias, must not be zero.
     * 3: A FLOAT32 value, specifying the scale factor, alpha.
     * 4: A FLOAT32 value, specifying the exponent, beta.
     */

    float alpha = PARAM_FP(3);
    float beta = PARAM_FP(4);
    int size = PARAM_I32(1);
    float k = PARAM_FP(2);
    // mPorts[operation.outputs[0]] = LRN(getPort(operation.inputs[0]), alpha, beta, size, true, k);
    mPorts[operation.outputs[0]] = LRN(getPort(operation.inputs[0]), alpha, beta, size, mBuilderCtx, false, k);

    return true;
}

bool IRConverter::operationLogisticSigmoid(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::LOGISTIC");
    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = Sigmoid(getPort(operation.inputs[0]), qtprms, mBuilderCtx);

    return true;
}
/*
bool IRConverter::operationLSTM(const Operation& operation)
{
    VLOG("operation type LSTM is supported, but not yet in this implementation");
    nnAssert(true);

    //return true;
}
*/
bool IRConverter::operationMUL(const V1_3::Operation &operation) {
    QuantizationParams act_qtprms;

    act_qtprms.inputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});
    act_qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[1]].scale, mModel.main.operands[operation.inputs[1]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    auto out = Mul(getPort(operation.inputs[0]), getPort(operation.inputs[1]), qtprms, mBuilderCtx);
    mPorts[operation.outputs[0]] = handleFusion(out, PARAM_I32(2), act_qtprms);
    return true;
}

bool IRConverter::operationRELU(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::RELU");
    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");

    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = ReLU(getPort(operation.inputs[0]), qtprms, mBuilderCtx);
    return true;
}

bool IRConverter::operationRELU1(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::RELU1");
    /**
     * Computes rectified linear 1 activation on the input tensor element-wise.
     *
     * In details:
     *     output = min(1.f, max(-1.f, input))
     *
     * Supported tensor types: {@link OperandType::TENSOR_FLOAT32}
     *                         {@link OperandType::TENSOR_QUANT8_ASYMM}
     * Supported tensor rank: up to 4.
     *
     * Inputs:
     * 0: A tensor, specifying the input.
     *
     * Ouputs:
     * 0: The output tensor of same shape as input0.
     */

    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = ReLU1(getPort(operation.inputs[0]), qtprms, mBuilderCtx);
    return true;
}

bool IRConverter::operationRELU6(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::RELU6");
    /**
     * Computes rectified linear 6 activation on the input tensor element-wise.
     *
     * In details:
     *     output = min(6, max(0, input))
     *
     * Supported tensor types: {@link OperandType::TENSOR_FLOAT32}
     *                         {@link OperandType::TENSOR_QUANT8_ASYMM}
     * Supported tensor rank: up to 4.
     *
     * Inputs:
     * 0: A tensor, specifying the input.
     *
     * Ouputs:
     * 0: The output tensor of same shape as input0.
     */

    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = ReLU6(getPort(operation.inputs[0]), qtprms, mBuilderCtx);
    return true;
}

bool IRConverter::operationReshape(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::RESHAPE");

    /**
     * Reshapes a tensor.
     *
     * Given tensor, this operation returns a tensor that has the same values as tensor,
     * but with a newly specified shape.
     *
     * Supported tensor types: {@link OperandType::TENSOR_FLOAT32}
     *                         {@link OperandType::TENSOR_QUANT8_ASYMM}
     * Supported tensor rank: up to 4.
     *
     * Inputs:
     * 0: A tensor, specifying the tensor to be reshaped.
     * 1: A 1-D tensor of type {@link OperandType::TENSOR_INT32}, defining the shape
     *    of the output tensor. The number of elements implied by shape must be the same
     *    as the number of elements in the input tensor.
     *
     * Ouputs:
     * 0: The output tensor, of shape specified by the input shape.
     */
    /* note: todo: We need to be careful here, inter-tensors are in different order,
     *       could be we need to reflect this also in reshape..
     */

    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    nnAssertWM(isConst(operation.inputs[1]) == true, "Weight/Bias as input was not implemented yet.");
    auto input = getPort(operation.inputs[0]);
    auto inDims = input->getTensorDesc().getDims();

    auto outDims = toDims(GetConstVecOperand<uint32_t>(mModel, operation.inputs[1]));

    // Reshape allows one of the targetDims components to have the
    // special -1 value, meaning it will be calculated automatically based on the
    // input. Here we calculate what that dimension should be so that the number
    // of output elements in the same as the number of input elements.

    // auto numInputElements = getNumberOfElements(static_cast<const vec<uint32_t>> (inDims));
    auto numInputElements = sizeOf(inDims);  // getNumberOfElements

    int strechDim = -1;
    auto numOutputElements = 1;  // shape
    for (auto i = 0; i < outDims.size(); i++) {
        VLOG(L3, "operand1: shape of output tensor outDims[%d] = %zu ", i, outDims[i]);
        if ((int)outDims[i] < 0) {
            strechDim = i;  // strechdim
            VLOG(L3, "strechDim = %d", i);
            continue;
        }
        numOutputElements *= outDims[i];  // shape
    }
    if (strechDim >= 0) {
        auto strechValue = numInputElements / numOutputElements;
        outDims[strechDim] = (uint32_t)strechValue;
        numOutputElements *= strechValue;

        VLOG(L3,
             "numInputElements or size = %zu, index = %d, outDims[index] = %zu",
             numInputElements,
             strechDim,
             outDims[strechDim]);
    }

    for (auto i = 0; i < outDims.size(); i++)
        VLOG(L3, "operand1: shape of output tensor outDims[%d] = %zu ", i, outDims[i]);
    if (numInputElements != numOutputElements) {
        VLOG(L3, "numInputElements is not equal to numOutputElements(%zu != %d)", numInputElements, numOutputElements);
        nnAssert(false);
    }

    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = Reshape(outDims, input, qtprms, mBuilderCtx);

    return true;
}

bool IRConverter::operationSoftmax(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::SOFTMAX");

    /**
     * Computes the softmax activation on the input tensor element-wise, per
     * batch, by normalizing the input vector so the maximum coefficient is
     * zero.
     *
     * The output is calculated using this formula:
     *
     *     output[batch, i] =
     *         exp((input[batch, i] - max(input[batch, :])) * beta) /
     *         sum_{k}{exp((input[batch, k] - max(input[batch, :])) * beta)}
     *
     * Supported tensor {@link OperandType}:
     * * {@link OperandType::TENSOR_FLOAT32}
     * * {@link OperandType::TENSOR_QUANT8_ASYMM}
     *
     * Supported tensor rank: 2 or 4.
     *
     * Inputs:
     * * 0: A 2-D or 4-D tensor, specifying the tensor to be reshaped.
     * * 1: An {@link OperandType::FLOAT32} scalar, specifying the positive
     *      scaling factor for the exponent, beta.
     *
     * Outputs:
     * * 0: The output tensor of same shape as input0.
     *      For {@link OperandType::TENSOR_QUANT8_ASYMM},
     *      the scale must be 1.f / 256 and the zeroPoint must be 0.
     */

    nnAssertWM(isQ8A(operation.inputs[0]) == true, "IFM is not Q8A Tensor");
    nnAssertWM(isQ8A(operation.outputs[0]) == true, "OFM is not Q8A Tensor");
    auto input = getPort(operation.inputs[0]);

    /*
        //handle 2D and 4D tensors
        auto inputDims = input->getTensorDesc().getDims();

        if (inputDims.size() == 2) {
            uint32_t batch_size = inputDims[0];//getSizeOfDimension(inputShape, 0);
            uint32_t input_size = sizeOf(inputDims) / batch_size; //getNumberOfElements(inputShape)
       / batch_size;

            //Shape shapeIn4D;
            //shapeIn4D.dimensions = {batch_size, 1, 1, input_size};
            TensorDims newDims = {batch_size, 1, 1, input_size};
            //inputDims = newDims;
            input->getTensorDesc().setDims(newDims);
            //dim = convertShapeToDims(shapeIn4D);

        } else if (inputDims.size() == 4) {
            //dim = convertShapeToDims(inputShape);
            //newDims = inputDims;
        } else {
            #ifdef NNLOG
            ALOGI("Softmax only 2D and 4D tensors supported");
            #endif
            //return false;
        }
    */

    int axis = -1;
    if (operation.inputs.size() == 3) {
        axis = PARAM_I32(2);
    }
    if (axis < 0) {
        axis += input->getTensorDesc().getDims().size();
    }
    if (input->getTensorDesc().getDims().size() == 4) {  // was NHWC, but permuted into NCHW
        switch (axis) {
        case 1: axis = 2; break;  // H
        case 2: axis = 3; break;  // W
        case 3: axis = 4; break;  // C
        default: break;
        }
    }
    float beta /*scale*/ = PARAM_FP(1);

    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = Softmax(input, beta, axis, qtprms, mBuilderCtx);
    /*
        if (scale != 1.0f) {
            ALOGE("scale of softmax not suported");
            nnAssert(false);
        }
    */
    VLOG(L3, "Softmax beta = %f ", beta);

    if (beta <= 0.0f) {
        VLOG(L1, "beta must be positive for softmax");
        nnAssert(false);
    }

    return true;
}

bool IRConverter::operationTANH(const V1_3::Operation &operation) {
    VLOG(L2, "OperationType::TANH");
    QuantizationParams qtprms;

    qtprms.inputs.push_back(
        {mModel.main.operands[operation.inputs[0]].scale, mModel.main.operands[operation.inputs[0]].zeroPoint});
    qtprms.outputs.push_back(
        {mModel.main.operands[operation.outputs[0]].scale, mModel.main.operands[operation.outputs[0]].zeroPoint});

    mPorts[operation.outputs[0]] = Tanh(getPort(operation.inputs[0]), qtprms, mBuilderCtx);

    return true;
}

void IRConverter::initializeInput() {
    VLOG(L2, "initialize Input");
    for (auto i : mModel.main.inputIndexes) {
        int dims_size = mOperands[i].dimensions.size();

        /*
            switch(dims_size) {
                case 2:
                    mPorts[i]->setLayout(NC);
                    break;
                case 4:
                    mPorts[i]->setLayout(NCHW);
                    break;
                case 1:
                    mPorts[i]->setLayout(C);
                    break;
                default:
                    VLOG(L1, "unsupported dims size %d", dims_size);
                    nnAssert(true);
            }
        */
        // mPorts[i]->setPrecision(InferenceEngine::Precision::FP16);

        VLOG(L3, "mPorts[%d] %s dims size %d", i, mPorts[i]->getName().c_str(), dims_size);
        VLOGDIMS(L2, mOperands[i].dimensions, "current operand inpu dims:");
        VLOGDIMS(L2, mPorts[i]->getTensorDesc().getDims(), "Real input dims:");

        auto inputDims = mPorts[i]->getTensorDesc().getDims();

        /*
        for (auto j = 0; j < outputDims.size(); j++)
        VLOG(L1, "output dims[%d] = %d & set output dims[%d] = %d ", j, mOperands[i].dimensions[j],
        j, outputDims[j]); VLOG(L1, "intialization for output data mPorts[%d]->name = %s\n", i,
        mPorts[i]->name.c_str());
        */

        uint32_t nelem = getNumberOfElements(mOperands[i].dimensions);
        auto inputElem = sizeOf(inputDims);
        if (nelem != inputElem) {
            VLOG(L2, "set operand input dims to real input dims\n");
            for (auto j = 0; j < inputDims.size(); j++)
                mOperands[i].dimensions[j] = static_cast<uint32_t>(inputDims[j]);
            mOperands[i].length = sizeOfData(mOperands[i].type, mOperands[i].dimensions);
        }
    }
}

void IRConverter::finalizeOutput(/*RunTimeOperandInfo* output */) {
    VLOG(L2, "finalize Output");
    for (auto i : mModel.main.outputIndexes) {
        int dims_size = mOperands[i].dimensions.size();

        /*
            switch(dims_size) {
                case 2:
                    mPorts[i]->setLayout(NC);
                    break;
                case 4:
                    mPorts[i]->setLayout(NHWC);
                    break;
                case 1:
                    mPorts[i]->setLayout(C);
                    break;
                default:
                    VLOG(L1, "unsupported dims size %d", dims_size);
                    nnAssert(true);
            }
        */
        // mPorts[i]->setPrecision(InferenceEngine::Precision::FP16);
        mPorts[i]->setPrecision(InferenceEngine::Precision::FP32);
        mNet.addOutput(mPorts[i]);

        VLOG(L3, "mPorts[%d] %s dims size %d", i, mPorts[i]->getName().c_str(), dims_size);
        VLOGDIMS(L2, mOperands[i].dimensions, "current operand Output dims:");
        VLOGDIMS(L2, mPorts[i]->getTensorDesc().getDims(), "Real Output dims:");

        auto outputDims = mPorts[i]->getTensorDesc().getDims();

        /*
        for (auto j = 0; j < outputDims.size(); j++)
        VLOG(L1, "output dims[%d] = %d & set output dims[%d] = %d ", j, mOperands[i].dimensions[j],
        j, outputDims[j]); VLOG(L1, "intialization for output data mPorts[%d]->name = %s\n", i,
        mPorts[i]->name.c_str());
        */

        uint32_t nelem = getNumberOfElements(mOperands[i].dimensions);
        auto outputElem = sizeOf(outputDims);
        if (nelem != outputElem) {
            VLOG(L2, "set correct dims as operand output dims different than real output dims\n");
            /*
            for (auto j = 0; j < outputDims.size(); j++)
            mOperands[i].dimensions[j] = static_cast<uint32_t>(outputDims[j]);
            mOperands[i].length = sizeOfData(mOperands[i].type, mOperands[i].dimensions);
            */
        }
    }
}

IRBlob::Ptr IRConverter::GetConstWeightsOperandAsTensor(uint32_t index) {
    dumpOperand(index);
    const auto op = mModel.main.operands[index];
    uint32_t len;
    const uint8_t *buf = GetOperandMemory(mModel, index, len);
    VLOG(L3, "IRConverter:: Operand: index: %d, len: %d, buf: %p", index, len, buf);
    if (op.type == V1_3::OperandType::TENSOR_FLOAT32 || op.type == V1_3::OperandType::FLOAT32) {
        vec<unsigned int> order;
        Layout layout;
        if (op.dimensions.size() == 4) {
            // order = {0,3,1,2};  //nhwc -> nchw
            order = {3, 0, 1, 2};  // IHWO -> OIHW for depth conv
            // layout = Layout::NCHW;
            // layout = Layout::NHWC;
            layout = Layout::OIHW;  // weights layout
        } else if (op.dimensions.size() == 2) {
            order = {0, 1};
            layout = Layout::NC;
        } else {
            order = {0};  //(op.dimensions.size() < 2)
            layout = Layout::C;
        }
        auto inputDims = toDims(op.dimensions);
        TensorDesc td(InferenceEngine::Precision::FP32, permuteDims(inputDims, order), layout);
        // TensorDesc td(InferenceEngine::Precision::FP32, toDims(op.dimensions), layout);
        if (buf == nullptr) {
            VLOG(L1, "TENSOR_FLOAT32 buf is NULL !!!!!!!!!!!!!!!");
            InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
            blob->allocate();
            return blob;
        } else {
            if (inputDims.size() != 4) {
                InferenceEngine::TBlob<float>::Ptr blob =
                    std::make_shared<InferenceEngine::TBlob<float>>(td, (float *)buf, len);
                return blob;
            } else {
                InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
                blob->allocate();

                auto dims_ohwi = toDims(op.dimensions);
                // auto dims_ohwi = inputDims;  // toDims(op.dimensions);
                size_t out_depth = dims_ohwi[0];
                size_t in_depth = dims_ohwi[3];
                size_t height = dims_ohwi[1];
                size_t width = dims_ohwi[2];
                size_t offset = 0;  // blob->size() == o*i*h*w and simlar to nchw memory layout
                const float *inputFilter = reinterpret_cast<const float *>(buf);  // OHWI memory layout

                // convert OHWI -> OIHW

                // for depth conv need reorder as IOHW since for tflite O is always 1 and IE expects
                // reorder to [in_channels, depth_multiplier, filter_height, filter_width]
                for (size_t i = 0; i < in_depth; i++) {
                    for (size_t o = 0; o < out_depth; o++) {
                        for (size_t h = 0; h < height; h++) {
                            for (size_t w = 0; w < width; w++) {
                                size_t offset_ohwi = o * height * width * in_depth + h * width * in_depth +
                                                     w * in_depth + i;  // similar to NHWC memory layout
                                blob->buffer().as<float *>()[offset++] = inputFilter[offset_ohwi];
                                // blob->buffer().as<float*>()[blob->getTensorDesc().offset(offset++)]
                                // = inputFilter[offset_ohwi]; size_t offset_oihw =
                                // o*in_depth*height*width + i*height*width + h*width + w; //similar
                                // to NCHW memory layout blob->buffer().as<float*>()[offset_oihw] =
                                // inputFilter[offset_ohwi]; VLOG(L1, "offset_ohwi= %d offset_oihw=
                                // %d", offset_ohwi, offset_oihw);
                            }
                        }
                    }
                }

                return blob;
            }
        }
    } else if (op.type == V1_3::OperandType::TENSOR_QUANT8_ASYMM) {
        vec<unsigned int> order;
        Layout layout;
        if (op.dimensions.size() == 4) {
            // order = {0,3,1,2};  //nhwc -> nchw
            order = {3, 0, 1, 2};  // IHWO -> OIHW for depth conv
            // layout = Layout::NCHW;
            // layout = Layout::NHWC;
            layout = Layout::OIHW;  // weights layout
        } else if (op.dimensions.size() == 2) {
            order = {0, 1};
            layout = Layout::NC;
        } else {
            order = {0};  //(op.dimensions.size() < 2)
            layout = Layout::C;
        }
        auto inputDims = toDims(op.dimensions);
        TensorDesc td(InferenceEngine::Precision::FP32, permuteDims(inputDims, order), layout);
        // TensorDesc td(InferenceEngine::Precision::FP32, toDims(op.dimensions), layout);
        if (buf == nullptr) {
            VLOG(L1, "TENSOR buf is NULL !!!!!!!!!!!!!!!");
            InferenceEngine::TBlob<uint8_t>::Ptr blob = std::make_shared<InferenceEngine::TBlob<uint8_t>>(td);
            blob->allocate();
            return blob;
        } else {
            if (inputDims.size() != 4) {
                InferenceEngine::TBlob<uint8_t>::Ptr blob =
                    std::make_shared<InferenceEngine::TBlob<uint8_t>>(td, (uint8_t *)buf, len);
                return blob;
            } else {
                InferenceEngine::TBlob<uint8_t>::Ptr blob = std::make_shared<InferenceEngine::TBlob<uint8_t>>(td);
                blob->allocate();

                auto dims_ohwi = toDims(op.dimensions);
                // auto dims_ohwi = inputDims;  // toDims(op.dimensions);
                size_t out_depth = dims_ohwi[0];
                size_t in_depth = dims_ohwi[3];
                size_t height = dims_ohwi[1];
                size_t width = dims_ohwi[2];
                size_t offset = 0;  // blob->size() == o*i*h*w and simlar to nchw memory layout
                const uint8_t *inputFilter = reinterpret_cast<const uint8_t *>(buf);  // OHWI memory layout

                // convert OHWI -> OIHW

                // for depth conv need reorder as IOHW since for tflite O is always 1 and IE expects
                // reorder to [in_channels, depth_multiplier, filter_height, filter_width]
                for (size_t i = 0; i < in_depth; i++) {
                    for (size_t o = 0; o < out_depth; o++) {
                        for (size_t h = 0; h < height; h++) {
                            for (size_t w = 0; w < width; w++) {
                                size_t offset_ohwi = o * height * width * in_depth + h * width * in_depth +
                                                     w * in_depth + i;  // similar to NHWC memory layout
                                blob->buffer().as<uint8_t *>()[offset++] = inputFilter[offset_ohwi];
                            }
                        }
                    }
                }

                return blob;
            }
        }

    } else if (op.type == V1_3::OperandType::TENSOR_INT32) {
        // VLOG(L1, "check if const tensors of type IN32 supported");
        TensorDesc td(InferenceEngine::Precision::I32, toDims(op.dimensions), Layout::ANY);
        if (buf == nullptr) {
            VLOG(L1, "TENSOR_INT32 buf is NULL !!!!!!!!!!!!!!!");
            InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
            blob->allocate();
            return blob;
        } else {
            InferenceEngine::TBlob<float>::Ptr blob =
                std::make_shared<InferenceEngine::TBlob<float>>(td, (float *)buf, len);
            return blob;
        }
    } else {
        VLOG(L1, "not supporting const tensors of type(%d) ", op.type);
        nnAssert(false);
    }
    return nullptr;
}

IRBlob::Ptr IRConverter::GetConstOperandAsTensor(uint32_t index, bool toNCHW) {
    dumpOperand(index);
    const auto op = mModel.main.operands[index];
    uint32_t len;
    const uint8_t *buf = GetOperandMemory(mModel, index, len);
    VLOG(L3, "IRConverter:: Operand: index: %d, len: %d, buf: %p", index, len, buf);
    if (op.type == V1_3::OperandType::TENSOR_FLOAT32 || op.type == V1_3::OperandType::FLOAT32) {
        vec<unsigned int> order;
        Layout layout;
        if (op.dimensions.size() == 4) {
            if (toNCHW) {
                order = {0, 3, 1, 2};  // nhwc -> nchw
            } else {
                order = {0, 1, 2, 3};  // no need to permute
            }
            // layout = Layout::NCHW;
            // layout = Layout::NHWC;
            layout = Layout::OIHW;  // weights layout
        } else if (op.dimensions.size() == 2) {
            order = {0, 1};
            layout = Layout::NC;
        } else {
            order = {0};  //(op.dimensions.size() < 2)
            layout = Layout::C;
        }
        auto inputDims = toDims(op.dimensions);
        TensorDesc td(InferenceEngine::Precision::FP32, permuteDims(inputDims, order), layout);
        // TensorDesc td(InferenceEngine::Precision::FP32, toDims(op.dimensions), layout);
        if (buf == nullptr) {
            VLOG(L1, "TENSOR_FLOAT32 buf is NULL !!!!!!!!!!!!!!!");
            InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
            blob->allocate();
            return blob;
        } else {
            if ((inputDims.size() != 4) || (toNCHW == false)) {
                InferenceEngine::TBlob<float>::Ptr blob =
                    std::make_shared<InferenceEngine::TBlob<float>>(td, (float *)buf, len);
                return blob;
            } else {
                InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
                blob->allocate();

                // auto dims_ohwi = inputDims;  // toDims(op.dimensions);
                auto dims_ohwi = toDims(op.dimensions);
                size_t out_depth = dims_ohwi[0];
                size_t in_depth = dims_ohwi[3];
                size_t height = dims_ohwi[1];
                size_t width = dims_ohwi[2];
                size_t offset = 0;  // blob->size() == o*i*h*w and simlar to nchw memory layout
                const float *inputFilter = reinterpret_cast<const float *>(buf);  // OHWI memory layout
                for (size_t o = 0; o < out_depth; o++) {
                    for (size_t i = 0; i < in_depth; i++) {
                        for (size_t h = 0; h < height; h++) {
                            for (size_t w = 0; w < width; w++) {
                                size_t offset_ohwi = o * height * width * in_depth + h * width * in_depth +
                                                     w * in_depth + i;  // similar to NHWC memory layout
                                // blob->buffer().as<float*>()[blob->getTensorDesc().offset(offset++)]
                                // = inputFilter[offset_ohwi];
                                blob->buffer().as<float *>()[offset++] = inputFilter[offset_ohwi];
                                // size_t offset_oihw = o*in_depth*height*width + i*height*width +
                                // h*width + w; //similar to NCHW memory layout
                                // blob->buffer().as<float*>()[offset_oihw] =
                                // inputFilter[offset_ohwi]; VLOG(L1, "offset_ohwi= %d offset_oihw=
                                // %d", offset_ohwi, offset_oihw);
                            }
                        }
                    }
                }

                return blob;
            }
        }
    } else if (op.type == V1_3::OperandType::TENSOR_QUANT8_ASYMM) {
        vec<unsigned int> order;
        Layout layout;
        if (op.dimensions.size() == 4) {
            if (toNCHW) {
                order = {0, 3, 1, 2};  // nhwc -> nchw
            } else {
                order = {0, 1, 2, 3};  // no need to permute
            }
            // layout = Layout::NCHW;
            // layout = Layout::NHWC;
            layout = Layout::OIHW;  // weights layout
        } else if (op.dimensions.size() == 2) {
            order = {0, 1};
            layout = Layout::NC;
        } else {
            order = {0};  //(op.dimensions.size() < 2)
            layout = Layout::C;
        }
        auto inputDims = toDims(op.dimensions);
        TensorDesc td(InferenceEngine::Precision::FP32, permuteDims(inputDims, order), layout);
        // TensorDesc td(InferenceEngine::Precision::FP32, toDims(op.dimensions), layout);
        if (buf == nullptr) {
            VLOG(L1, "TENSOR buf is NULL !!!!!!!!!!!!!!!");
            InferenceEngine::TBlob<uint8_t>::Ptr blob = std::make_shared<InferenceEngine::TBlob<uint8_t>>(td);
            blob->allocate();
            return blob;
        } else {
            if ((inputDims.size() != 4) || (toNCHW == false)) {
                InferenceEngine::TBlob<uint8_t>::Ptr blob =
                    std::make_shared<InferenceEngine::TBlob<uint8_t>>(td, (uint8_t *)buf, len);
                return blob;
            } else {
                InferenceEngine::TBlob<uint8_t>::Ptr blob = std::make_shared<InferenceEngine::TBlob<uint8_t>>(td);
                blob->allocate();

                // auto dims_ohwi = inputDims;  // toDims(op.dimensions);
                auto dims_ohwi = toDims(op.dimensions);
                size_t out_depth = dims_ohwi[0];
                size_t in_depth = dims_ohwi[3];
                size_t height = dims_ohwi[1];
                size_t width = dims_ohwi[2];
                size_t offset = 0;  // blob->size() == o*i*h*w and simlar to nchw memory layout
                const uint8_t *inputFilter = reinterpret_cast<const uint8_t *>(buf);  // OHWI memory layout
                for (size_t o = 0; o < out_depth; o++) {
                    for (size_t i = 0; i < in_depth; i++) {
                        for (size_t h = 0; h < height; h++) {
                            for (size_t w = 0; w < width; w++) {
                                size_t offset_ohwi = o * height * width * in_depth + h * width * in_depth +
                                                     w * in_depth + i;  // similar to NHWC memory layout
                                blob->buffer().as<uint8_t *>()[offset++] = inputFilter[offset_ohwi];
                            }
                        }
                    }
                }

                return blob;
            }
        }
    } else if (op.type == V1_3::OperandType::TENSOR_INT32) {
        // VLOG(L1, "check if const tensors of type IN32 supported");
        TensorDesc td(InferenceEngine::Precision::I32, toDims(op.dimensions), Layout::ANY);
        if (buf == nullptr) {
            VLOG(L1, "TENSOR_INT32 buf is NULL !!!!!!!!!!!!!!!");
            InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
            blob->allocate();
            return blob;
        } else {
            InferenceEngine::TBlob<float>::Ptr blob =
                std::make_shared<InferenceEngine::TBlob<float>>(td, (float *)buf, len);
            return blob;
        }
    } else {
        VLOG(L1, "not supporting const tensors of type(%d) ", op.type);
        nnAssert(false);
    }
    return nullptr;
}

Blob::Ptr IRConverter::GetInOutOperandAsBlob(RunTimeOperandInfo &op, const uint8_t *buf, uint32_t &len) {
    if (op.type == V1_3::OperandType::TENSOR_FLOAT32 || op.type == V1_3::OperandType::FLOAT32) {
        if (op.lifetime == V1_3::OperandLifeTime::SUBGRAPH_INPUT) {
            VLOG(L2, "Create input blob !!!!");
            vec<unsigned int> order;
            Layout layout;
            if (op.dimensions.size() == 4) {
                order = {0, 3, 1, 2};  // nhwc -> nchw
                layout = Layout::NCHW;
                // layout = Layout::NHWC;
            } else if (op.dimensions.size() == 2) {
                order = {0, 1};
                layout = Layout::NC;
            } else {
                order = {0};  //(op.dimensions.size() < 2)
                layout = Layout::C;
            }

            auto inputDims = toDims(op.dimensions);
            TensorDesc td(InferenceEngine::Precision::FP32, permuteDims(inputDims, order), layout);
            // TensorDesc td(InferenceEngine::Precision::FP32, inputDims, layout);

            if (buf == nullptr) {
                VLOG(L1, "SUBGRAPH_INPUT buf is NULL !!!!!!!!!!!!!!!");
                InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
                blob->allocate();
                return blob;
            } else {
                if (inputDims.size() != 4) {
                    InferenceEngine::TBlob<float>::Ptr blob =
                        std::make_shared<InferenceEngine::TBlob<float>>(td, (float *)buf, len);
                    return blob;
                } else {
                    InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
                    blob->allocate();

                    auto dims_nhwc = inputDims;  // toDims(op.dimensions);
                    size_t batch = dims_nhwc[0];
                    size_t in_depth = dims_nhwc[3];  // channels
                    size_t height = dims_nhwc[1];
                    size_t width = dims_nhwc[2];
                    size_t offset = 0;  // blob->size() == o*i*h*w and simlar to nchw memory layout
                    const float *input = reinterpret_cast<const float *>(buf);  // OHWI memory layout

                    // convert NHWC -> NCHW

                    for (size_t b = 0; b < batch; b++) {
                        for (size_t i = 0; i < in_depth; i++) {
                            for (size_t h = 0; h < height; h++) {
                                for (size_t w = 0; w < width; w++) {
                                    size_t offset_nhwc = b * height * width * in_depth + h * width * in_depth +
                                                         w * in_depth + i;  // similar to NHWC memory layout
                                    blob->buffer().as<float *>()[offset++] = input[offset_nhwc];
                                    // blob->buffer().as<float*>()[blob->getTensorDesc().offset(offset++)]
                                    // = input[offset_nhwc]; size_t offset_nchw =
                                    // b*in_depth*height*width + i*height*width + h*width + w;
                                    // //similar to NCHW memory layout
                                    // blob->buffer().as<float*>()[offset_oihw] =
                                    // inputFilter[offset_ohwi]; VLOG(L1, "offset_nhwc= %d
                                    // offset_nchw= %d", offset_nhwc, offset_nchw);
                                }
                            }
                        }
                    }

                    return blob;
                }
            }
        } else if (op.lifetime == V1_3::OperandLifeTime::SUBGRAPH_OUTPUT) {
            VLOG(L2, "Create output blob !!!!");
            vec<unsigned int> order;
            Layout layout;
            if (op.dimensions.size() == 4) {
                // order = {0,3,1,2};  //nhwc -> nchw
                layout = Layout::NHWC;
            } else if (op.dimensions.size() == 2) {
                // order = {0, 1};
                layout = Layout::NC;
            } else {
                // order = {0}; //(op.dimensions.size() < 2)
                layout = Layout::C;
            }

            TensorDesc td(InferenceEngine::Precision::FP32, toDims(op.dimensions), layout);  // nhwc
            if (buf == nullptr) {
                VLOG(L1, "SUBGRAPH_OUTPUT buf is NULL !!!!!!!!!!!!!!!");
                InferenceEngine::TBlob<float>::Ptr blob = std::make_shared<InferenceEngine::TBlob<float>>(td);
                blob->allocate();
                return blob;
            } else {
                InferenceEngine::TBlob<float>::Ptr blob =
                    InferenceEngine::make_shared_blob<float>(td, (float *)buf, len);
                return blob;
            }
        }
    } else if (op.type == V1_3::OperandType::TENSOR_INT32) {
        VLOG(L1, "check if const tensors of type IN32 supported");
        // nnAssert(true);
        TensorDesc td(InferenceEngine::Precision::I32, toDims(op.dimensions), Layout::ANY);
        return std::make_shared<InferenceEngine::TBlob<int32_t>>(td, (int32_t *)buf, len);
    } else {
        VLOG(L1, "not supporting const tensors of type(%d) ", op.type);
        nnAssert(false);
    }
    return nullptr;
}

void IRConverter::updateModelLayout(Layout layout) {
    if (mModelLayout == Layout::ANY) {
        mModelLayout = layout;
    } else {
        nnAssert(mModelLayout == layout);
    }
}

bool IRConverter::isNCHWLayoutSupportLayer(const V1_3::Operation &operation) {
    bool isNCHW = false;
    switch (operation.type) {
    case V1_3::OperationType::CONV_2D:
        if (isExplicitPadding(operation) == true) {  // explicit
            if (operation.inputs.size() > 10) {
                isNCHW = PARAM_BOOL(10);
            }
        } else {  // implicit
            if (operation.inputs.size() > 7) {
                isNCHW = PARAM_BOOL(7);
            }
        }
        updateModelLayout(isNCHW ? Layout::NCHW : Layout::NHWC);
        break;
    case V1_3::OperationType::DEPTHWISE_CONV_2D:
        if (isExplicitPadding(operation) == true) {  // explicit
            if (operation.inputs.size() > 11) {
                isNCHW = PARAM_BOOL(11);
            }
        } else {  // implicit
            if (operation.inputs.size() > 8) {
                isNCHW = PARAM_BOOL(8);
            }
        }
        updateModelLayout(isNCHW ? Layout::NCHW : Layout::NHWC);
        break;
    case V1_3::OperationType::MAX_POOL_2D:
        if (isExplicitPadding(operation) == true) {  // explicit
            if (operation.inputs.size() > 10) {
                isNCHW = PARAM_BOOL(10);
            }
        } else {  // implicit
            if (operation.inputs.size() > 7) {
                isNCHW = PARAM_BOOL(7);
            }
        }
        updateModelLayout(isNCHW ? Layout::NCHW : Layout::NHWC);
        break;
    case V1_3::OperationType::AVERAGE_POOL_2D:
        if (isExplicitPadding(operation) == true) {  // explicit
            if (operation.inputs.size() > 10) {
                isNCHW = PARAM_BOOL(10);
            }
        } else {  // implicit
            if (operation.inputs.size() > 7) {
                isNCHW = PARAM_BOOL(7);
            }
        }
        updateModelLayout(isNCHW ? Layout::NCHW : Layout::NHWC);
        break;
    default: break;
    }
    return isNCHW;
}

bool IRConverter::isExplicitPadding(const V1_3::Operation &operation) {
    bool isExplicit = false;

    switch (operation.type) {
    case V1_3::OperationType::CONV_2D:
        if (operation.inputs.size() > 7) {
            if (mModel.main.operands[operation.inputs[7]].type != V1_3::OperandType::BOOL) {
                isExplicit = true;
            }
        }
        break;
    case V1_3::OperationType::DEPTHWISE_CONV_2D:
        if (operation.inputs.size() > 8) {
            if (mModel.main.operands[operation.inputs[8]].type != V1_3::OperandType::BOOL) {
                isExplicit = true;
            }
        }
        break;
    case V1_3::OperationType::MAX_POOL_2D:
        if (operation.inputs.size() > 7) {
            if (mModel.main.operands[operation.inputs[7]].type != V1_3::OperandType::BOOL) {
                isExplicit = true;
            }
        }
        break;
    case V1_3::OperationType::AVERAGE_POOL_2D:
        if (operation.inputs.size() > 7) {
            if (mModel.main.operands[operation.inputs[7]].type != V1_3::OperandType::BOOL) {
                isExplicit = true;
            }
        }
        break;
    default: break;
    }
    return isExplicit;
}

std::string IRConverter::getIRXml() {
    std::stringstream sXml;
    size_t binsize = 0;
    mNet.save(sXml, binsize);
    return sXml.str();
}

BlobPair IRConverter::getShuffledBlobs() { return mNet.saveShuffledBlobs(); }

bool IRConverter::isSupportedModelByNPUC(const V1_3::Model &model) {
    VLOG(L2, "%s(+)\n", __func__);
    bool ret = true;

    // Only for IV3
    int32_t bitSum = 0;
    for (const V1_3::Operation &androidOperation : model.main.operations) {
        switch (androidOperation.type) {
        case V1_3::OperationType::CONV_2D: bitSum |= 1; break;
        case V1_3::OperationType::MAX_POOL_2D: bitSum |= 1 << 1; break;
        case V1_3::OperationType::AVERAGE_POOL_2D: bitSum |= 1 << 2; break;
        case V1_3::OperationType::RESHAPE: bitSum |= 1 << 3; break;
        case V1_3::OperationType::CONCATENATION: bitSum |= 1 << 4; break;
        case V1_3::OperationType::SOFTMAX: bitSum |= 1 << 5; break;
        default: ret = false; break;
        }
        if (ret == false)
            break;
    }

    if (ret == true) {
        switch (bitSum) {
        case 0x1F:  // 0b 0001 1111
        case 0x3F:  // 0b 0011 1111
            // supportedOperations[ANEURALNETWORKS_CONV_2D] = true;
            // supportedOperations[ANEURALNETWORKS_MAX_POOL_2D] = true;
            // supportedOperations[ANEURALNETWORKS_AVERAGE_POOL_2D] = true;
            // supportedOperations[ANEURALNETWORKS_RESHAPE] = true;
            // supportedOperations[ANEURALNETWORKS_CONCATENATION] = true;
            // ANEURALNETWORKS_SOFTMAX is not operated by NPU
            ret = true;
            break;
        default: ret = false; break;
        }
    } else {
        ret = false;
    }

    VLOG(L2, "%s() ret = %d, bitSum = 0x%x\n", __func__, ret, bitSum);
    VLOG(L2, "%s(-)\n", __func__);
    return ret;
}

bool IRConverter::isSupportedModelByDSPC(const V1_3::Model &model) {
    VLOG(L2, "%s(+)\n", __func__);
    bool ret = false;
    size_t opcnt = model.main.operations.size();
    if ((opcnt == 65) || (opcnt == 183)) {
        std::string opseq = "";
        for (size_t idx = 0; idx < opcnt; idx++) {
            switch (model.main.operations[idx].type) {
            case V1_3::OperationType::CONV_2D: opseq += "C"; break;
            case V1_3::OperationType::DEPTHWISE_CONV_2D: opseq += "D"; break;
            case V1_3::OperationType::ADD: opseq += "E"; break;
            case V1_3::OperationType::AVERAGE_POOL_2D: opseq += "A"; break;
            case V1_3::OperationType::RESHAPE: opseq += "R"; break;
            case V1_3::OperationType::MAX_POOL_2D: opseq += "M"; break;
            case V1_3::OperationType::CONCATENATION: opseq += "T"; break;
            case V1_3::OperationType::FULLY_CONNECTED: opseq += "F"; break;
            default: idx = opcnt; break;
            }
        }
        const std::string opseq_mv2 = "CDCCDCCDCECDCCDCECDCECDCCDCECDCECDCECDCCDCECDCECDCCDCECDCECDCCACR";
        if (opseq == opseq_mv2) {
            ret = true;
            VLOG(L1, "%s() model = MV2", __func__);
        }
        // const std::string opseq_incresnet =
        //     "CCCMCCCCCCCCCTCECCCCCCTCECCCCCCTCECCCCCCTCECCCCCCTCEMCCCCTCCCCTCECCCCTCECCCCTCECCCCTCECCCCTCECCCCTCECCCCTC"
        //     "ECCCCTCECCCCTCECCCCTCEMCCCCCCCTCCCCTCECCCCTCECCCCTCECCCCTCECCCCTCECCCCTCEAFRR";
        // else if (opseq == opseq_incresnet) {
        //     ret = true;
        //     VLOG(L1, "%s() model = INCRESNET", __func__);
        // }
    }
    VLOG(L2, "%s() ret = %c\n", __func__, ret ? 'Y' : 'N');
    VLOG(L2, "%s(-)\n", __func__);
    return ret;
}

}  // namespace ofi
}  // namespace eden_driver
}  // namespace nn
}  // namespace android
