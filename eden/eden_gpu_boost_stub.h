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
 * @file    eden_gpu_boost_stub.h
 * @brief   This is EDEN GPU boost stub
 * @details This file invokes GPU Boost service.
 * @version 1.0 Basic scenario support.
 */

#ifndef _EDEN_GPU_BOOST_H_
#define _EDEN_GPU_BOOST_H_

namespace eden {
namespace rt {

/**
 *  @brief Run GPU Boost for EDEN
 *  @details This API function boosts GPU models.
 *  @param void
 *  @returns int
 */
int RunGpuBoost(void);

} // namespace rt
} // namespace eden
#endif
