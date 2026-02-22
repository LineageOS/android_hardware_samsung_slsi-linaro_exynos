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
 * @file    ModelOptimizer.cpp
 * @brief   This is ModelOptimizer class file.
 * @details This header defines ModelOptimizer class.
 *          This class is implementing optimization for a given model
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#include "ModelOptimizer.h"

namespace android {
namespace nn {
namespace eden_driver {

/**
 * @brief Optimize model
 * @details This function tries to optimize a model to make it NPU-friendly.
 *          It migth include several model modification without changing original behavior.
 *          e.g. operation reordering to be compiled in efficient way etc.
 * @param[in] model Android NN Model to be optimized
 * @returns return code
 */
int32_t ModelOptimizer::optimize(const V1_3::Model& /*model*/) {
    return 0;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

