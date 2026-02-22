// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

/**
 * @brief This is a header file for the Inference Engine plugin C++ API
 * @file ie_plugin_cpp.hpp
 */
#pragma once

#include <map>
#include <string>
#include <memory>

#include "ie_plugin.hpp"
#include "details/ie_exception_conversion.hpp"
#include "cpp/ie_executable_network.hpp"
#include "ie_plugin_ptr.hpp"
#include "ie_cnn_network.h"
#ifdef ENABLE_CV_API
    #include "../../../cv/include/seva.hpp"
#endif


namespace InferenceEngine {

/**
 * @brief This class is a C++ API wrapper for IInferencePlugin.
 * It can throw exceptions safely for the application, where it is properly handled.
 */
class InferencePlugin {
    InferenceEnginePluginPtr actual;

  public:
    /** @brief A default constructor */
    InferencePlugin() = default;

    /**
     * @brief Constructs a plugin instance from the given pointer.
     */
    explicit InferencePlugin(const InferenceEnginePluginPtr &pointer) : actual(pointer) {}

    /**
     * @brief Wraps original method
     * IInferencePlugin::GetVersion
     */
    const Version *GetVersion() {
        const Version *versionInfo = nullptr;
        actual->GetVersion(versionInfo);
        if (versionInfo == nullptr) {
            THROW_IE_EXCEPTION << "Unknown device is used";
        }
        return versionInfo;
    }

    /**
     * @deprecated Use InferencePlugin::LoadNetwork(ICNNNetwork &, const std::map<std::string, std::string> &)
     * @brief Wraps original method IInferencePlugin::LoadNetwork(ICNNNetwork &, ResponseDesc *)
     */
    INFERENCE_ENGINE_DEPRECATED
    void LoadNetwork(ICNNNetwork &network) {
        IE_SUPPRESS_DEPRECATED_START
        CALL_STATUS_FNC(LoadNetwork, network);
        IE_SUPPRESS_DEPRECATED_END
    }

    /**
     * @brief Wraps original method
     * IInferencePlugin::LoadNetwork(IExecutableNetwork::Ptr&, ICNNNetwork&, const std::map<std::string, std::string> &, ResponseDesc*).
     */
    ExecutableNetwork LoadNetwork(ICNNNetwork &network, const std::map<std::string, std::string> &config) {
        IExecutableNetwork::Ptr ret;
        CALL_STATUS_FNC(LoadNetwork, ret, network, config);
        return ExecutableNetwork(ret, actual);
    }

    ExecutableNetwork LoadNetwork(ICNNNetwork &network, const std::map<std::string, std::string> &config,
                                const TBlob<int8_t>::Ptr& weight, const TBlob<int8_t>::Ptr& bias) {
        IExecutableNetwork::Ptr ret;
        CALL_STATUS_FNC(LoadNetwork, ret, network, config, weight, bias);
        return ExecutableNetwork(ret, actual);
    }

    /**
     * @brief Wraps original method
     * IInferencePlugin::LoadNetwork(IExecutableNetwork::Ptr&, ICNNNetwork&, const std::map<std::string, std::string> &, ResponseDesc*).
     */
    /*
    ExecutableNetwork LoadNetwork(CNNNetwork network, const std::map<std::string, std::string> &config) {
        IExecutableNetwork::Ptr ret;
        CALL_STATUS_FNC(LoadNetwork, ret, network, config);
        if (ret.get() == nullptr) THROW_IE_EXCEPTION << "Internal error: pointer to executable network is null";
        return ExecutableNetwork(ret, actual);
    }*/

#ifdef ENABLE_CV_API
    /**
     * @brief Wraps original method
     * IInferencePlugin::LoadNetwork(IExecutableNetwork::Ptr&, Graph&, const std::map<std::string, std::string> &, ResponseDesc*).
     */
    ExecutableNetwork LoadNetwork(seva::graph::Graph &graph, const std::map<std::string, std::string> &config) {
        IExecutableNetwork::Ptr ret;
        CALL_STATUS_FNC(LoadNetwork, ret, graph, config);
        return ExecutableNetwork(ret, actual);
    }
#endif

    /**
     * @deprecated Use IExecutableNetwork to create IInferRequest.
     * @brief Wraps original method IInferencePlugin::Infer(const BlobMap&, BlobMap&, ResponseDesc *)
     */
    INFERENCE_ENGINE_DEPRECATED
    void Infer(const BlobMap &input, BlobMap &result) {
        IE_SUPPRESS_DEPRECATED_START
        CALL_STATUS_FNC(Infer, input, result);
        IE_SUPPRESS_DEPRECATED_END
    }

    /**
     * @deprecated Use IInferRequest to get performance counters
     * @brief Wraps original method IInferencePlugin::GetPerformanceCounts
     */
    INFERENCE_ENGINE_DEPRECATED
    std::map<std::string, InferenceEngineProfileInfo> GetPerformanceCounts() const {
        std::map<std::string, InferenceEngineProfileInfo> perfMap;
        IE_SUPPRESS_DEPRECATED_START
        CALL_STATUS_FNC(GetPerformanceCounts, perfMap);
        IE_SUPPRESS_DEPRECATED_END
        return perfMap;
    }

    /**
     * @brief Wraps original method
     * IInferencePlugin::AddExtension
     */
    void AddExtension(InferenceEngine::IExtensionPtr extension) {
        CALL_STATUS_FNC(AddExtension, extension);
    }

    /**
     * @brief Wraps original method
     * IInferencePlugin::SetConfig
     */
    void SetConfig(const std::map<std::string, std::string> &config) {
        CALL_STATUS_FNC(SetConfig, config);
    }

    /**
     * @deprecated Use InferencePlugin::QueryNetwork(const ICNNNetwork &, const std::map<std::string, std::string> &, QueryNetworkResult &) const
     * @brief Wraps original method
     * IInferencePlugin::QueryNetwork(const ICNNNetwork&, QueryNetworkResult& ) const
     */
    INFERENCE_ENGINE_DEPRECATED
    void QueryNetwork(const ICNNNetwork &network, QueryNetworkResult &res) const {
        QueryNetwork(network, { }, res);
    }

    /**
     * @brief Wraps original method
     * IInferencePlugin::QueryNetwork(const ICNNNetwork&, const std::map<std::string, std::string> &, QueryNetworkResult&) const
     */
    void QueryNetwork(const ICNNNetwork &network, const std::map<std::string, std::string> &config, QueryNetworkResult &res) const {
        actual->QueryNetwork(network, config, res);
        if (res.rc != OK) THROW_IE_EXCEPTION << res.resp.msg;
    }

    /**
     * @brief Converts InferenceEngine to InferenceEnginePluginPtr pointer
     * @brief Returns wrapped object
     */
    operator InferenceEngine::InferenceEnginePluginPtr() {
        return actual;
    }

    /**
     * @brief Shared pointer on InferencePlugin object
     */
    using Ptr = std::shared_ptr<InferencePlugin>;
};
}  // namespace InferenceEngine
