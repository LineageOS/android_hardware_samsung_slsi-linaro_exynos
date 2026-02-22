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
 * @file    PrePostProcessor.h
 * @brief   This is PrePostProcessor class file.
 * @details This header defines PrePostProcessor class.
 *          This class is implementing pre/post processing such as data layout change.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_PREPOSTPROCESSOR_H_
#define DRIVER_PREPOSTPROCESSOR_H_

#include <cstdint>  // int32_t

#include "Common.h"  // DISALLOW_COPY_AND_ASSIGN

namespace android {
namespace nn {
namespace eden_driver {

class PrePostProcessor {
 public:
    static PrePostProcessor* getInstance() {
        static PrePostProcessor instance_;
        return &instance_;
    }
    int32_t convertDataLayoutFromNCHWToNHWC(void* addr, int32_t size,
                                            int32_t number, int32_t channel,
                                            int32_t height, int32_t width,
                                            DATA_TYPE dataType,
                                            int32_t offset = 0);
    int32_t convertDataLayoutFromNHWCToNCHW(void* addr, int32_t size,
                                            int32_t number, int32_t channel,
                                            int32_t height, int32_t width,
                                            DATA_TYPE dataType,
                                            int32_t offset = 0);
    int32_t convertDataLayoutFromNHWCToCNHW(void* addr, int32_t size,
                                            int32_t number, int32_t channel,
                                            int32_t height, int32_t width,
                                            DATA_TYPE dataType,
                                            int32_t offset = 0);

 private:
    PrePostProcessor() {}
    PrePostProcessor(const PrePostProcessor&);
    void operator=(const PrePostProcessor&);
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_PREPOSTPROCESSOR_H_
