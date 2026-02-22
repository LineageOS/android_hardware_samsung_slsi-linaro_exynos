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
 * @file    MyUtils.h
 * @brief   This is unitily functions to be used for EdenDriver.
 * @details This header defines utility functions to be used for EdenDriver.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_MYUTILS_H_
#define DRIVER_MYUTILS_H_

#include <string>

#include "Common.h"

namespace android {
namespace nn {
namespace eden_driver {

void DumpToFile(std::string filename, void* addr, int32_t size);
void LoadFromFile(std::string filename, void* addr, int32_t size);

void DumpToStdio(void* addr, int32_t size);
void DumpToStdio(void* addr, int32_t size, DATA_TYPE& dataType);

void ShowArray(const char* msg, const int32_t* arr, int32_t len);
void ShowVector(const char* msg, const std::vector<int32_t>& arr);

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_MYUTILS_H_
