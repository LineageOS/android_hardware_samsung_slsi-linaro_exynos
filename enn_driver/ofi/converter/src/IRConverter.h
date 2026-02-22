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

#ifndef ANDROID_ML_NN_IRCONVERTER_H
#define ANDROID_ML_NN_IRCONVERTER_H

#include <android/hidl/memory/1.0/IMemory.h>
#include <hidlmemory/mapping.h>
#include <hardware/hardware.h>
#include <sys/mman.h>
#include <string>
#include <fstream>
#include <HalInterfaces.h>
// #include "NeuralNetworks.h"

// #include "IENetwork.h"

#include "IRDocument.h"
#include "IRLayers.h"

using ::android::hidl::memory::V1_0::IMemory;
using namespace IRBuilder;
using namespace InferenceEngine;

namespace android {
namespace nn {
namespace eden_driver {
namespace ofi {

template <class T> using vec = std::vector<T>;
typedef uint8_t *memory;

// The type and dimensions of an operand.
struct Shape {
    V1_3::OperandType type;
    std::vector<uint32_t> dimensions;
    float scale;
    int32_t offset;
};

// Information we maintain about each operand during execution that
// may change during execution.
struct RunTimeOperandInfo {
    // std::string name;
    // uint32_t opIdx;
    // void * opIdx;

    // TODO Storing the type here is redundant, as it won't change during execution.
    V1_3::OperandType type;
    // The type and dimensions of the operand.  The dimensions can
    // change at runtime.  We include the type because it's useful
    // to pass together with the dimension to the functions implementing
    // the operators.
    std::vector<uint32_t> dimensions;

    float scale;
    int32_t zeroPoint;
    // Where the operand's data is stored.  Check the corresponding
    // location information in the model to figure out if this points
    // to memory we have allocated for an temporary operand.
    uint8_t *buffer;
    // The length of the buffer.
    uint32_t length;
    // Whether this is a temporary variable, a model input, a constant, etc.
    V1_3::OperandLifeTime lifetime;
    // Keeps track of how many operations have yet to make use
    // of this temporary variable.  When the count is decremented to 0,
    // we free the buffer.  For non-temporary variables, this count is
    // always 0.
    uint32_t numberOfUsesLeft;

    Shape shape() const { return Shape{.type = type, .dimensions = dimensions, .scale = scale, .offset = zeroPoint}; }
};

// Used to keep a pointer to each of the memory pools.
struct RunTimePoolInfo {
    sp<IMemory> memory;
    hidl_memory hidlMemory;
    uint8_t *buffer;

    bool set(const hidl_memory &hidlMemory);
    bool update();
};

bool setRunTimePoolInfosFromHidlMemories(std::vector<RunTimePoolInfo> *poolInfos, const hidl_vec<hidl_memory> &pools);

// Base class used to create vpu drivers for the NN HAL.  This class
// provides some implementation of the more common functions.
//
// Since these drivers simulate hardware, they must run the computations
// on the CPU.  An actual driver would not do that.
class IRConverter {
  public:
    IRConverter(const V1_3::Model &model) :
        mTargetDevice(TargetDevice::eCPU), mModel(model), mNet("nnNet"),
        mBuilderCtx({0, InferenceEngine::Precision::FP32}) {}

    ~IRConverter() { deinitialize(); }
    bool parse();
    IRDocument &getIRDoc() { return mNet; }
    bool isOperationSupported(const V1_3::Operation &operation, const V1_3::Model &model);
    std::string getIRXml();
    BlobPair getShuffledBlobs();
    std::string getError() { return mErrorMessage; }

  protected:
    void deinitialize();
    bool initializeRunTimeOperandInfo();

    bool operationAdd(const V1_3::Operation &operation);
    bool operationAveragePool2D(const V1_3::Operation &operation);
    bool operationConCat(const V1_3::Operation &operation);
    bool operationConv2D(const V1_3::Operation &operation);
    bool operationDepthwiseConv2D(const V1_3::Operation &operation);
    bool operationFullyConnected(const V1_3::Operation &operation);
    bool operationL2Normalization(const V1_3::Operation &operation);
    bool operationLRN(const V1_3::Operation &operation);
    bool operationMaxPool2D(const V1_3::Operation &operation);
    bool operationLogisticSigmoid(const V1_3::Operation &operation);
    // bool operationLSTM(const hav::V1_3::Operation& operation);
    bool operationMUL(const V1_3::Operation &operation);
    bool operationRELU(const V1_3::Operation &operation);
    bool operationRELU1(const V1_3::Operation &operation);
    bool operationRELU6(const V1_3::Operation &operation);
    bool operationReshape(const V1_3::Operation &operation);
    bool operationSoftmax(const V1_3::Operation &operation);
    bool operationTANH(const V1_3::Operation &operation);

    void initializeInput();
    void finalizeOutput(/*RunTimeOperandInfo* output*/);

    OutputPort handleFusion(const OutputPort &out, int32_t fusedOp, QuantizationParams &act_qtprms);
    template <typename T> T GetConstFromBuffer(const uint8_t *buf, uint32_t len);
    template <typename T> std::vector<T> GetConstVecFromBuffer(const uint8_t *buf, uint32_t len);
    const uint8_t *GetOperandMemory(const V1_3::Model &model, uint32_t index, uint32_t &len_out);
    template <typename T> T ParseOperationInput(const V1_3::Model &model, const V1_3::Operation &operation, uint32_t index);
    template <typename T> T GetConstOperand(const V1_3::Model &model, uint32_t index);
    template <typename T> std::vector<T> GetConstVecOperand(const V1_3::Model &model, uint32_t index);
    Blob::Ptr GetConstOperandAsTensor(uint32_t index, bool toNCHW = true);
    Blob::Ptr GetInOutOperandAsBlob(RunTimeOperandInfo &op, const uint8_t *buf, uint32_t &len);
    Blob::Ptr GetConstWeightsOperandAsTensor(uint32_t index);
    void SetOperandMemory(const V1_3::Model &model, uint32_t index, uint32_t &len_out, const uint8_t *buf);
    void SetOperandFromTensor(uint8_t *buf, uint32_t &length, Blob::Ptr infOutput);
    bool isConst(int index);
    bool isQ8A(int index);
    OutputPort getPort(int index, bool toNCHW = true);
    bool isNCHWLayoutSupportLayer(const V1_3::Operation &operation);
    bool isExplicitPadding(const V1_3::Operation &operation);
    bool isSupportedModelByNPUC(const V1_3::Model &model);
    bool isSupportedModelByDSPC(const V1_3::Model &model);

    TargetDevice mTargetDevice;
    V1_3::Model mModel;
    std::vector<RunTimeOperandInfo> mOperands;
    std::vector<RunTimePoolInfo> mPoolInfos;
    IRDocument mNet;
    std::vector<OutputPort> mPorts;  // typedef std::shared_ptr<Data> DataPtr;

    // debug to find out layout combinated model.
    // enum class Layout { UNKNOWN, NHWC, NCHW };
    Layout mModelLayout = Layout::ANY;
    void updateModelLayout(Layout layout);
    BuilderContext mBuilderCtx;
    std::string mErrorMessage;
};

}  // namespace ofi
}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // ANDROID_ML_NN_IRCONVERTER_H
