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
 * @file    NpucIrConverter.h
 * @brief   This is NpucIrConverter class file.
 * @details This header defines NpucIrConverter class.
 *          This class is implementing handshaking with compiler component.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_NPUCIRCONVERTER_H_
#define DRIVER_NPUCIRCONVERTER_H_

#include <vector>
#include <cstdint>  // int32_t

#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc

#include "NNNetworkBuilder.h"  // NPUC::NNNetwork

namespace android {
namespace nn {
namespace eden_driver {

class NpucIrConverter {
 public:
    int32_t convertToNNNetwork(const V1_3::Model& model, ModelInfo& modelInfo, const std::vector<int32_t>& operationList, std::shared_ptr<NPUC::NNNetwork>& network);
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_NPUCIRCONVERTER_H_

