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
 * @file    NNNetworkBuilder.cpp
 * @brief   This is NNNetworkBuilder class file.
 * @details This header defines NNNetworkBuilder class.
 *          This class is creating NNNetwork for npuc from android nn.
 * @author  hexiang.ji (hexiang.ji@samsung.com)
 *          minsu.jeon (minsu.jeon@samsung.com)
 */

#include <string>
#include <vector>
#include <memory>

#include <sys/mman.h>  // mmap

#include "log.h"

#include "ActivationFunctor.h"  // kActivationNone, kActivationRelu, kActivationTanh etc

#include "Common.h"
#include "EdenModelConvertLib.h"

#include "NNNetworkBuilder.h"
#include "./npuc/include/npuc/CompileOptions.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::NNNetworkBuilder"

namespace android {
namespace nn {
namespace eden_driver {

static void releaseConfigOperand(EdenOperand*& configOperand);

enum PaddingScheme {
    kPaddingUnknown = 0,
    kPaddingSame = 1,
    kPaddingValid = 2,
    kPaddingExplicit = 3,
};

template <typename T>
T getPtr(const V1_3::Model& model, const V1_3::Operation& androidOperation, const int op_id, ModelInfo& modelInfo) {
    V1_3::Operand operand = model.main.operands[androidOperation.inputs[op_id]];
    T data = nullptr;
    if (operand.lifetime == V1_3::OperandLifeTime::CONSTANT_COPY) {
        data = reinterpret_cast<T>(modelInfo.operandValues.get() + operand.location.offset);
        LOGD(EDEN_DRIVER, "getPtr) op_id=%d, operandlifetime == CONSTANT_COPY, addr=%p\n", op_id, data);
    } else if (operand.lifetime == V1_3::OperandLifeTime::CONSTANT_REFERENCE) {
        // TODO multiple pools. currently size==1
        for (auto& virtualAddressOnPools : modelInfo.vecVirtualAddressOnPools) {
            LOGD(EDEN_DRIVER, "getPtr) type:%d\n", virtualAddressOnPools.type);
            LOGD(EDEN_DRIVER, "getPtr) addr:%p\n", virtualAddressOnPools.addr);
            LOGD(EDEN_DRIVER, "getPtr) size:%d\n", virtualAddressOnPools.size);
            char* mappedPtr = virtualAddressOnPools.addr;
            data = reinterpret_cast<T>(mappedPtr + operand.location.offset);
        }
        LOGD(EDEN_DRIVER, "getPtr) op_id=%d, operandlifetime == CONSTANT_REFERENCE, addr=%p, offset=%d, length=%d\n", op_id, data, operand.location.offset, operand.location.length);
    } else {
        LOGE(EDEN_DRIVER, "getPtr) op_id=%d, operandliftime == %d, Using Internal buffer only", op_id, static_cast<int32_t>(operand.lifetime));
        return nullptr;
    }
    return data;
}

static void releaseConfigOperand(EdenOperand*& configOperand) {
    delete[] static_cast<uint32_t*>(configOperand->buffer->addr);
    delete configOperand->buffer;
    delete configOperand;
    configOperand = nullptr;
}

NNNetworkBuilder::NNNetworkBuilder() {
}

std::shared_ptr<NPUC::NNNetwork> NNNetworkBuilder::buildNNNetwork(const V1_3::Model& model, ModelInfo& modelInfo, const std::vector<int32_t>& operationList) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    _dummyInputCount = 0; // initialize
    NPUC::CompileOptions *options = NPUC::CompileOptions::GetInstance();
    options->setDataFormat(NPUC::DataFormat::NCHW);

    LOGD(EDEN_DRIVER, "OperationList size : %zu\n", operationList.size());

    // if (operationList.size() > 1) {
    //     LOGE(EDEN_DRIVER, "Oops, currently only 1 operation is supported by NPU...");
    //     return nullptr;
    // }

    ////////////////////////////////////
    // parsing  code
    ////////////////////////////////////
    std::shared_ptr<NPUC::NNNetwork> nnNetwork = std::make_shared<NPUC::NNNetwork>();

    for (size_t idx = 0; idx < operationList.size(); idx++) {
        int32_t operationIdx = operationList[idx];
        int32_t opType = static_cast<int32_t>(model.main.operations[operationIdx].type);
        LOGD(EDEN_DRIVER, "op type: %d\n", opType);

        LOGD(EDEN_DRIVER, "=============================================================operationIdx : %d\n", operationIdx);
        LOGD(EDEN_DRIVER, "=============================================================idx : %zu\n", idx);

        switch (opType) {
            case EDEN_OP_CONV_2D:
            {
                LOGD(EDEN_DRIVER, "operator type is conv2d\n");

                V1_3::Operation operation = model.main.operations[operationIdx];
                NPUC::NNConvLayer *convLayer = getConvolutionLayer(operationIdx, model, operation, modelInfo);
                if (convLayer == nullptr) {
                    LOGE(EDEN_DRIVER, "NNConvLayer is nullptr\n");
                    return nullptr;
                }

                for (int32_t inputIndex : model.main.inputIndexes) {
                    for(int32_t opInputIndex : model.main.operations[operationIdx].inputs) {
                        if (inputIndex == opInputIndex) {
                            LOGD(EDEN_DRIVER, "Make input layers of %d\n", opType);
                            std::vector<int> inputShape = convLayer->getInputShape(0);
                            // in case one input has multiple consumers
                            if(model.main.inputIndexes.size() == 1 && _dummyInputCount >= 1) {
                                convLayer->removeInput(convLayer->getInputs()[0]);
                                convLayer->insertInput("data0", inputShape, 0);
                            } else { // in case an input has one consumer
                                NPUC::NNLayer* inputLayer = createDummyInputLayer(inputShape, opInputIndex);
                                nnNetwork->appendLayers(inputLayer);
                                // update the connection between the input layer and conv layer
                                convLayer->removeInput(convLayer->getInputs()[0]);
                                convLayer->insertInput(inputLayer->getName(), inputLayer->getInputShape(0), 0);
                            }
                        }
                    }
                }
                int32_t ret = nnNetwork->appendLayers(convLayer);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "Fail to append layer becuase there are duplicate layers.\n");
                    return nullptr;
                }
                break;
            }
            case EDEN_OP_AVERAGE_POOL_2D:
            {
                LOGD(EDEN_DRIVER, "operator type is average_pool_2d\n");

                V1_3::Operation operation = model.main.operations[operationIdx];
                NPUC::NNPoolLayer *averagePoolLayer = getPoolLayer(operationIdx, model, operation, &modelInfo, "AVE");
                if (averagePoolLayer == nullptr) {
                    LOGE(EDEN_DRIVER, "NNPoolLayer is nullptr\n");
                    return nullptr;
                }

                for (int32_t inputIndex : model.main.inputIndexes) {
                    for(int32_t opInputIndex : model.main.operations[operationIdx].inputs) {
                        if (inputIndex == opInputIndex) {
                            LOGD(EDEN_DRIVER, "Make input layers of %d\n", opType);
                            std::vector<int> inputShape = averagePoolLayer->getInputShape(0);
                            // in case one input has multiple consumers
                            if(model.main.inputIndexes.size() == 1 && _dummyInputCount >= 1) {
                                averagePoolLayer->removeInput(averagePoolLayer->getInputs()[0]);
                                averagePoolLayer->insertInput("data0", inputShape, 0);
                            } else { // in case an input has one consumer
                                NPUC::NNLayer* inputLayer = createDummyInputLayer(inputShape, opInputIndex);
                                nnNetwork->appendLayers(inputLayer);
                                averagePoolLayer->removeInput(averagePoolLayer->getInputs()[0]);
                                averagePoolLayer->insertInput(inputLayer->getName(), inputLayer->getInputShape(0), 0);
                            }
                        }
                    }
                }
                int32_t ret = nnNetwork->appendLayers(averagePoolLayer);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "Fail to append layer becuase there are duplicate layers.\n");
                    return nullptr;
                }
                break;
            }
            case EDEN_OP_MAX_POOL_2D:
            {
                LOGD(EDEN_DRIVER, "operator type is max_pool_2d\n");

                V1_3::Operation operation = model.main.operations[operationIdx];
                NPUC::NNPoolLayer *maxPoolLayer = getPoolLayer(operationIdx, model, operation, &modelInfo, "MAX");
                if (maxPoolLayer == nullptr) {
                    LOGE(EDEN_DRIVER, "NNPoolLayer is nullptr\n");
                    return nullptr;
                }

                for (int32_t inputIndex : model.main.inputIndexes) {
                    for(int32_t opInputIndex : model.main.operations[operationIdx].inputs) {
                        if (inputIndex == opInputIndex) {
                            LOGD(EDEN_DRIVER, "Make input layers of %d\n", opType);
                            LOGD(EDEN_DRIVER, "opInputIndex %d\n", opInputIndex);
                            std::vector<int> inputShape = maxPoolLayer->getInputShape(0);
                            // in case one input has multiple consumers
                            if(model.main.inputIndexes.size() == 1 && _dummyInputCount >= 1) {
                                maxPoolLayer->removeInput(maxPoolLayer->getInputs()[0]);
                                maxPoolLayer->insertInput("data0", inputShape, 0);
                            } else { // in case an input has one consumer
                                NPUC::NNLayer* inputLayer = createDummyInputLayer(inputShape, opInputIndex);
                                nnNetwork->appendLayers(inputLayer);
                                maxPoolLayer->removeInput(maxPoolLayer->getInputs()[0]);
                                maxPoolLayer->insertInput(inputLayer->getName(), inputLayer->getInputShape(0), 0);
                            }
                        }
                    }
                }
                int32_t ret = nnNetwork->appendLayers(maxPoolLayer);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "Fail to append layer becuase there are duplicate layers.\n");
                    return nullptr;
                }
                break;
            }
            case EDEN_OP_RESHAPE:
            {
                LOGD(EDEN_DRIVER, "operator type is reshape\n");

                V1_3::Operation operation = model.main.operations[operationIdx];
                NPUC::NNReshapeLayer *reshapeLayer = getReshapeLayer(operationIdx, model, operation, &modelInfo);
                if (reshapeLayer == nullptr) {
                    LOGE(EDEN_DRIVER, "NNReshapeLayer is nullptr\n");
                    return nullptr;
                }

                for (int32_t inputIndex : model.main.inputIndexes) {
                    for(int32_t opInputIndex : model.main.operations[operationIdx].inputs) {
                        // in case axis is passed as a MODEL_INPUT
                        if (inputIndex == opInputIndex && opInputIndex != 1 ) {
                            LOGD(EDEN_DRIVER, "Make input layers of %d\n", opType);
                            std::vector<int> inputShape = reshapeLayer->getInputShape(0);
                            NPUC::NNLayer* inputLayer = createDummyInputLayer(inputShape, opInputIndex);
                            nnNetwork->appendLayers(inputLayer);
                            reshapeLayer->removeInput(reshapeLayer->getInputs()[0]);
                            reshapeLayer->insertInput(inputLayer->getName(), inputLayer->getInputShape(0), 0);
                        }
                    }
                }
                int32_t ret = nnNetwork->appendLayers(reshapeLayer);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "Fail to append layer becuase there are duplicate layers.\n");
                    return nullptr;
                }
                break;
            }
            case EDEN_OP_CONCATENATION:
            {
                LOGD(EDEN_DRIVER, "operator type is concat\n");
                V1_3::Operation operation = model.main.operations[operationIdx];
                NPUC::NNConcatLayer *concatLayer = getConcatLayer(operationIdx, model, operation, &modelInfo);
                if (concatLayer == nullptr) {
                    LOGE(EDEN_DRIVER, "NNConcatLayer is nullptr\n");
                    return nullptr;
                }
                int32_t multipleInputIndex = 0;
                for (int32_t inputIndex : model.main.inputIndexes) {
                    for(int32_t opInputIndex : model.main.operations[operationIdx].inputs) {
                        if (inputIndex == opInputIndex) {
                            LOGD(EDEN_DRIVER, "===============================================\n");
                            LOGD(EDEN_DRIVER, "Make input layers of %d\n", opType);
                            LOGD(EDEN_DRIVER, "opInputIndex: %d\n", opInputIndex);
                            std::vector<int> inputShape = concatLayer->getInputShape(multipleInputIndex);
                            NPUC::NNLayer* inputLayer = createDummyInputLayer(inputShape, opInputIndex);
                            nnNetwork->appendLayers(inputLayer);

                            concatLayer->removeInput(concatLayer->getInputs()[multipleInputIndex]);
                            concatLayer->insertInput(inputLayer->getName(), inputLayer->getInputShape(0), multipleInputIndex);
                            multipleInputIndex++;
                        }
                    }
                }
                uint32_t ret = nnNetwork->appendLayers(concatLayer);
                if (ret != RET_OK) {
                    LOGE(EDEN_DRIVER, "Fail to append layer becuase there are duplicate layers.\n");
                    return nullptr;
                }
                break;
            }
            default:
            {
                LOGE(EDEN_DRIVER, "not yet supported : %d\n", opType);
                break;
            }
        }
    }
	LOGD(EDEN_DRIVER, "ManipulateNetwork!\n");
    nnNetwork->manipulateNetwork();
    LOGD(EDEN_DRIVER, "ManipulateNetwork is Done!\n");
 
    LOGD(EDEN_DRIVER, "NNNetworkBuilder::buildNNNetwork() is done!\n");
    return nnNetwork;
}

// create the input layer base the shape of first layer
NPUC::NNInputLayer* NNNetworkBuilder::createDummyInputLayer(const std::vector<int> &inputShape, uint32_t layerNameCount) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    // create Input Layer
    std::string layerName = "data" + std::to_string(layerNameCount);
    std::string layerType = "Input";
    std::string layerInput = "ROOT";

    std::vector<std::string> i_inputs;
    std::vector<std::string> i_outputs;

    i_inputs.push_back(layerInput);
    i_outputs.push_back(layerName);

    std::shared_ptr<NPUC::InputAttr> inputAttr = std::make_shared<NPUC::InputAttr>();
    inputAttr->setBatch(inputShape[N_NCHW]);
    inputAttr->setChannel(inputShape[C_NCHW]);
    inputAttr->setHeight(inputShape[H_NCHW]);
    inputAttr->setWidth(inputShape[W_NCHW]);

    NPUC::NNInputLayer* inputLayer = new NPUC::NNInputLayer(layerName,
                                                            layerType,
                                                            i_inputs,
                                                            i_outputs,
                                                            inputAttr);

    std::vector<std::vector<int32_t>> inShape;
    inShape.push_back(inputLayer->getShapeFromName(layerName));
    inputLayer->computeOutputShape(inShape);

    inputLayer->getAttr()->printAttr();
    _dummyInputCount++;
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return inputLayer;
}

// get the inputs of layer
std::shared_ptr<std::vector<std::string>> NNNetworkBuilder::getInputs(const V1_3::Model& model, const V1_3::Operation& operation, int operationIdx, uint32_t numOfInput) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::shared_ptr<std::vector<std::string>> inputsOperandNames = std::make_shared<std::vector<std::string>>();

    LOGD(EDEN_DRIVER, "numOfInput is %d\n", numOfInput);
    for (uint32_t idx = 0; idx < numOfInput; idx++) {
        auto inputOperandId = operation.inputs[idx];
        LOGD(EDEN_DRIVER, "inputOperandId : %d\n", inputOperandId);
        const V1_3::Operand& from = model.main.operands[inputOperandId];
        if (from.dimensions.size() == 0)
            continue;
        if (operationIdx > 0) { // The operation is not the first.
            bool doneInputSetting = false;
            // The operation's input is one of multiple inputs of a model
            LOGD(EDEN_DRIVER, "model.main.inputIndexes.size() : %zu\n", model.main.inputIndexes.size());
            if (model.main.inputIndexes.size() >= 1)
            {
                for (size_t inIdx = 0; inIdx < model.main.inputIndexes.size(); inIdx++) {
                    LOGD(EDEN_DRIVER, "model.main.inputIndexes[inIdx] : %d\n", model.main.inputIndexes[inIdx]);
                    if(model.main.inputIndexes[inIdx] == inputOperandId) {
                        std::string inputLayerName = "data" + std::to_string(inputOperandId);
                        inputsOperandNames.get()->push_back(inputLayerName);
                        LOGD(EDEN_DRIVER, "inputLayerName : %s\n", inputLayerName.c_str());
                        doneInputSetting = true;
                        break;
                    }
                }
            }
            // The operation's input is an output of the previous layer
            if (!doneInputSetting) {
                int32_t numOfInputs = operation.inputs.size();
                if (numOfInputs > 0) {
                    int32_t numOfOperations = model.main.operations.size();
                    for (int32_t operationIdx = 0; operationIdx < numOfOperations; operationIdx++) {
                        auto operation = model.main.operations[operationIdx];
                        int32_t numOfOutputs = operation.outputs.size();
                        for (int outputIdx = 0; outputIdx < numOfOutputs; outputIdx++) {
                            auto outputOperandId = operation.outputs[outputIdx];
                            if (inputOperandId == outputOperandId) {
                                EdenString newName;
                                int32_t opType = static_cast<int32_t>(operation.type);
                                getMatchedEdenOperationName(opType, newName.name, newName.length);
                                std::string inputLayerName((const char*)newName.name);
                                inputLayerName += "_" + std::to_string(operationIdx);
                                inputsOperandNames.get()->push_back(inputLayerName);
                                LOGD(EDEN_DRIVER, "inputLayerName : %s\n", inputLayerName.c_str());
                                releaseName(newName.name);
                            }
                        }
                    }
                }
            }
        } else { // The operation is the first which has an input of a model
            std::string inputLayerName = "data" + std::to_string(inputOperandId);
            inputsOperandNames.get()->push_back(inputLayerName);
            LOGD(EDEN_DRIVER, "the first input operand name: %s\n", inputLayerName.c_str());
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return inputsOperandNames;
}

// get the output of layer
std::shared_ptr<std::vector<std::string>> NNNetworkBuilder::getOutputs(const V1_3::Model& model, const V1_3::Operation& operation) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::shared_ptr<std::vector<std::string>> outputOperandNames = std::make_shared<std::vector<std::string>>();

    int32_t numOfOutputs = operation.outputs.size();
    if (numOfOutputs > 0) {
        for (int outputIdx = 0; outputIdx < numOfOutputs; outputIdx++) {
            auto outputOperandId = operation.outputs[outputIdx];

            int32_t numOfOperations = model.main.operations.size();
            for (int32_t operationIdx = 0; operationIdx < numOfOperations; operationIdx++) {
                auto operation = model.main.operations[operationIdx];
                int32_t numOfInputs = operation.inputs.size();

                for (int inputIdx = 0; inputIdx < numOfInputs; inputIdx++) {
                    auto inputOperandId = operation.inputs[inputIdx];
                    if (inputOperandId == outputOperandId) {
                        EdenString newName;
                        int32_t opType = static_cast<int32_t>(operation.type);
                        getMatchedEdenOperationName(opType, newName.name, newName.length);
                        std::string outputLayerName((const char*)newName.name);
                        outputLayerName += "_" + std::to_string(operationIdx);
                        outputOperandNames.get()->push_back(outputLayerName);
                        LOGD(EDEN_DRIVER, "outputLayerName : %s\n", outputLayerName.c_str());
                        releaseName(newName.name);
                    }
                }
            }
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return outputOperandNames;
}

// get the input shape of layer
std::vector<std::vector<int>> NNNetworkBuilder::getInputShape(const V1_3::Model& model, const V1_3::Operation& operation, uint32_t numOfInputs) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    std::vector<std::vector<int>> shape;
    for (uint32_t idx = 0; idx < numOfInputs; idx++)
    {
        auto operandId = operation.inputs[idx];
        const V1_3::Operand& inputOperand = model.main.operands[operandId];

        int32_t numOfDims = 0;
        std::unique_ptr<int32_t[]> dims = std::make_unique<int32_t[]>(4);
        std::vector<int32_t> vecDims;
        getEdenDimensions(inputOperand.dimensions, dims.get(), numOfDims);
        for (int32_t idx = 0; idx < numOfDims; idx++) {
            vecDims.push_back(dims[idx]);
        }
        shape.push_back(vecDims);

    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return shape;
}

// get the output shape of layer
std::vector<std::vector<int>> NNNetworkBuilder::getOutputShape(const V1_3::Model& model, const V1_3::Operation& operation) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // @todo Only one at 0th input operand is enough?
    auto operandId = operation.outputs[0];
    const V1_3::Operand& outputOperand = model.main.operands[operandId];

    std::vector<std::vector<int>> shape;

    // @todo currently numOfDims is fixed for 4
    int32_t numOfDims;
    std::unique_ptr<int32_t[]> dims = std::make_unique<int32_t[]>(4);
    std::vector<int32_t> vecDims;
    getEdenDimensions(outputOperand.dimensions, dims.get(), numOfDims);
    for (int32_t idx = 0; idx < numOfDims; idx++) {
        vecDims.push_back(dims[idx]);
    }
    shape.push_back(vecDims);


    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return shape;
}

// get the consumer of layer
std::vector<std::string> NNNetworkBuilder::getConsumers(const V1_3::Model& model, const V1_3::Operation& targetOperation) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::vector<std::string> consumers;
    int32_t numOfOutputs = targetOperation.outputs.size();
    if (numOfOutputs > 0) {
        for (int outputIdx = 0; outputIdx < numOfOutputs; outputIdx++) {
            auto outputOperandId = targetOperation.outputs[outputIdx];

            int32_t numOfOperations = model.main.operations.size();
            for (int32_t operationIdx = 0; operationIdx < numOfOperations; operationIdx++) {
                auto operation = model.main.operations[operationIdx];
                int32_t numOfInputs = operation.inputs.size();

                for (int inputIdx = 0; inputIdx < numOfInputs; inputIdx++) {
                    auto inputOperandId = operation.inputs[inputIdx];
                    if (inputOperandId == outputOperandId) {
                        EdenString newName;
                        int32_t opType = static_cast<int32_t>(operation.type);
                        getMatchedEdenOperationName(opType, newName.name, newName.length);
                        std::string consumerLayerName((const char*)newName.name);
                        consumerLayerName += "_" + std::to_string(operationIdx);
                        consumers.push_back(consumerLayerName);
                        LOGD(EDEN_DRIVER, "consumerLayerName : %s\n", consumerLayerName.c_str());
                        releaseName(newName.name);
                    }
                }
            }
        }
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return consumers;
}

// load data from the operand
template <typename T>
std::shared_ptr<std::vector<T>> NNNetworkBuilder::loadData(const V1_3::Model& model, const V1_3::Operation& operation, const V1_3::Operand& operand, int32_t op_id, ModelInfo& modelInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    std::shared_ptr<std::vector<T>> loadedData = std::make_shared<std::vector<T>>();

    // Get address for data to be loaded
    T* srcPtr = getPtr<T*>(model, operation, op_id, modelInfo);

    // Caculate size for data to be loaded
    int32_t totalSize = 1;
    for (size_t idx = 0; idx < operand.dimensions.size(); idx++) {
        LOGD(EDEN_DRIVER, "operand.dimensions[idx]: %d\n", operand.dimensions[idx]);
        totalSize *= operand.dimensions[idx];
    }

    // Load data
    LOGD(EDEN_DRIVER, "totalSize: %d\n", totalSize);
    for(int32_t idx = 0; idx < totalSize; idx++) {
        loadedData->push_back(srcPtr[idx]);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return loadedData;
}

bool NNNetworkBuilder::isQuanzied(const V1_3::Model& model, const V1_3::Operation& operation) {
    int32_t inputOperandId = operation.inputs[0];
    const V1_3::Operand& inputOperand = model.main.operands[inputOperandId];
    if (inputOperand.type == V1_3::OperandType::TENSOR_QUANT8_ASYMM) {
        return true;
    }

    return false;
}

// get the scale and zeroPoint of input for layer
bool NNNetworkBuilder::getInputScaleAndZeroPoint(const V1_3::Model& model, const V1_3::Operation& operation, std::vector<float>& scale, std::vector<uint32_t>& zeroPoint) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    bool quant = false;

    int32_t numOfInputs = operation.inputs.size();
    LOGD(EDEN_DRIVER, "operation.inputs.size(): %d \n", numOfInputs);
    for (int32_t idx = 0; idx < numOfInputs; idx++) {
        LOGD(EDEN_DRIVER, "idx: %d \n", idx);
        auto inputOperandId = operation.inputs[idx];
        const V1_3::Operand& inputOperand = model.main.operands[inputOperandId];
        quant |= getScaleAndZeroPoint(inputOperand, scale, zeroPoint);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return quant;
}

// get the scale and zeroPoint of output for layer
bool NNNetworkBuilder::getOutputScaleAndZeroPoint(const V1_3::Model& model, const V1_3::Operation& operation, std::vector<float>& scale, std::vector<uint32_t>& zeroPoint) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    bool quant = false;

    int32_t numOfOutputs = operation.outputs.size();
    LOGD(EDEN_DRIVER, "operation.outputs.size(): %d \n", numOfOutputs);
    for (int32_t idx = 0; idx < numOfOutputs; idx++) {
        auto outputOperandId = operation.outputs[idx];
        const V1_3::Operand& outputOperand = model.main.operands[outputOperandId];
        quant |= getScaleAndZeroPoint(outputOperand, scale, zeroPoint);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return quant;
}

// get the scale and zeroPoint for a given operand
bool NNNetworkBuilder::getScaleAndZeroPoint(const V1_3::Operand& operand, std::vector<float>& scale, std::vector<uint32_t>& zeroPoint) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    bool quant = false;
    // scale and zeroPoint
    if ((operand.scale != 0)) {
        LOGD(EDEN_DRIVER, "operand.scale: %f \n", operand.scale);
        scale.push_back(operand.scale);
    }

    LOGD(EDEN_DRIVER, "operand.zeroPoint: %d \n", operand.zeroPoint);
    zeroPoint.push_back(operand.zeroPoint);
    quant = true;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return quant;
}

std::shared_ptr<NPUC::ConvAttr> NNNetworkBuilder::loadConvAttrForConvolutionLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, int32_t numOfInputs, ModelInfo& modelInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    const int32_t inputIndexForinput = 0;
    const int32_t inputIndexForKernel = 1;
    const int32_t inputIndexForBias = 2;

    int32_t inputIdx = operation.inputs[inputIndexForinput];
    LOGD(EDEN_DRIVER, "inputIdx: %d\n", inputIdx);
    int32_t filterIdx = operation.inputs[inputIndexForKernel];
    int32_t biasIdx = operation.inputs[inputIndexForBias];

    const V1_3::Operand& inputOperand = model.main.operands[inputIdx];
    const V1_3::Operand& filterOperand = model.main.operands[filterIdx];
    const V1_3::Operand& biasOperand = model.main.operands[biasIdx];

    uint32_t filterOperandDimSize = static_cast<uint32_t>(filterOperand.dimensions.size());
    for (uint32_t i=0; i < filterOperandDimSize; i++) {
         LOGD(EDEN_DRIVER, "filterOperand.dimensions[%d]: %d\n", i, filterOperand.dimensions[i]);
    }

    uint32_t biasOperandDimSize = static_cast<uint32_t>(biasOperand.dimensions.size());
    for (uint32_t i=0; i < biasOperandDimSize; i++) {
         LOGD(EDEN_DRIVER, "biasOperand.dimensions[%d]: %d\n", i, biasOperand.dimensions[i]);
    }
    // Check unknown dimension, unknown rank
    if ((filterOperand.dimensions.size() == 0) ||   // unknown rank
        (filterOperand.dimensions[W_NHWC] == 0) ||  // unknown dimension
        (filterOperand.dimensions[H_NHWC] == 0)) {  // unknown dimension
        LOGE(EDEN_DRIVER, "filterOperand Unknown dimension, unknown rank is detected... skip it\n");
        return nullptr;
    }
    if ((biasOperand.dimensions.size() == 0) ||   // unknown rank
        (biasOperand.dimensions[0] == 0)) {  // unknown dimension (bias is 1D tensor)
        LOGE(EDEN_DRIVER, "biasOperand Unknown dimension, unknown rank is detected... skip it\n");
        return nullptr;
    }

    EdenOperand* configOperand;
    configConv2d(model, operation, &modelInfo, &configOperand);
    Conv2DOptions* options = reinterpret_cast<Conv2DOptions*>(configOperand->buffer->addr);

    // convAttr
    std::shared_ptr<NPUC::ConvAttr> convAttr = std::make_shared<NPUC::ConvAttr>();

    // getConvolutionLayer always set Convolution 2D.
    // And npuc uses "Conv2D".
    convAttr->setConvType("Conv2D");

    // pad
    std::vector<int> pad;
    pad.push_back(options->padTop);
    pad.push_back(options->padBottom);
    pad.push_back(options->padLeft);
    pad.push_back(options->padRight);
    convAttr->setPad(pad);

    // stride
    std::vector<int> stride;
    stride.push_back(1);
    stride.push_back(1);
    stride.push_back(options->strideHeight);
    stride.push_back(options->strideWidth);
    convAttr->setStride(stride);

    // dilation
    std::vector<int> dilation;
    dilation.push_back(1);
    dilation.push_back(1);
    dilation.push_back(options->dilationHeightFactor);
    dilation.push_back(options->dilationWidthFactor);
    convAttr->setDilation(dilation);

    // padType
    if (numOfInputs == 10) {
        convAttr->setPadType("EXPLICIT");
    } else {
        int32_t paddingType = getValue<int32_t>(model, operation, 3);
        if (paddingType == kPaddingSame) {
            convAttr->setPadType("SAME");
        } else if (paddingType == kPaddingValid) {
            convAttr->setPadType("VALID");
        }
    }

    int32_t filterWidth = filterOperand.dimensions[W_NHWC];
    int32_t filterHeight = filterOperand.dimensions[H_NHWC];
    int32_t numOutput = filterOperand.dimensions[N_NHWC];

    // kernelName
    EdenString newFilterName;
    getMatchedEdenOperandName(FILTER, newFilterName.name, newFilterName.length);
    std::string tmpKernalName((const char*)newFilterName.name);
    std::string kernelName = tmpKernalName + "_" + std::to_string(filterIdx);
    LOGD(EDEN_DRIVER, "kernel name: %s\n", kernelName.c_str());
    releaseName(newFilterName.name);

    // inputs
    std::shared_ptr<std::vector<std::string>> inputs = getInputs(model, operation, operationIdx, 1);
    std::vector<std::vector<int>> inputShape = getInputShape(model, operation, 1);

    // kernel
    std::vector<int> kernel;
    kernel.push_back(filterHeight);
    kernel.push_back(filterWidth);
    convAttr->setKernelInfo(inputShape[0][1], numOutput, kernel);  // <<<<< ??
    convAttr->setKernelName(kernelName);

    // biasName
    EdenString newBiasName;
    getMatchedEdenOperandName(BIAS, newBiasName.name, newBiasName.length);
    std::string tmpBiasName((const char*)newBiasName.name);
    std::string biasName = tmpBiasName + "_" + std::to_string(biasIdx);
    LOGD(EDEN_DRIVER, "bias name: %s\n", biasName.c_str());
    releaseName(newBiasName.name);

    // bias
    convAttr->setBiasName(biasName);
    convAttr->setHasBias(true);

    bool quantized = isQuanzied(model, operation);
    LOGD(EDEN_DRIVER, "Quantized Model: %d\n", quantized);
    if (quantized) {
        LOGD(EDEN_DRIVER, "Quantized Model\n");
        std::shared_ptr<std::vector<uint8_t>> kernelDataOnInt8 = loadData<uint8_t>(model, operation, filterOperand, inputIndexForKernel, modelInfo);
        std::shared_ptr<std::vector<int32_t>> biasDataOnInt32 = loadData<int32_t>(model, operation, biasOperand, inputIndexForBias, modelInfo);
        convAttr->setQuantKernelData_U8(kernelDataOnInt8);
        convAttr->setQuantBiasData(biasDataOnInt32);

        // inputScale, inputZeroPoint
        std::vector<float> inputScale;
        std::vector<uint32_t> inputZeroPoint;
        getScaleAndZeroPoint(inputOperand, inputScale, inputZeroPoint);

        // outputScale, outputZeroPoint
        std::vector<float> outputScale;
        std::vector<uint32_t> outputZeroPoint;
        getOutputScaleAndZeroPoint(model, operation, outputScale, outputZeroPoint);

        // filterScale, filterZeroPoint, biasScale, biasZeroPoint
        std::vector<float> filterScale, biasScale;
        std::vector<uint32_t> filterZeroPoint, biasZeroPoint;
        getScaleAndZeroPoint(filterOperand, filterScale, filterZeroPoint);
        getScaleAndZeroPoint(biasOperand, biasScale, biasZeroPoint);

        convAttr->setInputScale(inputScale);
        convAttr->setInputZeroPoint(inputZeroPoint);
        convAttr->setOutputScale(outputScale);
        convAttr->setOutputZeroPoint(outputZeroPoint);
        convAttr->setWeightScale(filterScale);
        convAttr->setWeightZeroPoint(filterZeroPoint);
        convAttr->setBiasScale(biasScale);
        convAttr->setBiasZeroPoint(biasZeroPoint);

        LOGD(EDEN_DRIVER, "Conv_2D inputZP size: %zu \n", inputZeroPoint.size());
        for (std::vector<uint32_t>::size_type i = 0; i < inputZeroPoint.size(); i++)
        {
            LOGD(EDEN_DRIVER, "inputZeroPoint.at(%zu) size: %d \n", i, inputZeroPoint.at(i));
        }
        LOGD(EDEN_DRIVER, "Conv_2D weightZP size: %zu \n", filterZeroPoint.size());
        for (std::vector<uint32_t>::size_type i = 0; i < filterZeroPoint.size(); i++)
        {
            LOGD(EDEN_DRIVER, "filterZeroPoint.at(%zu) size: %d \n", i, filterZeroPoint.at(i));
        }
        LOGD(EDEN_DRIVER, "Conv_2D biasZP size: %zu \n", biasZeroPoint.size());
        for (std::vector<uint32_t>::size_type i = 0; i < biasZeroPoint.size(); i++)
        {
            LOGD(EDEN_DRIVER, "biasZeroPoint.at(%zu) size: %d \n", i, biasZeroPoint.at(i));
        }
        LOGD(EDEN_DRIVER, "Conv_2D outputZP size: %zu \n", outputZeroPoint.size());
        for (std::vector<uint32_t>::size_type i = 0; i < outputZeroPoint.size(); i++)
        {
            LOGD(EDEN_DRIVER, "outputZeroPoint.at(%zu) size: %d \n", i, outputZeroPoint.at(i));
        }
    } else {
        LOGD(EDEN_DRIVER, "No Quantized Model\n");
        // std::shared_ptr<std::vector<float>> kernelData = loadData<float>(model, operation, filterOperand, inputIndexForKernel);
        // std::shared_ptr<std::vector<float>> biasData = loadData<float>(model, operation, biasOperand, inputIndexForBias);
        // convAttr->setkernelData(kernelData);
        // convAttr->setBiasData(biasData);
    }

    // FusedActivation
    LOGD(EDEN_DRIVER, "FusedActivation: %d\n", static_cast<int32_t>(options->fusedActivation));
    convAttr->setFusedActivation(static_cast<int32_t>(options->fusedActivation));

    convAttr->printAttr();

    releaseConfigOperand(configOperand);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return convAttr;
}

NPUC::NNConvLayer* NNNetworkBuilder::getConvolutionLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, ModelInfo& modelInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // get user inputs form user model
    auto numOfInputs = operation.inputs.size();
    if (numOfInputs != 10 && numOfInputs != 7) {
        LOGE(EDEN_DRIVER, "Exceed supported input parameter count : %d\n", static_cast<int32_t>(numOfInputs));
        return nullptr;
    }

    // layerName
    EdenString newName;
    int32_t opType = static_cast<int32_t>(operation.type);
    getMatchedEdenOperationName(opType, newName.name, newName.length);
    std::string layerName((const char*)newName.name);
    layerName +=  "_" + std::to_string(operationIdx);
    LOGD(EDEN_DRIVER, "layerName : %s\n", layerName.c_str());
    releaseName(newName.name);

    // layerType
    std::string layerType = "Convolution";

    // inputs
    std::shared_ptr<std::vector<std::string>> inputStrings = getInputs(model, operation, operationIdx, 1);

    // outputs
    std::vector<std::string> outputStrings;
    outputStrings.push_back(layerName);

    // convAttr
    std::shared_ptr<NPUC::ConvAttr> convAttr = loadConvAttrForConvolutionLayer(operationIdx, model, operation, numOfInputs, modelInfo);
    if (convAttr == nullptr) {
        LOGE(EDEN_DRIVER, "ConvAttr is null.\n");
        return nullptr;
    }
    NPUC::NNConvLayer *convolutionLayer = new NPUC::NNConvLayer(layerName,
                                                                layerType,
                                                                *inputStrings,
                                                                outputStrings,
                                                                convAttr);

    std::vector<std::vector<int>> inputShape = getInputShape(model, operation, 1);
    LOGD(EDEN_DRIVER, "convolutionLayer->computeOutputShape\n");
    convolutionLayer->computeOutputShape(inputShape);

    auto tempshape = convolutionLayer->getInputShape(0);
    // consumers
    std::vector<std::string> consumers = getConsumers(model, operation);
    LOGD(EDEN_DRIVER, "consumers.size() : %zu\n", consumers.size());
    for(size_t i = 0; i < consumers.size(); i++) {
        convolutionLayer->appendCustomer(consumers[i]);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);

    return convolutionLayer;
}

std::shared_ptr<NPUC::PoolAttr> NNNetworkBuilder::loadPoolAttrForPoolLayer(const V1_3::Model& model, const V1_3::Operation& operation, int32_t numOfInputs,
                                                                           std::string poolType, const ModelInfo* modelInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    EdenOperand* configOperand = nullptr;
    std::shared_ptr<NPUC::PoolAttr> poolAttr = std::make_shared<NPUC::PoolAttr>();
    if (poolType == "AVE") {
        configAveragePool2d(model, operation, modelInfo, &configOperand);
        poolAttr->setPoolType("AvgPool");
    }
    else if (poolType == "MAX"){
        configMaxpool2d(model, operation, modelInfo, &configOperand);
        poolAttr->setPoolType("MaxPool");
    }
    else {
       LOGD(EDEN_DRIVER, "Not supported Pool type!\n");
       return nullptr;
    }

    if (configOperand->buffer->addr == nullptr) {
        LOGE(EDEN_DRIVER, "Error, configOperand->buffer->addr is NULL!\n");
        releaseConfigOperand(configOperand);
        return poolAttr;
    }

    Pool2DOptions* options = reinterpret_cast<Pool2DOptions*>(configOperand->buffer->addr);
    // pad
    std::vector<int> pad;
    pad.push_back(options->padTop);
    pad.push_back(options->padBottom);
    pad.push_back(options->padLeft);
    pad.push_back(options->padRight);
    poolAttr->setPad(pad);

    LOGD(EDEN_DRIVER, "options->padTop: %d\n", options->padTop);
    LOGD(EDEN_DRIVER, "options->padBottom: %d\n", options->padBottom);
    LOGD(EDEN_DRIVER, "options->padLeft: %d\n", options->padLeft);
    LOGD(EDEN_DRIVER, "options->padRight: %d\n", options->padRight);

    // stride
    std::vector<int> stride;
    stride.push_back(1);
    stride.push_back(1);
    stride.push_back(options->strideHeight);
    stride.push_back(options->strideWidth);
    poolAttr->setStride(stride);

    // padType
    if (numOfInputs == 10) {
        poolAttr->setPadType("EXPLICIT");
    } else {
        int32_t paddingType = getValue<int32_t>(model, operation, 1);
        if (paddingType == kPaddingSame) {
            poolAttr->setPadType("SAME");
        } else if (paddingType == kPaddingValid) {
            poolAttr->setPadType("VALID");
        }
    }
    LOGD(EDEN_DRIVER, "PadType: %s\n", poolAttr->getPadType().c_str());

    // input operation
    const int32_t inputIndexForinput = 0;
    int32_t inputIdx = operation.inputs[inputIndexForinput];
    LOGD(EDEN_DRIVER, "inputIdx: %d\n", inputIdx);
    const V1_3::Operand& inputOperand = model.main.operands[inputIdx];

    // kernel
    int32_t filterHeight = 1;
    int32_t filterWidth = 1;
    if(numOfInputs == 10) {
        filterWidth = getValue<int32_t >(model, operation, 7);
        filterHeight = getValue<int32_t >(model, operation, 8);
    } else {
        filterWidth = getValue<int32_t>(model, operation, 4);
        filterHeight = getValue<int32_t>(model, operation, 5);
    }
    std::vector<int> kernel;
    kernel.push_back(1);
    kernel.push_back(1);
    kernel.push_back(filterHeight);
    kernel.push_back(filterWidth);
    poolAttr->setKernel(kernel);

    bool quantized = isQuanzied(model, operation);
    if (quantized) {
        // inputScale, inputZeroPoint
        std::vector<float> inputScale;
        std::vector<uint32_t> inputZeroPoint;
        getScaleAndZeroPoint(inputOperand, inputScale, inputZeroPoint);

        // outputScale, outputZeroPoint
        std::vector<float> outputScale;
        std::vector<uint32_t> outputZeroPoint;
        getOutputScaleAndZeroPoint(model, operation, outputScale, outputZeroPoint);

        poolAttr->setInputScale(inputScale);
        poolAttr->setInputZeroPoint(inputZeroPoint);
        poolAttr->setOutputScale(outputScale);
        poolAttr->setOutputZeroPoint(outputZeroPoint);
        LOGD(EDEN_DRIVER, "Pool inputZP size: %zu \n", inputZeroPoint.size());
        for (uint32_t i = 0; i < inputZeroPoint.size(); i++)
        {
            LOGD(EDEN_DRIVER, "inputZeroPoint.at(%d) size: %d \n", i, inputZeroPoint.at(i));
        }
    }

    // FusedActivation
    LOGD(EDEN_DRIVER, "FusedActivation: %d\n", static_cast<int32_t>(options->fusedActivation));
    poolAttr->setFusedActivation(static_cast<int32_t>(options->fusedActivation));

    releaseConfigOperand(configOperand);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);

    return poolAttr;
}

NPUC::NNPoolLayer* NNNetworkBuilder::getPoolLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, const ModelInfo* modelInfo, std::string poolType) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // get user inputs form user model
    auto numOfInputs = operation.inputs.size();
    if (numOfInputs != 10 && numOfInputs != 7) {
        LOGE(EDEN_DRIVER, "Exceed supported input parameter count : %d\n", static_cast<int32_t>(numOfInputs));
        return nullptr;
    }

    // layerName
    EdenString newName;
    int32_t opType = static_cast<int32_t>(operation.type);
    getMatchedEdenOperationName(opType, newName.name, newName.length);
    std::string layerName((const char*)newName.name);
    layerName +=  "_" + std::to_string(operationIdx);
    LOGD(EDEN_DRIVER, "layerName : %s\n", layerName.c_str());
    releaseName(newName.name);

    // layerType
    std::string layerType = "Pooling";

    // inputs
    std::shared_ptr<std::vector<std::string>> inputStrings = getInputs(model, operation, operationIdx, 1);

    // outputs
    std::vector<std::string> outputStrings;
    outputStrings.push_back(layerName);

    // poolAttr
    std::shared_ptr<NPUC::PoolAttr> poolAttr = loadPoolAttrForPoolLayer(model, operation, numOfInputs, poolType, modelInfo);
    if (nullptr == poolAttr) {
          return nullptr;
    }

    NPUC::NNPoolLayer *poolLayer = new NPUC::NNPoolLayer(layerName,
                                                         layerType,
                                                         *inputStrings,
                                                         outputStrings,
                                                         poolAttr);

    std::vector<std::vector<int>> inputShape = getInputShape(model, operation, 1);
    poolLayer->computeOutputShape(inputShape);

    auto tempshape = poolLayer->getInputShape(0);
    // consumers
    std::vector<std::string> consumers = getConsumers(model, operation);
    LOGD(EDEN_DRIVER, "consumers.size() : %zu\n", consumers.size());
    for(size_t i = 0; i < consumers.size(); i++) {
        poolLayer->appendCustomer(consumers[i]);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);

    return poolLayer;
}

std::shared_ptr<NPUC::ReshapeAttr> NNNetworkBuilder::loadReshapeAttrForReshapeLayer(const V1_3::Model &model, const V1_3::Operation &operation, const ModelInfo* modelInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    EdenOperand* configOperand;
    configReshape(model, operation, modelInfo, &configOperand);
    ReshapeOptions* options = reinterpret_cast<ReshapeOptions*>(configOperand->buffer->addr);

    // reshapeAttr
    std::shared_ptr<NPUC::ReshapeAttr> reshapeAttr = std::make_shared<NPUC::ReshapeAttr>();

    // reshape type
    reshapeAttr->setType("Reshape");
    // shapeInfo
    std::vector<int> shapeInfo;
    for (int32_t i = 0; i < options->numOfDims; i++) {
        shapeInfo.push_back(options->dims.get()[i]);
        LOGD(EDEN_DRIVER, "Shape info defining the shape of the output tensor [%d] = %d\n", i, options->dims.get()[i]);
    }
    reshapeAttr->setShapeInfo(shapeInfo);

    releaseConfigOperand(configOperand);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return reshapeAttr;
}

NPUC::NNReshapeLayer* NNNetworkBuilder::getReshapeLayer(int32_t operationIdx, const V1_3::Model &model,
                                                     const V1_3::Operation &operation, const ModelInfo* modelInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // layerName
    EdenString newName;
    int32_t opType = static_cast<int32_t>(operation.type);
    getMatchedEdenOperationName(opType, newName.name, newName.length);
    std::string layerName((const char*)newName.name);
    layerName +=  "_" + std::to_string(operationIdx);
    LOGD(EDEN_DRIVER, "layerName : %s\n", layerName.c_str());
    releaseName(newName.name);

    // layerType
    std::string layerType = "Reshape";

    // inputs
    std::shared_ptr<std::vector<std::string>> inputStrings = getInputs(model, operation, operationIdx, 1);

    // outputs
    std::vector<std::string> outputStrings;
    outputStrings.push_back(layerName);

    // reshapeAttr
    std::shared_ptr<NPUC::ReshapeAttr> reshapeAttr = loadReshapeAttrForReshapeLayer(model, operation, modelInfo);

    NPUC::NNReshapeLayer *reshapeLayer = new NPUC::NNReshapeLayer(layerName,
                                                                layerType,
                                                                *inputStrings,
                                                                outputStrings,
                                                                reshapeAttr);


    std::vector<std::vector<int>> inputShape = getInputShape(model, operation, 1);
    // set the reshapeParam of reshape layer
    reshapeLayer->computeOutputShape(inputShape);


    auto tempshape = reshapeLayer->getInputShape(0);
    // consumers
    std::vector<std::string> consumers = getConsumers(model, operation);
    LOGD(EDEN_DRIVER, "consumers.size() : %zu\n", consumers.size());
    for(size_t i = 0; i < consumers.size(); i++) {
        reshapeLayer->appendCustomer(consumers[i]);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);

    return reshapeLayer;
}

std::shared_ptr<NPUC::ConcatAttr> NNNetworkBuilder::loadConcatAttrForConcatLayer(const V1_3::Model& model, const V1_3::Operation& operation, const ModelInfo* modelInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    EdenOperand* configOperand;
    configConcatenation(model, operation, modelInfo, &configOperand);
    ConcatenationOptions* options = reinterpret_cast<ConcatenationOptions*>(configOperand->buffer->addr);

    // concatAttr
    std::shared_ptr<NPUC::ConcatAttr> concatAttr = std::make_shared<NPUC::ConcatAttr>();
    int32_t axis = options->axis;

    concatAttr->setAxis(axis);

    // outputScale, outputZeroPoint
    std::vector<float> outputScale;
    std::vector<uint32_t> outputZeroPoint;
    getOutputScaleAndZeroPoint(model, operation, outputScale, outputZeroPoint);
    concatAttr->setOutputScale(outputScale);

    releaseConfigOperand(configOperand);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return concatAttr;
}

NPUC::NNConcatLayer* NNNetworkBuilder::getConcatLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, const ModelInfo* modelInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // layerName
    EdenString newName;
    int32_t opType = static_cast<int32_t>(operation.type);
    getMatchedEdenOperationName(opType, newName.name, newName.length);
    std::string layerName((const char*)newName.name);
    layerName +=  "_" + std::to_string(operationIdx);
    LOGD(EDEN_DRIVER, "layerName : %s\n", layerName.c_str());
    releaseName(newName.name);

    // layerType
    std::string layerType = "Concat";

    // inputs
    uint32_t numberOfInput = static_cast<uint32_t>(operation.inputs.size() - 1);
    std::shared_ptr<std::vector<std::string>> inputStrings = getInputs(model, operation, operationIdx, numberOfInput);

    // outputs
    std::vector<std::string> outputStrings;
    outputStrings.push_back(layerName);

    // reshapeAttr
    std::shared_ptr<NPUC::ConcatAttr> concatAttr = loadConcatAttrForConcatLayer(model, operation, modelInfo);

    NPUC::NNConcatLayer *concatLayer = new NPUC::NNConcatLayer(layerName,
                                                                  layerType,
                                                                  *inputStrings,
                                                                  outputStrings,
                                                                  concatAttr);

    std::vector<std::vector<int>> inputShape = getInputShape(model, operation, numberOfInput);
    concatLayer->computeOutputShape(inputShape);

    auto tempshape = concatLayer->getInputShape(0);
    // consumers
    std::vector<std::string> consumers = getConsumers(model, operation);
    LOGD(EDEN_DRIVER, "consumers.size() : %zu\n", consumers.size());
    for(size_t i = 0; i < consumers.size(); i++) {
        concatLayer->appendCustomer(consumers[i]);
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);

    return concatLayer;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

