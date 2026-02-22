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
 * @file    ResourceManager.cpp
 * @brief   This is ResourceManager class file.
 * @details This header defines ResourceManager class.
 *          This class is implementing resource managing such as caching etc.
 * @author  minsu.jeon (minsu.jeon@samsung.com)
 */

#include <iostream>
#include <inttypes.h>  // PRId64, PRIu64

#include "log.h"
#include "Utils.h"               // convertToV1_0, convertToV1_1, android::nn::initVLogMask, logModelToInfo, DRIVER

#include "ResourceManager.h"
#include "Common.h"  // RET_OK

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "EdenDriver::ResourceManager"

namespace android {
namespace nn {
namespace eden_driver {

/**
 * @brief Load cached model matched to a given model
 * @details This function tries to find a cached model matching to a given model on cache.
 *          If it finds a cached model for a gvien model, it is loaded on preparedModel.
 *          If it fails, preparedModel is set to nullptr.
 * @param[in] model Android NN Model
 * @param[out] preparedModel IPreparedModel loaded from the cached model
 * @returns return code
 */
int32_t ResourceManager::loadCachedModel(const V1_3::Model& model, V1_3::IPreparedModel** preparedModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    int64_t signature = 0;
    if (isCachedModel(model, signature)) {
        // Now signature has filled by isCachedModel
        return getCachedModel(signature, preparedModel);
    }

    preparedModel = nullptr;

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;
}

/**
 * @brief Store preparedModel to cached model
 * @details This function stores a preparedModel to cached model.
 *          Signature for this model should be generated using a given model.
 * @param[in] model Android NN Model
 * @param[out] preparedModel IPreparedModel to be stored to the cached model
 * @returns return code
 */
int32_t ResourceManager::storeModelToCache(const V1_3::Model& /*model*/, V1_3::IPreparedModel* /*preparedModel*/) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // @todo Below caching logic should be properly modified. Now caching is not used.
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return RET_OK;

#if 0
    int64_t signature = 0;
    calcSignature(model, signature);
    if (setCachedModel(signature, preparedModel) == false) {
        LOGE(EDEN_DRIVER, << "Oops, fail to set model to cache...";
        return FAIL_TO_STORE_MODEL_TOCACHE;
    }

    return RET_OK;
#endif
}

/**
 * @brief Check a given model is cached
 * @details This function checks whether a given model is cached before.
 *          If so, signature matched to a given model is returned via a given parameter.
 * @param[in] model Android NN Model
 * @param[out] signature signature of cached model for a given Android NN Model
 * @returns true(cached), false(not cached)
 */
bool ResourceManager::isCachedModel(const V1_3::Model& /*model*/, int64_t& /*signature*/) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // @todo Below caching logic should be properly modified. Now caching is not used.
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return false;

#if 0
    int64_t tempSignature = 0;
    calcSignature(model, tempSignature);

    auto iter = mapFromSignatureToCachedModel_.find(tempSignature);
    if (iter != mapFromSignatureToCachedModel_.end()) {
        signature = tempSignature;
        return true;
    }

    return false;
#endif
}

/**
 * @brief Calculate a signature for a given model
 * @details This function calculates a signature for a given model.
 *          This signature should be an unique number and calculated to represent for a given model.
 * @param[in] model Android NN Model
 * @param[out] signature signature for a given Android NN Model
 * @returns void
 */
void ResourceManager::calcSignature(const V1_3::Model& model, int64_t& signature) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    // @todo How to calculate an unique signature for a given model?
    // Graph shape and its weights should be considered
    signature = (int64_t)(&model);
    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
}

/**
 * @brief Get cached model matching to a given signature and return it via a given preparedModel
 * @details This function gets a cached model matching to a given signature.
 *          Cached model is returned via a given preparedModel.
 * @param[in] signature signature for a cached model
 * @param[out] preparedModel IPreparedModel loaded from the cached model matched to a given signature
 * @returns true(success), false(fail)
 */
bool ResourceManager::getCachedModel(int64_t signature, V1_3::IPreparedModel** preparedModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    auto iter = mapFromSignatureToCachedModel_.find(signature);
    if (iter != mapFromSignatureToCachedModel_.end()) {
        *preparedModel = iter->second;
        return true;
    }

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return false;
}

/**
 * @brief Set cached model with a signature
 * @details This function sets a preparedModel to cached model with a given signature.
 *          If there is previously cached model for a given signature,
 *          previous model will be replaced to the new one.
 * @param[in] signature signature for a cached model
 * @param[out] preparedModel IPreparedModel stored to the cached model matching to a given signature
 * @returns true(success), false(fail)
 */
bool ResourceManager::setCachedModel(int64_t signature, V1_3::IPreparedModel* preparedModel) {
    LOGD(EDEN_DRIVER, "%s(+)\n", __func__);

    auto iter = mapFromSignatureToCachedModel_.find(signature);
    if (iter != mapFromSignatureToCachedModel_.end()) {
        LOGE(EDEN_DRIVER, "Oops, there is a cached model which has same signature?\n");
        LOGE(EDEN_DRIVER, " (signature=%" PRId64 ")... Then, drop the previous one!\n", signature);
        V1_3::IPreparedModel* prevModel = iter->second;
        mapFromSignatureToCachedModel_.erase(iter);
        delete prevModel;
    }

    mapFromSignatureToCachedModel_.insert(std::pair<int32_t, V1_3::IPreparedModel*>(signature, preparedModel));

    LOGD(EDEN_DRIVER, "%s(-)\n", __func__);
    return true;
}

}  // namespace eden_driver
}  // namespace nn
}  // namespace android

