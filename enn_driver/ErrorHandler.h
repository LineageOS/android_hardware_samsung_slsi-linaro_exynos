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
 * @file    ErrorHandler.h
 * @brief   This is ErrorHandler class file.
 * @details This header defines ErrorHandler class.
 *          This class is implementing error handling.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_ERRORHANDLER_H_
#define DRIVER_ERRORHANDLER_H_

#include <cstdint>  // int32_t

namespace android {
namespace nn {
namespace eden_driver {

class ErrorHandler {
 public:
    int32_t handleInvalidModel();
    int32_t handleEmergencyRecovery();
    int32_t handleSRAMFull();
    int32_t handleServiceDied();
    int32_t handleTimeout();
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_ERRORHANDLER_H_

