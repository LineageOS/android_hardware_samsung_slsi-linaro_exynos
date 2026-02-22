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
 * @file    eden_gb_api.cpp
 * @brief   This is EDEN NN API implementation.
 * @details This is the implementation of EDEN NN API.
 * @version 1.0 Basic scenario support & modified
 *          Supported functions are as below.
 *          NnRet BoostNN(void)
 */

// gb
#include "include/eden_gb_api.h"  // GPU Boost API
#include "eden_gb.h"              // EdenGB

#include "log.h"                   // LOGD, LOGI, LOGE
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "NN::EdenGBApi"  // Change defined LOG_TAG on log.h for purpose.

using namespace eden::nn;

EdenGB* edenGB = nullptr;

/**
 *  @brief Boost EDEN GPU.
 *  @details This function boosts the EDEN NN for GPU models.
 *  @param void
 *  @returns return code
 */
NnRet BoostNN(void) {
    if (edenGB == nullptr) {
        edenGB = new EdenGB();
    }
    return edenGB->Boost();
}

