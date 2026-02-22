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
 * @file    SchedulePolicy.h
 * @brief   This is SchedulePolicy class file.
 * @details This header defines SchedulePolicy class.
 *          This class is implementing schedule policy.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 *          yeongjun.kim (yj0576.kim@samsung.com)
 */

#ifndef DRIVER_SCHEDULEPOLICY_H_
#define DRIVER_SCHEDULEPOLICY_H_

#include <cstdint>  // int32_t

namespace android {
namespace nn {
namespace eden_driver {

class SchedulePolicy {
 public:
    int32_t decidePriority(void* data1, void* data2, int32_t& priority);
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_SCHEDULEPOLICY_H_

