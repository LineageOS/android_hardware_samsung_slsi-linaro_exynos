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
 * @file    SchedulePolicy.cpp
 * @brief   This is SchedulePolicy class file.
 * @details This header defines SchedulePolicy class.
 *          This class is implementing schedule policy.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#include "SchedulePolicy.h"

namespace android {
namespace nn {
namespace eden_driver {

/**
 * @brief Determine priority refer to the current policy
 * @details This function determines a priority number for a given Request.
 *          This priority dicision depends on the SchedulePolicy.
 * @param[in] preparedModel IPreparedModel to be executed
 * @param[in] request Request to be executed
 * @param[out] priority priority number for this execution
 * @return error code
 */
int32_t SchedulePolicy::decidePriority(void* /*data1*/, void* /*data2*/, int32_t& priority) {
    static uint32_t priorityNumber = 0;

    priority = priorityNumber++;

    return 0;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

