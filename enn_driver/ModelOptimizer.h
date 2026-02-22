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
 * @file    ModelOptimizer.h
 * @brief   This is ModelOptimizer class file.
 * @details This header defines ModelOptimizer class.
 *          This class is implementing error handling.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_MODELOPTIMIZER_H_
#define DRIVER_MODELOPTIMIZER_H_

#include <cstdint>  // int32_t

#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc

namespace android {
namespace nn {
namespace eden_driver {

class ModelOptimizer {
 public:
    int32_t optimize(const V1_3::Model& model);
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_MODELOPTIMIZER_H_

