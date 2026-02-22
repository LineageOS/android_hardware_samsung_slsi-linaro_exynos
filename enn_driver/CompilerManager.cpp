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
 * @file    CompilerManager.cpp
 * @brief   This is CompilerManager class file.
 * @details This header defines CompilerManager class.
 *          This class is implementing handshaking with compiler component.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#include <iostream>
#include "log.h"
#include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "CompilerManager.h"
#include "NpucIrConverter.h"
#include "EdenModelConvertLib.h"
#include "EdenRuntime.h"
#include "eden_types.h"
#include "MyUtils.h"  // LoadFromFile
#include "./npuc/include/npuc/NPUcompiler.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::CompilerManager"

namespace android {
namespace nn {
namespace eden_driver {

/**
 * @brief CompilerManager constructor
 * @details This function creates NpucIrConverter and NPUCompiler instances
 * @param void
 */
CompilerManager::CompilerManager() {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    inputOffset_ = 0;
    outputOffset_ = 0;
    // Create NpucIrConverter
    npucIrConverter_ = std::make_shared<NpucIrConverter>();
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief initNPUCCompiler
 * @details This function creates set npuc options and set npuCompiler
   @warnig : This function should be called afert RT init done.
 * @param void
 */
void CompilerManager::initNPUCCompiler(){
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // Create NPUCompiler
    NPUC::NPUCCompilerOptions *options = new NPUC::NPUCCompilerOptions();
    NPUC::SOCType npucSocType;
    getNpucSOCType(npucSocType);
    options->setSOCType(npucSocType);

    if (options->getSOCType() == NPUC::SOCType::ST_MAKALU) {
        inputOffset_ = -128;
        outputOffset_ = 128;
        options->setQuantizationMode(NPUC::QuantizationMode::QUASI_SYM);
    } else if (options->getSOCType() == NPUC::SOCType::ST_NEUS || options->getSOCType() == NPUC::SOCType::ST_2020) {
        inputOffset_ = 0;
        outputOffset_ = 0;
        options->setQuantizationMode(NPUC::QuantizationMode::ASYM);
    } else {
        inputOffset_ = 0;
        outputOffset_ = 0;
        options->setQuantizationMode(NPUC::QuantizationMode::SYM);
    }

    npuCompiler_ = std::make_shared<NPUC::NPUCompiler>(options);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief get SOC type from RT and convert type according to NPUC rule.
   @rarams NPUC::SOCType
 * @return void
 */
void CompilerManager::getNpucSOCType(NPUC::SOCType& npuSocType){
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    uint32_t modelId = 0;
    int32_t versions[VERSION_MAX] = {0, };
    RtRet ret = eden::rt::GetEdenVersion(modelId, versions);
    if (RT_SUCCESS != ret){
        LOGE(EDEN_DRIVER, "Fail to GetEdenVersion\n");
        return ;
    }
    int32_t socType = versions[PLATFORM_VERSION_SOC];
    LOGD(EDEN_DRIVER, "SOC Type = %d\n", socType);

    switch(socType){
    case 990:
    case 9830:
        // FIXME : NPUC use ST_NEUS for 9830 and 990.
        // npuSocType = NPUC::SOCType::ST_2020;
         npuSocType = NPUC::SOCType::ST_NEUS;
        break;
    case 980:
    case 9630:
        npuSocType = NPUC::SOCType::ST_NEUS;
        break;
    case 9820:
        npuSocType = NPUC::SOCType::ST_MAKALU;
        break;
    default :
        npuSocType = NPUC::SOCType::ST_NEUS;
        break;
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return;
}

/**
 * @brief get input offset in order to convert input by adding input offset
 * @return inputOffset offset to be added to input values (for example, MAKALU: -128)
 */
int32_t CompilerManager::getInputOffset() {
    return inputOffset_;
}

/**
 * @brief get input offset in order to convert input by adding output offset
 * @return inputOffset offset to be added to input values (for example, MAKALU: +128)
 */
int32_t CompilerManager::getOutputOffset() {
    return outputOffset_;
}

/**
 * @brief Get performance information in terms of execution time and power usage
 * @details This function returns a performance information in terms of execution time and power useage.
 *          It depends on a given type for quant8, float32 or float16.
 *          This is the ratio number as cpu's performance for mobilenet.
 * @param[in] type 0: quant8, 1: float32, 2: float16
 * @param[out] perfInfo structure for executin time and power usage
 * @return error code
 */
int32_t CompilerManager::getPerformanceInfo(DATA_TYPE type, V1_0::PerformanceInfo& perfInfo) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    switch (type) {
    case DATA_TYPE::FLOAT32:
        // @todo Performance number should be retrived from other module which knows the exact performance number.
        perfInfo.execTime = 0.4;    // Naive number
        perfInfo.powerUsage = 0.4;  // Naive number
        break;

    case DATA_TYPE::QUANT8:
        // @todo Performance number should be retrived from other module which knows the exact performance number.
        perfInfo.execTime = 0.1;    // Naive number
        perfInfo.powerUsage = 0.1;  // Naive number
        break;

    case DATA_TYPE::RELAXED_FLOAT32:
        // @todo Performance number should be retrived from other module which knows the exact performance number.
        perfInfo.execTime = 0.5;    // Naive number
        perfInfo.powerUsage = 0.5;  // Naive number
        break;

    default:
        LOGE(EDEN_DRIVER, "Invalid type is delivered! type=%d\n", static_cast<int32_t>(type));
        return INVALID_PARAMS;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return 0;
}

/**
 * @brief isSupportedModelByNPUC
 * @details This function indicates whether the model is supported by NPUC.
 * @param[in] model Android NN Model
 * @param[out]
 * @return yes(true) or no(false)
 */
bool CompilerManager::isSupportedModelByNPUC(const V1_3::Model& model) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    bool ret = true;

    // Only for IV3
    int32_t bitSum = 0;
    int32_t opType;
    for (const V1_3::Operation& androidOperation : model.main.operations) {
        opType = static_cast<int32_t>(androidOperation.type);
        switch (opType) {
            case ANEURALNETWORKS_CONV_2D:
                bitSum |= 1; break;
            case ANEURALNETWORKS_MAX_POOL_2D:
                bitSum |= 1 << 1; break;
            case ANEURALNETWORKS_AVERAGE_POOL_2D:
                bitSum |= 1 << 2; break;
            case ANEURALNETWORKS_RESHAPE:
                bitSum |= 1 << 3; break;
            case ANEURALNETWORKS_CONCATENATION:
                bitSum |= 1 << 4; break;
            case ANEURALNETWORKS_SOFTMAX:
                bitSum |= 1 << 5; break;
            default :
                ret = false; break;
        }
        if(ret == false) break;
    }

    if(ret == true) {
        switch(bitSum) {
            case 0x1F: // 0b 0001 1111
            case 0x3F: // 0b 0011 1111
                //supportedOperations[ANEURALNETWORKS_CONV_2D] = true;
                //supportedOperations[ANEURALNETWORKS_MAX_POOL_2D] = true;
                //supportedOperations[ANEURALNETWORKS_AVERAGE_POOL_2D] = true;
                //supportedOperations[ANEURALNETWORKS_RESHAPE] = true;
                //supportedOperations[ANEURALNETWORKS_CONCATENATION] = true;
                //ANEURALNETWORKS_SOFTMAX is not operated by NPU
                ret = true;
                break;
            default :
                ret = false;
                break;
        }
    }
    else {
        ret = false;
    }

    LOGD(EDEN_DRIVER, "%s() ret = %d, bitSum = 0x%x\n", __func__, ret, bitSum);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return ret;
}



/**
 * @brief Get supported operation list by NPU compiler
 * @details This function returns a supported operation list by NPU compiler.
 *          Operation index matched to OperationType defined on Android NN's types.hal.
 *          e.g. Android NN's OperationType for ADD is 0, if NPU compiler supports this operation,
 *               supportedOperations[0] has true. Otherwise it has false.
 * @param[in] model Android NN Model
 * @param[in] constraints list of constraints for each operation
 * @param[out] supportedOperations supported operation list by NPU compiler
 * @return error code
 */
int32_t CompilerManager::getSupportedOperations(const V1_3::Model& model, const std::vector<std::shared_ptr<void>>& constraints,
                                                std::vector<bool>& supportedOperations) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    if (!isSupportedModelByNPUC(model)) {
        for (const V1_3::Operation& androidOperation : model.main.operations) {
            supportedOperations[static_cast<int32_t>(androidOperation.type)] = false;
        }
        return RET_OK;
    }

    uint32_t index = 0;
    for (const V1_3::Operation& androidOperation : model.main.operations) {
        int32_t opType = static_cast<int32_t>(androidOperation.type);
        switch (opType) {
        case ANEURALNETWORKS_CONV_2D:
            {
                LOGD(EDEN_DRIVER, "OPLIST:%d: ANEURALNETWORKS_CONV_2D\n", index);
                // @todo constraints might be used in future
                if (verifyConstraints(model, androidOperation, opType, constraints) == false) {
                    LOGD(EDEN_DRIVER, "CONV_2D is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                    break;
                }

                // Read kernelSize, paddingSize, strideSize and zeroPoint
                Conv2DParams conv2DParams;
                readConv2DParams(model, androidOperation, conv2DParams);
                showConv2DParams(conv2DParams);

                NPUC::SupportedOperationOptions opOptions;
                opOptions.setKernelSize(conv2DParams.kernelSize);
                opOptions.setPaddingSize(conv2DParams.paddingSize);
                opOptions.setStrideSize(conv2DParams.strideSize);
                opOptions.setZeroPoint(conv2DParams.zeroPoint);
                bool supported = npuCompiler_->isSupportedOperation(NPUC::OpType::CONV_2D, opOptions);
                if (supported) {
                    LOGD(EDEN_DRIVER, "CONV_2D is supported by NPU.\n");
                    supportedOperations[opType] = true;
                } else {
                    LOGD(EDEN_DRIVER, "CONV_2D is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                }
            }
            break;
        case ANEURALNETWORKS_MAX_POOL_2D:
            {
                LOGD(EDEN_DRIVER, "OPLIST:%d: ANEURALNETWORKS_MAX_POOL_2D\n", index);
                // @todo constraints might be used in future
                if (verifyConstraints(model, androidOperation, opType, constraints) == false) {
                    LOGD(EDEN_DRIVER, "MAX_POOL is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                    break;
                }

                // Read kernelSize, paddingSize, strideSize and zeroPoint
                Pool2DParams poolParams;
                readPool2DParams(model, androidOperation, poolParams);
                showPool2DParams(poolParams);

                NPUC::SupportedOperationOptions opOptions;
                opOptions.setKernelSize(poolParams.kernelSize);
                opOptions.setPaddingSize(poolParams.paddingSize);
                opOptions.setStrideSize(poolParams.strideSize);
                opOptions.setZeroPoint(poolParams.zeroPoint);
                LOGD(EDEN_DRIVER, "MAX_POOL kernelsize: %d, paddingsize: %d, stridesize: %d, zp: %d .\n", poolParams.kernelSize, poolParams.paddingSize, poolParams.strideSize, poolParams.zeroPoint);
                bool supported = npuCompiler_->isSupportedOperation(NPUC::OpType::MAX_POOL_2D, opOptions);
                if (supported) {
                    LOGD(EDEN_DRIVER, "MAX_POOL is supported by NPU.\n");
                    supportedOperations[opType] = true;
                } else {
                    LOGD(EDEN_DRIVER, "MAX_POOL is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                }
            }
            break;
        case ANEURALNETWORKS_AVERAGE_POOL_2D:
            {
                LOGD(EDEN_DRIVER, "OPLIST:%d: ANEURALNETWORKS_AVERAGE_POOL_2D\n", index);
                // @todo constraints might be used in future
                if (verifyConstraints(model, androidOperation, opType, constraints) == false) {
                    LOGD(EDEN_DRIVER, "AVG_POOL is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                    break;
                }

                // Read kernelSize, paddingSize, strideSize and zeroPoint
                Pool2DParams poolParams;
                readPool2DParams(model, androidOperation, poolParams);
                showPool2DParams(poolParams);

                NPUC::SupportedOperationOptions opOptions;
                opOptions.setKernelSize(poolParams.kernelSize);
                opOptions.setPaddingSize(poolParams.paddingSize);
                opOptions.setStrideSize(poolParams.strideSize);
                opOptions.setZeroPoint(poolParams.zeroPoint);
                LOGD(EDEN_DRIVER, "AVG_POOL kernelsize: %d, paddingsize: %d, stridesize: %d, zp: %d .\n", poolParams.kernelSize, poolParams.paddingSize, poolParams.strideSize, poolParams.zeroPoint);
                uint32_t width = model.main.operands[androidOperation.inputs[0]].dimensions[W_NHWC];
                uint32_t height = model.main.operands[androidOperation.inputs[0]].dimensions[H_NHWC];
                LOGD(EDEN_DRIVER, "AVG_POOL input width: %d, height: %d \n", width, height);
                bool supported = npuCompiler_->isSupportedOperation(NPUC::OpType::AVERAGE_POOL_2D, opOptions);
                // add special case (global avg pooling) for hosted IV3 model
                if ((width == 8 || width == 9) && poolParams.kernelSize == 8 && poolParams.strideSize == 2 && poolParams.paddingSize == 0) {
                    supported = true;
                }
                if (supported) {
                    LOGD(EDEN_DRIVER, "AVG_POOL is supported by NPU.\n");
                    supportedOperations[opType] = true;
                } else {
                    LOGD(EDEN_DRIVER, "AVG_POOL is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                }
            }
            break;
        case ANEURALNETWORKS_RESHAPE:
            {
                LOGD(EDEN_DRIVER, "OPLIST:%d: ANEURALNETWORKS_RESHAPE\n", index);
                // @todo constraints might be used in future
                if (verifyConstraints(model, androidOperation, opType, constraints) == false) {
                    LOGD(EDEN_DRIVER, "RESHAPE is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                    break;
                }

                NPUC::SupportedOperationOptions opOptions;
                bool supported = npuCompiler_->isSupportedOperation(NPUC::OpType::RESHAPE, opOptions);
                if (supported) {
                    LOGD(EDEN_DRIVER, "RESHAPE is supported by NPU.\n");
                    supportedOperations[opType] = true;
                } else {
                    LOGD(EDEN_DRIVER, "RESHAPE is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                }
            }
            break;
        case ANEURALNETWORKS_CONCATENATION:
            {
                LOGD(EDEN_DRIVER, "OPLIST:%d: ANEURALNETWORKS_CONCATENATION\n", index);
                // @todo constraints might be used in future
                if (verifyConstraints(model, androidOperation, opType, constraints) == false) {
                    LOGD(EDEN_DRIVER, "CONCATENATION is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                    break;
                }

                // Read axis
                ConcatParams concatParams;
                LOGD(EDEN_DRIVER, "CONCATENATION readConcatParams is started.\n");
                readConcatParams(model, androidOperation, concatParams);
                showConcatParams(concatParams);

                NPUC::SupportedOperationOptions opOptions;
                opOptions.setAxis(concatParams.axis);
                LOGD(EDEN_DRIVER, "CONCATENATION axis: %d .\n", concatParams.axis);

                bool supported = npuCompiler_->isSupportedOperation(NPUC::OpType::CONCATENATION, opOptions);
                if (supported) {
                    LOGD(EDEN_DRIVER, "CONCATENATION is supported by NPU.\n");
                    supportedOperations[opType] = true;
                } else {
                    LOGD(EDEN_DRIVER, "CONCATENATION is not supported by NPU.\n");
                    supportedOperations[opType] = false;
                }
            }
            break;
        default:
            LOGD(EDEN_DRIVER, "OPLIST: %d: opType=%d is not supported by NPU.\n", index, opType);
            supportedOperations[opType] = false;
            break;
        }
        index++;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
* @brief Get constraints for each operations by NPU compiler
* @details This function returns contraints for each operations by NPU compiler.
*          Operation index matched to OperationType defined on Android NN's types.hal.
*          e.g. Android NN's OperationType for CONV2D is 3,
*               if NPU compiler supports this operation and has a specific constraints like,
*               kernel_size=3x3 is only supported, constraints[3] has a pointer to Conv2DConstrains.
*               which represents for a constrain of Conv2D by NPU Compiler.
*               Otherwise it has nullptr.
* @param[out] constraints constrains list by NPU compiler. Each operation has its own data structure.
* @return error code
*/
int32_t CompilerManager::getConstraints(std::vector<std::shared_ptr<void>>& constraints) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // ANEURALNETWORKS_CONV_2D
    {
        // Now just add constraint for Conv2D
        std::shared_ptr<Conv2DConstraint> spConv2DConstraint = std::make_shared<Conv2DConstraint>();

        // @todo, query constraints to NPU compiler via npuc ir converter
        spConv2DConstraint->maxKernelSize = 8;
        spConv2DConstraint->maxStrideSize = 5;
        spConv2DConstraint->maxPaddingSize = 5;

        // currently, convolution is supported.
        constraints[ANEURALNETWORKS_CONV_2D] = spConv2DConstraint;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Compile operations on operationList of a given model and returns generated binary(NCP) on buffer
 * @details This function compiles an operation list of a given model for NPU and generates a result on buffer.
 *          Memory allocation is not caller's responsibility.
 *          This function cooperates with npuc_ir_converter to complete compilation for NPU.
 * @param[in] model Android NN Model
 * @param[in] operationList operation index list corresponding on Model's Operation[] to be compiled for NPU
 * @param[out] buffer buffer for NCP. Memory allocation should be handled within this function.
 * @param[out] bufferSize buffer size for NCP. Memory allocation should be handled within this function.
 * @return error code
 */
int32_t CompilerManager::compile(const V1_3::Model& model, ModelInfo& modelInfo, const std::vector<int32_t>& operationList, void** buffer, int32_t* bufferSize) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int32_t ret = 0;

    LOGD(EDEN_DRIVER, "Call convertToNNNetwork to convert Android NN Model to NNNetwork...\n");
    for (auto opId : operationList) LOGD(EDEN_DRIVER, "opId=%d\n", opId);

    std::shared_ptr<NPUC::NNNetwork> network = std::make_shared<NPUC::NNNetwork>();
    ret = npucIrConverter_->convertToNNNetwork(model, modelInfo, operationList, network);
    if (ret != RET_OK) {
        LOGE(EDEN_DRIVER, "Fail to convert Android NN Model to NNNetwork.\n");
        return FAIL_ON_NPUC_IR_CONVERTER;
    }

    LOGD(EDEN_DRIVER, "Call npuCompiler_->compile to generate NPUC on buffer...\n");

    std::unique_ptr<NPUC::NCPBuffer> ncpBuffer =  std::make_unique<NPUC::NCPBuffer>();
    ret = npuCompiler_->compile(network.get(), ncpBuffer.get());
    if (ret != 0) {
        LOGD(EDEN_DRIVER, "Fail to compile NNNetwork to get NCP.\n");
        return FAIL_ON_NPU_COMPILER;
    }
    LOGD(EDEN_DRIVER, "Compile is complete, addr=%p, size=%d\n", reinterpret_cast<void*>(ncpBuffer->addr), ncpBuffer->size);

    // Get address and size
    *buffer = ncpBuffer->addr;
    *bufferSize = ncpBuffer->size;

    // *buffer = (unsigned char*)malloc(ncpBuffer->size);
    // std::memcpy(*buffer, ncpBuffer->addr, ncpBuffer->size);

    // delete ncpBuffer;
    // std::string dumpFileName("/data/ir_ncp_object.bin");
    // DumpToFile(dumpFileName, ncpBuffer->addr, ncpBuffer->size);

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Verify if a given operation is acceptable with a given constraints
 * @details This function verifies a given operation if it has any violation of a given constraints.
 * @param[in] model Android NN Model
 * @param[in] operation Android Operation
 * @param[in] opType opType defined on EdenModel
 * @param[in] constraints list of constraints for each operation
 * @return error code
 */
bool CompilerManager::verifyConstraints(const V1_3::Model& model, const V1_3::Operation& operation, int32_t opType,
                                        const std::vector<std::shared_ptr<void>>& /*constraints*/) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    bool verified = true;
    switch (opType) {
        case ANEURALNETWORKS_CONV_2D:
        case ANEURALNETWORKS_MAX_POOL_2D:
        case ANEURALNETWORKS_AVERAGE_POOL_2D:
        case ANEURALNETWORKS_RESHAPE:
        case ANEURALNETWORKS_CONCATENATION:
        // Tensor type should be TENSOR_QUANT8_ASYMM
        int32_t type = readOperandType(model, operation, 0);
        if (type == -1) {
            verified = false;
        }
        if (static_cast<V1_3::OperandType>(type) != V1_3::OperandType::TENSOR_QUANT8_ASYMM) {
            verified = false;
        }
        break;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return verified;
}

void CompilerManager::clearNcpBuffer(void* ncpBuffer) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);
    if (ncpBuffer) {
        // TODO : Let compiler do this job.
        // npuCompiler_->clearNcpBuffer(ncpBuffer);
    }
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

