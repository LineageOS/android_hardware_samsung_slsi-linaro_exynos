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
 * @file    ErrorHandler.cpp
 * @brief   This is ErrorHandler class file.
 * @details This header defines ErrorHandler class.
 *          This class is implementing error handling.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#include "ErrorHandler.h"

namespace android {
namespace nn {
namespace eden_driver {

/**
 * @brief Handle invalid model error
 * @details This function handles invalid model error.
 * @param void
 * @returns return code
 */
int32_t ErrorHandler::handleInvalidModel() {
    return 0;
}

/**
 * @brief Handle emergency recovery error
 * @details This function handles emergency recovery error.
 * @param void
 * @returns return code
 */
int32_t ErrorHandler::handleEmergencyRecovery() {
    return 0;
}

/**
 * @brief Handle SRAM full error
 * @details This function handles SRAM full error.
 * @param void
 * @returns return code
 */
int32_t ErrorHandler::handleSRAMFull() {
    return 0;
}

/**
 * @brief Handle service died
 * @details This function handles service died.
 * @param void
 * @returns return code
 */
int32_t ErrorHandler::handleServiceDied() {
    return 0;
}

/**
 * @brief Handle timeout
 * @details This function handles timeout.
 * @param void
 * @returns return code
 */
int32_t ErrorHandler::handleTimeout() {
    return 0;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

