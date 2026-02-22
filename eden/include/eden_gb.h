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
 * @file    eden_gb.h
 * @brief   This is EDEN GB class
 * @details This header defines EDEN GB class.
 *          This class is implementing the Eden GB API.
 * @version 1.0 Basic scenario support.
 *          Supported functions are as below.
 *          NnRet Boost(void)
 */

#ifndef NN_EDEN_GB_H_
#define NN_EDEN_GB_H_

#include <cstdint>        // int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t

// nn
#include "eden_nn_types.h"  // NnRet

namespace eden {
namespace nn {
/**
 *  EdenGB class. This class implements EDEN GB API.
 */
class EdenGB {
 public:
  /**
   * @brief EdenGB constructor
   * @details Initialize internal variables and resources
   * @param void
   */
    EdenGB(void) {}
  /**
   * @brief EdenNN destructor
   * @details Release internal resourses
   * @param void
   */
    virtual ~EdenGB(void) {}

  /**
   * @brief Boost EDEN NN.
   * @details This function boosts the EDEN GB for GPU Models.
   * @param void
   * @returns return code
   */
    NnRet Boost(void);
};

}  // namespace nn
}  // namespace eden

#endif  // NN_EDEN_GB_H_

