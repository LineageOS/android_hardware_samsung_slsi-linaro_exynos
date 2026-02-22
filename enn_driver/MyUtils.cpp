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
 * @file    MyUtils.cpp
 * @brief   This is unitily functions to be used for EdenDriver.
 * @details This header defines utility functions to be used for EdenDriver.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#include <iostream>  // cout, endl
#include <fstream>   // ifstream, ofstream
#include <vector>
#include <string>

#include "log.h"
#include "Utils.h"  // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "MyUtils.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::MyUtils"

namespace android {
namespace nn {
namespace eden_driver {

void DumpToFile(std::string filename, void* addr, int32_t size) {
    LOGD(EDEN_DRIVER, "Dump addr=[%p] with size=[%d] to [%s] files...\n", addr, size, filename.c_str());
    std::ofstream dumpFile(filename, std::ofstream::binary);
    char* data = static_cast<char*>(addr);
    dumpFile.write(data, size);
    dumpFile.close();
    LOGD(EDEN_DRIVER, "Dump addr=[%p] with size=[%d] to [%s] files...Done!\n", addr, size, filename.c_str());
}

void LoadFromFile(std::string filename, void* addr, int32_t size) {
    LOGD(EDEN_DRIVER, "Load [%s] files to addr=[%p] with size=[%d]...\n", filename.c_str(), addr, size);
    std::ifstream dumpFile(filename, std::ifstream::binary);

    // get size of file
    dumpFile.seekg(0, dumpFile.end);
    long sizeOfFile = dumpFile.tellg();
    dumpFile.seekg(0);

    LOGD(EDEN_DRIVER, "File size is [%ld], Buffer size is [%d]\n", sizeOfFile, size);

    char* data = static_cast<char*>(addr);
    dumpFile.read(data, size);
    dumpFile.close();
    if (dumpFile) {
        LOGD(EDEN_DRIVER, "Load [%s] files to addr=[%p] with size=[%d]...Done!\n", filename.c_str(), addr, size);
    } else {
        LOGE(EDEN_DRIVER, "Error: only %td could be read\n", dumpFile.gcount());
    }
}

void DumpToStdio(void* addr, int32_t size) {
    LOGD(EDEN_DRIVER, "Dump addr=[%p], size=[%d] on stdio...\n", addr, size);
    unsigned char* chPtr = reinterpret_cast<unsigned char*>(addr);
    for (int32_t idx = 0; idx < size; idx++) {
        LOGD(EDEN_DRIVER, "%d \n", static_cast<int32_t>(chPtr[idx]));
    }
    LOGD(EDEN_DRIVER, "Dump addr=[%p], size=[%d] on stdio...Done!\n", addr, size);
}

void DumpToStdio(void* addr, int32_t size, DATA_TYPE& dataType) {
    LOGD(EDEN_DRIVER, "Dump addr=[%p], size=[%d], dataType=[%d] on stdio...\n", addr, size, static_cast<int32_t>(dataType));

    switch (dataType) {
    case DATA_TYPE::FLOAT32:
    {
        float* floatPtr = reinterpret_cast<float*>(addr);
        int32_t iterEnd = size/sizeof(float);
        if (iterEnd > 16) {
            LOGD(EDEN_DRIVER, "# of data is more than 16, so just show 16 items...\n");
            iterEnd = 16;
        }
        for (int32_t idx = 0; idx < iterEnd; idx++) {
            LOGD(EDEN_DRIVER, "%f \n", floatPtr[idx]);
        }
        break;
    }
    case DATA_TYPE::QUANT8:
    {
        uint8_t* chPtr = reinterpret_cast<uint8_t*>(addr);
        int32_t iterEnd = size/sizeof(uint8_t);
        if (iterEnd > 16) {
            LOGD(EDEN_DRIVER, "# of data is more than 16, so just show 16 items...\n");
            iterEnd = 16;
        }
        for (int32_t idx = 0; idx < iterEnd; idx++) {
            LOGD(EDEN_DRIVER, "%d \n", static_cast<int32_t>(chPtr[idx]));
        }
        break;
    }
    case DATA_TYPE::RELAXED_FLOAT32:
    {
        uint16_t* u16Ptr = reinterpret_cast<uint16_t*>(addr);
        int32_t iterEnd = size/sizeof(uint16_t);
        if (iterEnd > 16) {
            LOGD(EDEN_DRIVER, "# of data is more than 16, so just show 16 items...\n");
            iterEnd = 16;
        }
        for (int32_t idx = 0; idx < iterEnd; idx++) {
            LOGD(EDEN_DRIVER, "%d \n", u16Ptr[idx]);
        }
        break;
    }
    case DATA_TYPE::INT32:
    {
        int32_t* intPtr = reinterpret_cast<int32_t*>(addr);
        int32_t iterEnd = size/sizeof(int32_t);
        if (iterEnd > 16) {
            LOGD(EDEN_DRIVER, "# of data is more than 16, so just show 16 items...\n");
            iterEnd = 16;
        }
        for (int32_t idx = 0; idx < iterEnd; idx++) {
            LOGD(EDEN_DRIVER, "%d \n", intPtr[idx]);
        }
        break;
    }
    default:
        LOGE(EDEN_DRIVER, "dataType=%d is not supported\n", static_cast<int32_t>(dataType));
        break;
    }

    LOGD(EDEN_DRIVER, "Dump addr=[%p], size=[%d], dataType=[%d] on stdio...Done!\n", addr, size, static_cast<int32_t>(dataType));
}

void ShowArray(const char* msg, const int32_t* arr, int32_t len) {
    std::string str(msg);
    str += std::string("( ");
    for (int32_t idx = 0; idx < len; idx++) {
        str += std::to_string(arr[idx]) + std::string(" ");
    }
    str += std::string(")");
    LOGD(EDEN_GTEST, "%s\n", str.c_str());
}

void ShowVector(const char* msg, const std::vector<int32_t>& arr) {
    std::string str(msg);
    str += std::string("( ");
    for (auto num : arr) {
        str += std::to_string(num) + std::string(" ");
    }
    str += std::string(")");
    LOGD(EDEN_GTEST, "%s\n", str.c_str());
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

