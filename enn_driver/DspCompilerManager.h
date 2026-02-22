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
 * @file    DspCompilerManager.h
 * @brief   This is DspCompilerManager class file.
 * @details This header defines DspCompilerManager class.
 *          This class is implementing handshaking with compiler component.
 */

#ifndef DRIVER_DSPCOMPILERMANAGER_H_
#define DRIVER_DSPCOMPILERMANAGER_H_

#include <vector>
#include <cstdint>  // int32_t
#include <memory>   // shared_ptr

#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
#include "Common.h"         // DATA_TYPE

namespace android {
namespace nn {
namespace eden_driver {

class DspCompilerManager {
 public:
    DspCompilerManager();
    int32_t getSupportedOperations(const V1_3::Model& model,
                                   const std::vector<std::shared_ptr<void>>& constraints,
                                   std::vector<bool>& supportedOperations);
    int32_t isSupportedModel(const V1_3::Model& model);
    int32_t compile(const V1_3::Model& model, hal::V1_3::IPreparedModel** preparedModel);

 private:
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_DSPCOMPILERMANAGER_H_
