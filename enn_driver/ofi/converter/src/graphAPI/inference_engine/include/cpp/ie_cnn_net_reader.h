// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

/**
 * @brief This is a header file for the Network reader class (wrapper) used to build networks from a given IR
 * @file ie_cnn_net_reader.h
 */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "ie_blob.h"
#include "ie_cnn_network.h"
#include "ie_common.h"
#include "ie_icnn_net_reader.h"
#include "details/ie_exception_conversion.hpp"

namespace InferenceEngine {
/**
 * @brief This is a wrapper class used to build and parse a network from the given IR.
 * All the methods here can throw exceptions.
 */
class CNNNetReader {
  public:
    /**
     * @brief A smart pointer to this class
     */
    using Ptr = std::shared_ptr<CNNNetReader>;

    /**
     * @brief A default constructor
     */
    CNNNetReader() : actual(shared_from_irelease(InferenceEngine::CreateCNNNetReader())) {}

    /**
     * @brief Wraps original method
     * ICNNNetReader::ReadNetwork
     */
    void ReadNetwork(const std::string &filepath) {
        CALL_STATUS_FNC(ReadNetwork, filepath.c_str());
    }

    /**
     * @brief Wraps original method
     * ICNNNetReader::ReadNetwork(const void*, size_t, ResponseDesc*)
     */
    void ReadNetwork(const void *model, size_t size) {
        CALL_STATUS_FNC(ReadNetwork, model, size);
    }

    /**
     * @brief Wraps original method
     * ICNNNetReader::SetSCaffeWeights
     */
    void SetSCaffeWeights(const TBlob<int8_t>::Ptr &weights, const TBlob<int8_t>::Ptr &biases) const {
        CALL_STATUS_FNC(SetSCaffeWeights, weights, biases);
    }

    const TBlob<int8_t>::Ptr getWeightbuf(void) {
        return weightBuf;
    }

    void setWeightbuf(TBlob<int8_t>::Ptr& buf) {
        weightBuf = buf;
    }

    const TBlob<int8_t>::Ptr getBiasbuf(void) {
        return biasBuf;
    }

    void setBiasbuf(TBlob<int8_t>::Ptr& buf) {
       biasBuf = buf;
    }


    /**
     * @brief Wraps original method
     * ICNNNetReader::ReadWeightsBias
     */
    void ReadWeightsBias(TBlob<int8_t>::Ptr &weightbufPtr, TBlob<int8_t>::Ptr &biasbufPtr) {
        CALL_STATUS_FNC(ReadWeightsBias, weightbufPtr, biasbufPtr);
        setWeightbuf(weightbufPtr);
        setBiasbuf(biasbufPtr);
    }

    void ReadWeightsBias(const std::string &wfilepath, const std::string &bfilepath) const {
        CALL_STATUS_FNC(ReadWeightsBias, wfilepath.c_str(), bfilepath.c_str());
    }

    /**
    * @brief Gets a copy of built network object
    * @return A copy of the CNNNetwork object to be loaded
     */
    CNNNetwork getNetwork() {
        // network obj are to be updated upon this call
        if (network.get() == nullptr) {
            try {
                network.reset(new CNNNetwork(actual));
            } catch (...) {
                THROW_IE_EXCEPTION << "Could not allocate memory";
            }
        }
        (network.get())->setWeightbuf(getWeightbuf());
        (network.get())->setBiasbuf(getBiasbuf());
        return *network.get();
    }

    /**
     * @brief Wraps original method
     * ICNNNetReader::getDescription
     */
    std::string getDescription() const {
        CALL_STATUS_FNC_NO_ARGS(getDescription);
        return resp.msg;
    }

    /**
     * @brief Wraps original method
     * ICNNNetReader::getName
     */
    std::string getName() const {
        char name[64];
        CALL_STATUS_FNC(getName, name, sizeof(name) / sizeof(*name));
        return name;
    }

    /**
     * @brief Wraps original method
     * ICNNNetReader::getVersion
     */
    int getVersion() const {
        CALL_FNC_NO_ARGS(getVersion);
    }

  private:
    std::shared_ptr<ICNNNetReader> actual;
    std::shared_ptr<CNNNetwork> network;
    TBlob<int8_t>::Ptr weightBuf = nullptr;
    TBlob<int8_t>::Ptr biasBuf = nullptr;
};
}  // namespace InferenceEngine
