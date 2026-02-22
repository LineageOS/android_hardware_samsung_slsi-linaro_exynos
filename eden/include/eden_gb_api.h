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
 * @file    eden_gb_api.h
 * @brief   This is EDEN GB interface
 * @details This header defines EDEN GB class.
 *          This class is implementing the Eden GB API.
 * @version 1.0 Basic scenario support.
 *          Supported functions are as below.
 *          NnRet Boost(void)
 */
#ifndef NN_INCLUDE_EDEN_GB_API_H_
#define NN_INCLUDE_EDEN_GB_API_H_

#include <cstdint>  // int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t
// nn
#include "eden_nn_types.h"  // NnRet

#ifdef __cplusplus
extern "C" {
#endif

    // Public functions

  /**
   * @brief Boost EDEN NN.
   * @details This function boosts the EDEN NN for GPU Models.
   * @param void
   * @returns return code
   */
    NnRet BoostNN(void);
#ifdef __cplusplus
}
#endif
#endif  // NN_INCLUDE_EDEN_GB_API_H_


