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
 * @file    eden_gb.cpp
 * @brief   This is EDEN GB class
 * @details This header defines EDEN GB class.
 *          This class is implementing the Eden NN framework.
 * @version 1.0 Basic scenario support.
 */

// gb
#include "eden_gb.h"      // EdenGB
#include "eden_gpu_boost_stub.h"

#include "log.h"               // LOGD, LOGI, LOGE
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "NN::EdenGB"  // Change defined LOG_TAG on log.h for purpose.

namespace eden {
namespace nn {

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// public functions ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Boost EDEN NN.
 * @details This function boosts  the EDEN NN for GPU Models.
 * @param void
 * @returns return code
 */
NnRet EdenGB::Boost(void) {
    LOGD(EDEN_NN, "(+)\n");
    int rtRet = eden::rt::RunGpuBoost();
    if (rtRet != 0) {
        LOGE(EDEN_NN, "(-) RET_ERROR_ON_BOOST, (rtRet=[%d])!\n", rtRet);
    }
    LOGD(EDEN_NN, "(-)\n");
    return RET_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// private functions //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
}  // namespace nn
}  // namespace eden

