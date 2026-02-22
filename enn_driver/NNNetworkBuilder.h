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
 * @file    NNNetworkBuilder.h
 * @brief   This is NNNetworkBuilder class file.
 * @details This header defines NNNetworkBuilder class.
 *          This class is implementing NNNetworkBuilder.
 * @author  hexiang.ji (hexiang.ji@samsung.com)
 *          minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_NN_NETWORK_BUILDER_H_
#define DRIVER_NN_NETWORK_BUILDER_H_

#include <memory>
#include <string>
#include <vector>

#include "HalInterfaces.h"        // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
#include "Utils.h"                // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "./npuc/include/npuc/nnmodel/NNNetwork.h"
#include "./npuc/include/npuc/nnmodel/InputLayer.h"
#include "./npuc/include/npuc/nnmodel/ConvLayer.h"
#include "./npuc/include/npuc/nnmodel/PoolLayer.h"
#include "./npuc/include/npuc/nnmodel/ReshapeLayer.h"
#include "./npuc/include/npuc/nnmodel/ConcatLayer.h"

#include "EdenModelConvertLib.h"

namespace android {
namespace nn {
namespace eden_driver {

class NNNetworkBuilder {
public:
    NNNetworkBuilder();
    std::shared_ptr<NPUC::NNNetwork> buildNNNetwork(const V1_3::Model& model, ModelInfo& modelInfo, const std::vector<int32_t>& operationList);

private:

    uint32_t _dummyInputCount = 0;

    // create the input layer base the shape of first layer
    NPUC::NNInputLayer* createDummyInputLayer(const std::vector<int>& inputShape, uint32_t layerNameCount = 0);

    // get the inputs of layer
    std::shared_ptr<std::vector<std::string>> getInputs(const V1_3::Model& model, const V1_3::Operation& targetOperation, int operationIdx, uint32_t numOfInput);

    // get the output of layer
    std::shared_ptr<std::vector<std::string>> getOutputs(const V1_3::Model& model, const V1_3::Operation& operation);

    // get the input shape of layer
    std::vector<std::vector<int>> getInputShape(const V1_3::Model& model, const V1_3::Operation& operation, uint32_t numOfInputs);

    // get the output shape of layer
    std::vector<std::vector<int>> getOutputShape(const V1_3::Model& model, const V1_3::Operation& operation);

    // get the consumer of layer
    std::vector<std::string> getConsumers(const V1_3::Model& model, const V1_3::Operation& targetOperation);

    // load data from the operand
    template <typename T>
    std::shared_ptr<std::vector<T>> loadData(const V1_3::Model& model, const V1_3::Operation& operation, const V1_3::Operand& operand, int32_t op_id, ModelInfo& modelInfo);

    bool isQuanzied(const V1_3::Model& model, const V1_3::Operation& operation);

    // get the scale and zeroPoint of input for layer
    bool getInputScaleAndZeroPoint(const V1_3::Model& model, const V1_3::Operation& operation, std::vector<float>& scale, std::vector<uint32_t>& zeroPoint);

    // get the scale and zeroPoint of output for layer
    bool getOutputScaleAndZeroPoint(const V1_3::Model& model, const V1_3::Operation& operation, std::vector<float>& scale, std::vector<uint32_t>& zeroPoint);

    // get the scale and zeroPoint for a given operand
    bool getScaleAndZeroPoint(const V1_3::Operand& operand, std::vector<float>& scale, std::vector<uint32_t>& zeroPoint);

    std::shared_ptr<NPUC::ConvAttr> loadConvAttrForConvolutionLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, int32_t numOfInputs, ModelInfo& modelInfo);

    // construct the Convolution layer
    NPUC::NNConvLayer* getConvolutionLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, ModelInfo& modelInfo);

    // get the Pool layer param
    std::shared_ptr<NPUC::PoolAttr> loadPoolAttrForPoolLayer(const V1_3::Model& model, const V1_3::Operation& operation, int32_t numOfInputs,
                                                         std::string poolType, const ModelInfo* modelInfo);

    // construct the Pool layer
    NPUC::NNPoolLayer* getPoolLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, const ModelInfo* modelInfo, std::string poolType);

    std::shared_ptr<NPUC::ReshapeAttr> loadReshapeAttrForReshapeLayer(const V1_3::Model& model, const V1_3::Operation& operation, const ModelInfo* modelInfo);

    // construct the Reshape layer
    NPUC::NNReshapeLayer* getReshapeLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, const ModelInfo* modelInfo);

    std::shared_ptr<NPUC::ConcatAttr> loadConcatAttrForConcatLayer(const V1_3::Model& model, const V1_3::Operation& operation, const ModelInfo* modelInfo);

    // construct the Concat layer
    NPUC::NNConcatLayer* getConcatLayer(int32_t operationIdx, const V1_3::Model& model, const V1_3::Operation& operation, const ModelInfo* modelInfo);
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_NN_NETWORK_BUILDER_H_

