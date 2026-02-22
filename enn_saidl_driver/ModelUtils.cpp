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
 * @file    ModelUtils.cpp
 * @brief   This is ModelUtils class file.
 * @details This header defines ModelConverter class.
 *          This class is implementing Model Utility Functions.
 * @author  nihar.desai/shashank.r/ankit.goel
 */

#include <iostream>
#include <variant>
#include <sys/mman.h>
#include "log.h"

#include "NeuralNetworks.h"  // Operation, Operand, ANEURALNETWORKS_ADD etc
#include "Common.h"   // DATA_TYPE, OperationInfo

#include "ModelUtils.h"
#include "GraphGenManager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EnnDriver::ModelUtils"

using namespace ::android::nn;
using namespace android::nn::enn_driver;

namespace android {
namespace nn {
namespace enn_driver {

#ifdef NNDRIVER_DEBUG
void showModel(const Model& model) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    int numOfOperand = model.main.operands.size();
    LOGD(ENN_DRIVER, "model.main.operands.size()=%d\n", numOfOperand);

    int numOfOperations = model.main.operations.size();
    LOGD(ENN_DRIVER, "model.main.operations.size()=%d\n", numOfOperations);

    int numOfInputIndexes = model.main.inputIndexes.size();
    LOGD(ENN_DRIVER, "model.main.inputIndexes.size()=%d\n", numOfInputIndexes);
    for (int i = 0; i < numOfInputIndexes; i++) {
        LOGD(ENN_DRIVER, "    %d\n", model.main.inputIndexes[i]);
    }

    int numOfOutputIndexes = model.main.outputIndexes.size();
    LOGD(ENN_DRIVER, "model.main.outputIndexes.size()=%d\n", numOfOutputIndexes);
    for (int i = 0; i < numOfOutputIndexes; i++) {
        LOGD(ENN_DRIVER, "    %d\n", model.main.outputIndexes[i]);
    }

    for (int i = 0; i < numOfOperations; i++) {
        const Operation& operation = model.main.operations[i];
        LOGD(ENN_DRIVER, "[%d] type=%d\n", i, operation.type);
        LOGD(ENN_DRIVER, "    inputs...\n");
        for (size_t j = 0; j < operation.inputs.size(); j++) {
            LOGD(ENN_DRIVER, "        %d\n", operation.inputs[j]);
        }
        LOGD(ENN_DRIVER, "    outputs...\n");
        for (size_t j = 0; j < operation.outputs.size(); j++) {
            LOGD(ENN_DRIVER, "        %d\n", operation.outputs[j]);
        }
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
}
#endif

/**
 * @brief Set GraphGenManager
 * @details This function sets an association to GraphGenManager
 * @param[in] GraphGenManager an instance of GraphGenManager.
 * @returns void
 */
void ModelConverter::setGraphGenManager(std::shared_ptr<GraphGenManager> graphgenManager) {
    graphgenManager_ = graphgenManager;
}


/**
 * @brief Convert Android NN Model to NNC
 * @details This function converts an Android NN Model to NNC.
 * @param[in] model Android NN Model to be converted
 * @param[out] nncBuffer NNCBuf structure
 * @param[out] modelInfo model specific information such as virtual address map, supported operation group
 * @returns return code
 */
int32_t ModelConverter::convertToNNC(const Model& model, NNCBuf& nncBuffer,
                                     NNCOpInfo& nncOpInfo) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

#ifdef NNDRIVER_DEBUG
     showModel(model);
#endif

    if (model.main.inputIndexes.size() == 0) {
        return INVALID_PARAMS;
    }
    if (model.main.outputIndexes.size() == 0) {
        return INVALID_PARAMS;
    }


    int32_t retCode = graphgenManager_->generateNNC(model, nncBuffer, nncOpInfo);
    if (retCode != RET_OK) {
        LOGE(ENN_DRIVER, "%s(-) Error on generateNNC(..)!\n", __func__);
        return retCode;
    }

    if (nncBuffer.addr == nullptr || nncBuffer.size <= 0) {
        LOGE(ENN_DRIVER, "%s(-) Buffer is nullptr! or Buffer size error(%d)!\n", __func__, nncBuffer.size);
        return FAIL_ON_GENERATE_NNC;
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}


int32_t getVirtualAddressOnPool(const SharedMemory& memory, bool needToWrite, VirtualAddressInfo& vInfo,
                                android::nn::Mapping mapping) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    if(auto *mFd = std::get_if<Memory::Fd>(&(memory->handle))) {
        int32_t size = mapping.size;
        int32_t fd = mFd->fd;
        int32_t prot = mFd->prot;
        size_t offset = mFd->offset;

        char* mappedAddr = reinterpret_cast<char*>(mmap(nullptr, size, prot, MAP_SHARED, fd, offset));

        if (needToWrite == false) {
            char* virtAddr = mappedAddr;

            vInfo.type = MMAP_FD_2;  // 2 for mmap_fd, need to call munmap(..) later
            vInfo.addr = virtAddr;
            vInfo.size = size;
        } else {
            if (prot & PROT_WRITE) {
                char* virtAddr = mappedAddr;

                vInfo.type = MMAP_FD_2;  // 2 for mmap_fd, need to call munmap(..) later
                vInfo.addr = virtAddr;
                vInfo.size = size;
            } else {
                char* virtAddr = new char[size];
                std::memcpy(virtAddr, mappedAddr, size);
                munmap(mappedAddr, size);

                vInfo.type = MMAP_FD_1;  // 1 for mmap_fd, need to call delete[] later
                vInfo.addr = virtAddr;
                vInfo.size = size;
            }
        }
        LOGD(ENN_DRIVER, "mmap_fd, vInfo.addr=%p, size=%d\n", vInfo.addr, vInfo.size);
    } else if(auto *mAshmem = std::get_if<Memory::Ashmem>(&(memory->handle))) {
          int32_t size = mapping.size;
          int32_t fd = mAshmem->fd;
          int32_t prot = PROT_READ|PROT_WRITE;
          size_t offset = 0;

          char* mappedAddr = reinterpret_cast<char*>(mmap(nullptr, size, prot, MAP_SHARED, fd, offset));
          vInfo.type = ASHMEM;  // 0 for ashmem
          vInfo.addr = mappedAddr;
          vInfo.size = size;
          LOGD(ENN_DRIVER, "ashmem, vInfo.addr=%p, size=%d\n", reinterpret_cast<void*>(vInfo.addr), vInfo.size);
    } else if(auto *mHwBuffer = std::get_if<Memory::HardwareBuffer>(&(memory->handle))) {
          AHardwareBuffer_Desc desc;
          AHardwareBuffer_describe(mHwBuffer->handle.get(), &desc);

         if (desc.format != AHARDWAREBUFFER_FORMAT_BLOB) {
              LOGE(ENN_DRIVER,  "Unable to map non-blob AHardwareBuffer memory");
         }
         const uint32_t size = desc.width;

         const uint64_t kCpuUsageMask = AHARDWAREBUFFER_USAGE_CPU_READ_MASK | AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK;
         void* data = nullptr;
         const auto status = AHardwareBuffer_lock(mHwBuffer->handle.get(), desc.usage & kCpuUsageMask, -1,
                                             nullptr, &data);
         if (status !=  0) {
              LOGE(ENN_DRIVER,  "Unable to lock AHardwareBuffer memory");
         }

         vInfo.type = AHWB_BLOB;  // AHWB_BLOB for Hardware Buffer
         vInfo.addr = static_cast<char*>(data);
         vInfo.size = size;
         LOGD(ENN_DRIVER, "HardwareBuffer, vInfo.addr=%p, size=%d\n", reinterpret_cast<void*>(vInfo.addr), vInfo.size);
    }
    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Verify request in terms of dimension
 * @details This function verifies a given request in terms of dimension.
 * @param[in] request Request
 * @param[in] model Android NN Model
 * @returns return code
 */
bool verifyDimension(const Request& request, const Model& model) {
    LOGD(ENN_DRIVER, "%s(+)\n", __func__);

    const auto& inputs = request.inputs;
    const auto& outputs = request.outputs;

    LOGD(ENN_DRIVER, "inputs.size()=%zu, outputs.size()=%zu\n", inputs.size(), outputs.size());

    for (size_t idx = 0; idx < inputs.size(); idx++) {
        const Request::Argument& inputArgs = inputs[idx];

        auto androidOperandIdx = model.main.inputIndexes[idx];
        auto androidOperand = model.main.operands[androidOperandIdx];

        for (size_t i = 0; i < inputArgs.dimensions.size(); i++) {
            if (inputArgs.dimensions[i] != androidOperand.dimensions[i]) {
                LOGW(ENN_DRIVER, "Error, different/ dimension of inputs\n");
                LOGW(ENN_DRIVER, "inputArgs.dimensions={\n");
                for (int32_t idx : inputArgs.dimensions) {
                    LOGW(ENN_DRIVER, "%d \n", idx);
                }
                LOGW(ENN_DRIVER, "}, androidOperand.dimensions={\n");
                for (int32_t idx : androidOperand.dimensions) {
                    LOGW(ENN_DRIVER, "%d \n", idx);
                }
                LOGW(ENN_DRIVER, "}\n");
                return false;
            }
        }
    }

    for (size_t idx = 0; idx < outputs.size(); idx++) {
        const Request::Argument& outputArgs = outputs[idx];

        auto androidOperandIdx = model.main.outputIndexes[idx];
        auto androidOperand = model.main.operands[androidOperandIdx];

        for (size_t i = 0; i < outputArgs.dimensions.size(); i++) {
            if (outputArgs.dimensions[i] != androidOperand.dimensions[i]) {
                LOGW(ENN_DRIVER, "Error, different dimension of outputs\n");
                LOGW(ENN_DRIVER, "outputArgs.dimensions={\n");
                for (int32_t idx : outputArgs.dimensions) {
                    LOGW(ENN_DRIVER, "%d \n", idx);
                }
                LOGW(ENN_DRIVER, "}, androidOperand.dimensions={\n");
                for (int32_t idx : androidOperand.dimensions) {
                    LOGW(ENN_DRIVER, "%d \n", idx);
                }
                LOGW(ENN_DRIVER, "}\n");
                return false;
            }
        }
    }

    LOGD(ENN_DRIVER, "%s(-)\n", __func__);
    return true;
}

}  // namespace enn_driver
}  // namespace nn
}  // namespace android

