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
 * @file    ResourceManager.h
 * @brief   This is ResourceManager class file.
 * @details This header defines ResourceManager class.
 *          This class is implementing resource managing such as caching etc.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#ifndef DRIVER_RESOURCEMANAGER_H_
#define DRIVER_RESOURCEMANAGER_H_

#include <map>
#include <cstdint>  // int32_t

//#include "EmulDataType.h"
#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc

namespace android {
namespace nn {
namespace eden_driver {

class ResourceManager {
 public:
    int32_t loadCachedModel(const V1_3::Model& model, V1_3::IPreparedModel** preparedModel);
    int32_t storeModelToCache(const V1_3::Model& model, V1_3::IPreparedModel* preparedModel);

 private:
    bool isCachedModel(const V1_3::Model& model, int64_t& signature);
    void calcSignature(const V1_3::Model& model, int64_t& signature);
    bool getCachedModel(int64_t signature, V1_3::IPreparedModel** preparedModel);
    bool setCachedModel(int64_t signature, V1_3::IPreparedModel* preparedModel);

    std::map<int32_t, V1_3::IPreparedModel*> mapFromSignatureToCachedModel_;
};

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // DRIVER_RESOURCEMANAGER_H_

