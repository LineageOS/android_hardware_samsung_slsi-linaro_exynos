/*
 * INTEL CONFIDENTIAL
 * Copyright 2017 Intel Corporation.
 *
 * The source code contained or described herein and all documents
 * related to the source code ("Material") are owned by Intel Corporation
 * or its suppliers or licensors. Title to the Material remains with
 * Intel Corporation or its suppliers and licensors. The Material may
 * contain trade secrets and proprietary and confidential information
 * of Intel Corporation and its suppliers and licensors, and is protected
 * by worldwide copyright and trade secret laws and treaty provisions.
 * No part of the Material may be used, copied, reproduced, modified,
 * published, uploaded, posted, transmitted, distributed, or disclosed
 * in any way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other
 * intellectual property right is granted to or conferred upon you by
 * disclosure or delivery of the Materials, either expressly, by implication,
 * inducement, estoppel or otherwise. Any license under such intellectual
 * property rights must be express and approved by Intel in writing.
 *
 * Include any supplier copyright notices as supplier requires Intel to use.
 *
 * Include supplier trademarks or logos as supplier requires Intel to use,
 * preceded by an asterisk. An asterisked footnote can be added as follows:
 * *Third Party trademarks are the property of their respective owners.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter
 * this notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#define LOG_TAG "IRDoc"
#include "IRDocument.h"
#include <fstream>
#include <locale>
#include "IRLayers.h"
#include "cnn_network_impl.hpp"
// #define NNLOG
#ifdef NNLOG
#include <android/log.h>
#include <log/log.h>
#endif
#include "ie_common.h"
#include <iostream>

using namespace IRBuilder;
using namespace std;
using namespace InferenceEngine;

#define PADSIZE(n, p) ((p - (n % p)) % p)

class InternalNetworkImpl : public InferenceEngine::details::CNNNetworkImpl {
  public:
    InternalNetworkImpl() {}
    InternalNetworkImpl(const std::string netName) : InternalNetworkImpl() {
        setPrecision(Precision::UNSPECIFIED);
        setName(netName);
    }

    void remove(const string &layer_name) { _layers.erase(layer_name); }

    bool hasLayer(const string &lname) const { return _layers.find(lname) != _layers.end(); }

    void addData(const DataPtr &data) { _data[data->getName()] = data; }

    using InferenceEngine::details::CNNNetworkImpl::addOutput;

    void addOutput(const DataPtr &data) {
        addData(data);
        auto docdatadims = data->getTensorDesc().getDims();
        _outputData[data->getName()] = data;
    }
};

IRDocument::IRDocument(const std::string &cs) : _name(cs) { network = new InternalNetworkImpl(cs); }

IRDocument::~IRDocument() {
    delete network;
    network = nullptr;
}

void IRDocument::add(const IRLayer &ir_layer) {
    network->addLayer(ir_layer);
    _layers.resize(_layers.size() + 1);
    _layers[_layers.size() - 1] = ir_layer;
}

bool IRDocument::shouldRemove(const IRLayer &l) {
    if (l->type != "Reshape")
        return false;
    return l->insData[0].lock()->getDims() == output(l)->getDims();
}

void IRDocument::process(const IRLayer &layer) {
    if (network->hasLayer(layer->name))
        return;
    add(layer);
    for (auto o : layer->outData) {
        network->addData(o);
        for (auto l : o->inputTo)
            process(l.second);
    }
}

void IRDocument::optimize() {
    for (auto it = _layers.begin(); it != _layers.end();) {
        auto l = *it;
        if (shouldRemove(l)) {
            // l-in -> (l,l-out) -> (b-in list) ===> a-out -> (b-in list)
            auto lin = l->input();
            auto lout = output(l);

            lin->inputTo.erase(l->name);

            auto lout_targets = lout->inputTo;
            for (auto i : lout_targets) {
                lin->inputTo[i.first] = i.second;
                // reaplce target input data from lout to lin
                for (auto &tar_inp : i.second->insData) {
                    if (tar_inp.lock() == lout) {
                        tar_inp = lin;
                        break;
                    }
                }
            }

            it = _layers.erase(it);
            network->remove(l->name);
        } else {
            ++it;
        }
    }
}

void IRDocument::build() {
    if (_processed)
        return;
    network->setPrecision(mPrecision);
    InputsDataMap inputs;
    network->getInputsInfo(inputs);
    for (auto i : inputs) {
        for (auto l : i.second->getInputData()->inputTo)
            process(l.second);
    }

    for (auto l : network->allLayers()) {
        process(l.second);
    }
    optimize();
    _processed = true;
}

void IRDocument::setPrecision(InferenceEngine::Precision precision) { mPrecision = precision; }

InferenceEngine::ICNNNetwork *IRDocument::buildNetwork() {
    build();
    return network;
}
InferenceEngine::ICNNNetwork *IRDocument::getNetwork() { return network; }
/**
 * \brief save a blob to IR
 * \param binFile
 * \param blob
 * \param layer
 * \param name
 */

void IRDocument::saveBlobToIR(size_t &bin_size,
                              const /*IRBlob::Ptr*/ InferenceEngine::Blob::Ptr &blob,
                              pugi::xml_node &layer,
                              const std::string &name) {
    const float *fp = blob->cbuffer().as<const float *>();

    auto fit = _segmentsMap.find(fp);
    bool newBlob = fit == _segmentsMap.end();
    size_t offset;
    if (newBlob) {
        offset = bin_size;
        //_segmentsMap[(float*)fp] = offset;
        _segmentsMap[fp] = offset;
    } else {
        offset = fit->second;
    }

    auto node = layer.append_child(name.c_str());
    node.append_attribute("offset").set_value(offset);
    node.append_attribute("size").set_value(blob->byteSize());
    if (newBlob) {
        bin_size += blob->byteSize();
    }

    node.append_attribute("precision").set_value(blob->getTensorDesc().getPrecision().name());
}

void IRDocument::save(std::ostream &xml_os, size_t &bin_size) {
    pugi::xml_document doc;

    build();
    pugi::xml_node root = doc.append_child("net");
    root.append_attribute("name").set_value(_name.c_str());
    root.append_attribute("version").set_value(2);
    root.append_attribute("batch").set_value(1);
    pugi::xml_node layers = root.append_child("layers");
    int id_cnt = 0;
    InputsDataMap netInputs;
    network->getInputsInfo(netInputs);
    int icnt = 0;
    for (auto &kvp : netInputs) {  // todo: consider adding input layer to actual network...
        InferenceEngine::LayerParams prms;
        prms.name = kvp.first;
        CNNLayer::Ptr inputLayer(new CNNLayer(prms));
        inputLayer->type = "Input";
        auto input_data = kvp.second->getInputData();
        const_cast<UserValue &>(input_data->getUserObject()).v_int = icnt++;
        inputLayer->outData.push_back(input_data);
        saveToIR(bin_size, layers, inputLayer);
    }

    for (auto &cnn_layer : _layers) {
        cnn_layer->userValue.v_int = ++id_cnt;
        int pcnt = cnn_layer->insData.size();
        for (auto output : cnn_layer->outData)
            const_cast<UserValue &>(output->getUserObject()).v_int = pcnt++;
        saveToIR(bin_size, layers, cnn_layer);
    }
    pugi::xml_node edgesNode = root.append_child("edges");
    for (auto &kvp : _layers) {
        int pcnt = 0;
        for (auto inputData : kvp->insData) {
            Edge edge;
            edge.to.lid = kvp->userValue.v_int;
            edge.to.pid = pcnt++;
            bool b = inputData.lock()->creatorLayer.expired();
            edge.from.lid = b ? 0 : inputData.lock()->creatorLayer.lock()->userValue.v_int;
            edge.from.pid = const_cast<UserValue &>(inputData.lock()->getUserObject()).v_int;

            _edges.push_back(edge);

            auto node = edgesNode.append_child("edge");
            node.append_attribute("from-layer").set_value(edge.from.lid);
            node.append_attribute("from-port").set_value(edge.from.pid);
            node.append_attribute("to-layer").set_value(edge.to.lid);
            node.append_attribute("to-port").set_value(edge.to.pid);
        }
    }
    doc.save(xml_os);
}

void IRDocument::crateDotFile(std::ostream &dot) const {
    dot << "digraph g {\n\tgraph[rankdir = \"LR\"];" << std::endl;

    for (auto &kvp : network->allLayers()) {
        saveLayerToDot(dot, kvp.second);
    }
    dot << std::endl << std::endl;
    for (auto &kvp : _edges) {
        dot << "\t\"layer_" << kvp.from.lid << "\":p" << kvp.from.pid << " -> \"layer_" << kvp.to.lid << "\":p"
            << kvp.to.pid << " [];" << std::endl;
    }
    dot << "}" << std::endl;
}

InferenceEngine::InputInfo::Ptr IRDocument::createInput(const std::string &name, const TensorDims &dims) const {
    Layout layout;
    if (dims.size() == 4)
        layout = NCHW;
    else if (dims.size() == 2)
        layout = InferenceEngine::Layout::NC;
    else if (dims.size() == 1)
        layout = InferenceEngine::Layout::C;
    else
        layout = InferenceEngine::Layout::ANY;

    // std::cout << "createInput input data dims[0] " << dims[0] << "dims[1]" << dims[1] << std::endl;
    TensorDesc td(mPrecision, dims, layout);

    auto inputData = std::make_shared<InferenceEngine::Data>(name, td);
    InferenceEngine::InputInfo::Ptr info(new InferenceEngine::InputInfo());

    info->setInputData(inputData);

    Precision inputPrecision = info->getPrecision();
    if (inputPrecision == Precision::FP16) {
        info->setPrecision(Precision::FP32);
    }

    network->setInputInfo(info);

    auto indatadims = inputData->getDims();
    /*
    std::cout << " createInput input data indatadims[0] " << indatadims[0] << "indatadims[1]"
              << indatadims[1] << std::endl;
    */
    return info;
}

void IRDocument::addOutput(const DataPtr &src) { network->addOutput(src); }

/**
 * \brief save output port to IR
 * \param parent
 * \param port
 */
void IRDocument::saveOutputToIR(pugi::xml_node &parent, const DataPtr &port) {
    auto node = parent.append_child("port");
    node.append_attribute("id").set_value(const_cast<UserValue &>(port->getUserObject()).v_int);
    // node.append_attribute("buffer").set_value(reinterpret_cast<size_t>(buffer) & 0x00FFFFFF);
    if (!port->inputTo.empty()) {
        std::string comment = "connected to ";
        for (auto peer : port->inputTo)
            comment += ", " + peer.first;
        node.append_child(pugi::xml_node_type::node_comment).set_value(comment.c_str());
    }
    auto dims = port->getDims();
    for (auto d : dims) {
        node.append_child("dim").text().set(d);
    }
}

/**
 * \brief save input port to IR
 * \param parent
 * \param index
 * \param port
 */
void IRDocument::saveInputToIR(pugi::xml_node &parent, int index, const DataPtr &port) {
    auto node = parent.append_child("port");
    node.append_attribute("id").set_value(index);
    auto peer = port->creatorLayer.lock();
    if (peer) {
        auto comment = "connected to " + peer->name;
        node.append_child(pugi::xml_node_type::node_comment).set_value(comment.c_str());
    }
    auto dims = port->getDims();
    for (auto d : dims) {
        node.append_child("dim").text().set(d);
    }
}

/**
 * \brief Save layer to IR
 * \param binFile
 * \param parent
 * \param irLayer
 */
void IRDocument::saveToIR(size_t &bin_size, pugi::xml_node &parent, const IRLayer &irLayer) {
    auto layer = parent.append_child("layer");
    layer.append_attribute("name").set_value(irLayer->name.c_str());
    layer.append_attribute("type").set_value(irLayer->type.c_str());
    layer.append_attribute("id").set_value(irLayer->userValue.v_int);
    layer.append_attribute("precision").set_value(irLayer->precision == Precision::FP16 ? "FP16" : "FP32");

    if (!irLayer->params.empty()) {
        auto attr = layer.append_child("data");  // todo: need to check for type and overide it
        for (auto &kvp : irLayer->params) {
            attr.append_attribute(kvp.first.c_str()).set_value(kvp.second.c_str());
        }
        attr.append_attribute("fl_in").set_value(1);
        attr.append_attribute("fl_out").set_value(1);
    }
    if (irLayer->type == "Input" || irLayer->type == "Output") {
        // ColinToDo : Need to fill data with proper values.
        auto attr = layer.append_child("data");
        attr.append_attribute("fl_in").set_value(1);
        attr.append_attribute("fl_out").set_value(1);
        attr.append_attribute("mean_value").set_value("[128.0, 128.0, 128.0]");
        attr.append_attribute("scale_value").set_value("[0.0000000]");
    }
    if (!irLayer->insData.empty()) {
        /*
        auto attr = layer.append_child("data");
        attr.append_attribute("fl_in").set_value("1,1");
        attr.append_attribute("fl_out").set_value("1");
        */

        auto inputs = layer.append_child("input");
        for (int i = 0; i < irLayer->insData.size(); ++i) {
            saveInputToIR(inputs, i, irLayer->insData[i].lock());
        }
    }
    if (!irLayer->outData.empty()) {
        auto outputs = layer.append_child("output");
        for (auto &inp : irLayer->outData) {
            saveOutputToIR(outputs, inp);
        }
    }

    for (auto blob : irLayer->blobs) {
        auto fb = blob.second;
        saveBlobToIR(bin_size, fb, layer, blob.first);
    }
}

void IRDocument::saveLayerToDot(std::ostream &dot, const IRLayer &irLayer) const {
    /*
    "layer_4" [
    label = "name| type | <f2> |-1"
    shape = "record"
    ];
    */
    dot << "\t\"layer_" << irLayer->userValue.v_int << "\" [ label = \"" << _name << "| type: " << irLayer->type;
    int pid = 0;
    for (auto &in : irLayer->insData) {
        dot << "| ";
        auto dims = in.lock()->getDims();
        dot << "<p" << (pid++) << "> " << dims[0];
        for (int i = 1; i < dims.size(); ++i)
            dot << ", " << dims[i];
    }
    for (auto &p : irLayer->outData) {
        dot << "| ";
        auto dims = p->getDims();
        dot << "<p" << const_cast<UserValue &>(p->getUserObject()).v_int << "> " << dims[0];
        for (int i = 1; i < dims.size(); ++i)
            dot << ", " << dims[i];
    }
    dot << "\"";
    dot << "\t\tshape = \"record\" ];" << std::endl;
}
void IRDocument::setName(const char *name) {
    _name = name;
    network->setName(_name);
}

#define PADTOSIZE(x, s) ((s - (x % s)) % s)

void IRDocument::ShuffleWeight(unsigned char *s_weight_ptr,
                               const unsigned char *weight_ptr,
                               int kernel_w,
                               int kernel_h,
                               int channel,
                               int kernel_n) {
#ifdef NNLOG
    ALOGD("%s", __func__);
#endif
    const uint32_t SHUFFLE_C = 4;
    const uint32_t SHUFFLE_K = 16;
    uint32_t k_size = channel * kernel_w * kernel_h;
    uint32_t c_size = kernel_w * kernel_h;
    uint32_t i = 0;
    int32_t kernel_n_t = ((kernel_n + 15) >> 4) << 4;
    for (uint32_t k = 0; k < kernel_n_t; k += SHUFFLE_K) {
        for (uint32_t c = 0; c < channel; c += SHUFFLE_C) {
            for (uint32_t h = 0; h < kernel_h; ++h) {
                for (uint32_t w = 0; w < kernel_w; ++w) {
                    int k_end = ((kernel_n_t - k) >= SHUFFLE_K) ? (k + SHUFFLE_K) : kernel_n_t;
                    for (uint32_t k_i = k; k_i < k_end; ++k_i) {
                        for (uint32_t c_i = 0; c_i < SHUFFLE_C; ++c_i) {
                            if ((c + c_i >= channel) || (k_i >= kernel_n)) {
                                s_weight_ptr[i++] = 0;
                            } else {
                                s_weight_ptr[i++] = weight_ptr[k_i * k_size + (c + c_i) * c_size + h * kernel_w + w];
                            }
                        }
                    }
                }
            }
        }
    }
}

void IRDocument::ShuffleWeight_PadtoC16(unsigned char *s_weight_ptr,
                                        const unsigned char *weight_ptr,
                                        int kernel_w,
                                        int kernel_h,
                                        int channel,
                                        int kernel_n) {
#ifdef NNLOG
    ALOGD("%s", __func__);
#endif
    const uint32_t SHUFFLE_C = 4;
    const uint32_t SHUFFLE_K = 16;

    uint32_t k_size = channel * kernel_w * kernel_h;
    uint32_t c_size = kernel_w * kernel_h;
    uint32_t i = 0;
    uint32_t channel_t = ((channel + 15) >> 4) << 4;
    int32_t kernel_n_t = ((kernel_n + 15) >> 4) << 4;

    for (uint32_t k = 0; k < kernel_n_t; k += SHUFFLE_K) {
        for (uint32_t c = 0; c < channel_t; c += SHUFFLE_C) {
            for (uint32_t h = 0; h < kernel_h; ++h) {
                for (uint32_t w = 0; w < kernel_w; ++w) {
                    int k_end = ((kernel_n_t - k) >= SHUFFLE_K) ? (k + SHUFFLE_K) : kernel_n_t;
                    for (uint32_t k_i = k; k_i < k_end; ++k_i) {
                        for (uint32_t c_i = 0; c_i < SHUFFLE_C; ++c_i) {
                            if ((c + c_i >= channel) || (k_i >= kernel_n)) {
                                s_weight_ptr[i++] = 0;
                            } else {
                                s_weight_ptr[i++] = weight_ptr[k_i * k_size + (c + c_i) * c_size + h * kernel_w + w];
                            }
                        }
                    }
                }
            }
        }
    }
}

void IRDocument::ShuffleWeight_DW(unsigned char *s_weight_ptr,
                                  const unsigned char *weight_ptr,
                                  int kernel_w,
                                  int kernel_h,
                                  int kernel_n) {
#ifdef NNLOG
    ALOGD("%s", __func__);
#endif
    uint32_t SHUFFLE_K = 16;
    uint32_t k_size = kernel_w * kernel_h;
    uint32_t i = 0;
    for (uint32_t k = 0; k < kernel_n; k += SHUFFLE_K) {
        for (uint32_t h = 0; h < kernel_h; ++h) {
            for (uint32_t w = 0; w < kernel_w; ++w) {
                int k_end = ((kernel_n - k) >= SHUFFLE_K) ? (k + SHUFFLE_K) : kernel_n;
                for (uint32_t k_i = k; k_i < k_end; k_i += 16) {  // channel should be multiple of 16
                    for (uint32_t k_j = 0; k_j < 16; ++k_j) {
                        if (k_i + k_j >= k_end) {
                            s_weight_ptr[i++] = 0;
                        } else {
                            s_weight_ptr[i++] = weight_ptr[(k_i + k_j) * k_size + h * kernel_w + w];
                        }
                    }
                }
            }
        }
    }
}

size_t IRDocument::padForAlign(unsigned char *out, size_t wrote, size_t block) {
    int padding_size = PADTOSIZE(wrote, block);
    memset(reinterpret_cast<char *>(out), 0x0, padding_size);
    return padding_size;
}

size_t IRDocument::writeNameNLength(unsigned char *out, std::string name, int length) {
    out += name.copy(reinterpret_cast<char *>(out), name.length(), 0);
    out += padForAlign(out, name.length(), 124);
    memcpy(out, &length, sizeof(int));
    return 128;
}

BlobPair IRDocument::saveShuffledBlobs() {
    calcShuffledBlobSize();

    size_t wsize = 0;
    for (auto elem : _shuffledWeightSizeMap) {
        wsize += 128 + elem.second + PADSIZE(elem.second, 64);
    }
    SizeVector dims_w;
    dims_w.push_back(wsize);
    InferenceEngine::TBlob<int8_t>::Ptr weightsPtr(
        new InferenceEngine::TBlob<int8_t>(InferenceEngine::Precision::I8, InferenceEngine::C, dims_w));
    weightsPtr->allocate();

    size_t bsize = 0;
    for (auto elem : _shuffledBiasSizeMap) {
        bsize += 128 + elem.second + PADSIZE(elem.second, 64);
    }

    SizeVector dims_b;
    dims_b.push_back(bsize);
    InferenceEngine::TBlob<int8_t>::Ptr biasesPtr(
        new InferenceEngine::TBlob<int8_t>(InferenceEngine::Precision::I8, InferenceEngine::C, dims_b));
    biasesPtr->allocate();

    saveShuffledBlobs(weightsPtr, biasesPtr);
    return std::make_pair(weightsPtr, biasesPtr);
}

void IRDocument::calcShuffledBlobSize() {
    for (auto &cnn_layer : _layers) {
        if (cnn_layer->type == "Convolution") {
            for (auto blob : cnn_layer->blobs) {
                if (blob.first == "weights") {
                    size_t n = blob.second->getTensorDesc().getDims()[0];
                    size_t c = blob.second->getTensorDesc().getDims()[1];
                    size_t h = blob.second->getTensorDesc().getDims()[2];
                    size_t w = blob.second->getTensorDesc().getDims()[3];
                    size_t s_filter_length = 0;
                    bool first_layer = false;
                    if (!cnn_layer->insData[0].lock()->creatorLayer.expired()) {
                        if (cnn_layer->insData[0].lock()->creatorLayer.lock()->type == "Input") {
                            first_layer = true;
                        }
                    } else {
                        first_layer = true;
                    }
#ifdef NNLOG
                    ALOGD("[%s] first_layer?%c", cnn_layer->name.c_str(), first_layer ? 'Y' : 'N');
#endif
                    if (first_layer && (c <= 4)) {
                        s_filter_length = w * h * (((c + 3) >> 2) << 2) * (((n + 15) >> 4) << 4);
                    } else {
                        s_filter_length = w * h * (((c + 15) >> 4) << 4) * (((n + 15) >> 4) << 4);
                    }
                    _shuffledWeightSizeMap[cnn_layer->name] = s_filter_length;
                } else if (blob.first == "biases") {
                    size_t bias_length = blob.second->getTensorDesc().getDims()[0];
                    size_t padding_length = PADTOSIZE(bias_length, 16);
                    size_t total_length = (bias_length + padding_length) * (sizeof(int) + sizeof(char));
                    _shuffledBiasSizeMap[cnn_layer->name] = total_length;
                }
            }
        } else if (cnn_layer->type == "FullyConnected") {
            for (auto blob : cnn_layer->blobs) {
                if (blob.first == "weights") {
                    size_t out_num = blob.second->getTensorDesc().getDims()[0];
                    size_t in_num = blob.second->getTensorDesc().getDims()[1];
                    size_t s_filter_length = (((in_num + 15) >> 4) << 4) * (((out_num + 15) >> 4) << 4);
                    _shuffledWeightSizeMap[cnn_layer->name] = s_filter_length;
                } else if (blob.first == "biases") {
                    size_t bias_length = blob.second->getTensorDesc().getDims()[0];
                    size_t padding_length = PADTOSIZE(bias_length, 16);
                    size_t total_length = (bias_length + padding_length) * (sizeof(int) + sizeof(char));
                    _shuffledBiasSizeMap[cnn_layer->name] = total_length;
                }
            }
        }
        if (cnn_layer->type == "Concat") {
            size_t length = static_cast<size_t>(cnn_layer->insData.size());
            _shuffledBiasSizeMap[cnn_layer->name] = length;
        }
    }
}

void IRDocument::saveShuffledBlobs(InferenceEngine::TBlob<int8_t>::Ptr weight,
                                   InferenceEngine::TBlob<int8_t>::Ptr bias) {
    unsigned char *wptr = weight->buffer().as<unsigned char *>();
    unsigned char *bptr = bias->buffer().as<unsigned char *>();
    for (auto &cnn_layer : _layers) {
        auto layerName = cnn_layer->name;
        if (cnn_layer->type == "Convolution") {
            for (auto blob : cnn_layer->blobs) {
                if (blob.first == "weights") {
                    size_t length = _shuffledWeightSizeMap[layerName];
                    // name and length
                    wptr += writeNameNLength(wptr, layerName, static_cast<int>(length));
                    size_t n = blob.second->getTensorDesc().getDims()[0];
                    size_t c = blob.second->getTensorDesc().getDims()[1];
                    size_t h = blob.second->getTensorDesc().getDims()[2];
                    size_t w = blob.second->getTensorDesc().getDims()[3];
                    size_t group = 1;
                    auto pGroup = cnn_layer->params.find("group");
                    memset(wptr, 0x00, length);
                    if (pGroup != cnn_layer->params.end()) {
                        char *end;
                        group = (size_t)(strtol(pGroup->second.c_str(), &end, 10));
                    }
                    // Shuffle weights and write to buffer
#ifdef NNLOG
                    ALOGI("ShuffleWeight : layer:%s, group:%zu, c:%zu", cnn_layer->name.c_str(), group, c);
#endif
                    if ((group == c) && (c != 1)) {  // DW conv
                        ShuffleWeight_DW(wptr, blob.second->cbuffer().as<const unsigned char *>(), w, h, c);
                    } else {
                        bool first_layer = false;
                        if (!cnn_layer->insData[0].lock()->creatorLayer.expired()) {
                            if (cnn_layer->insData[0].lock()->creatorLayer.lock()->type == "Input") {
                                first_layer = true;
                            }
                        } else {
                            first_layer = true;
                        }
                        if (first_layer && (c <= 4)) {
                            ShuffleWeight(wptr, blob.second->cbuffer().as<const unsigned char *>(), w, h, c, n);
                        } else {
                            ShuffleWeight_PadtoC16(
                                wptr, blob.second->cbuffer().as<const unsigned char *>(), w, h, c, n);
                        }
                    }
                    wptr += length;

                    // 64 byte align padding
                    wptr += padForAlign(wptr, length, 64);
                } else if (blob.first == "biases") {
                    size_t length = _shuffledBiasSizeMap[layerName];
                    // name and length
                    bptr += writeNameNLength(bptr, layerName, static_cast<int>(length));

                    size_t bias_length = blob.second->getTensorDesc().getDims()[0];
                    size_t padding_length = PADTOSIZE(bias_length, 16);

                    // bias data
                    memcpy(bptr, blob.second->cbuffer().as<const char *>(), blob.second->byteSize());
                    bptr += blob.second->byteSize();
                    // padding to 16
                    memset(bptr, 0x0, padding_length * sizeof(int));
                    bptr += padding_length * sizeof(int);

                    // accum shift data
                    memset(bptr, 0x0, bias_length);  // ColinToDo : Need to save proper accurshift values
                    bptr += bias_length;
                    // padding to 16
                    memset(bptr, 0x0, padding_length * sizeof(char));
                    bptr += padding_length * sizeof(char);

                    // 64 byte align padding
                    bptr += padForAlign(bptr, static_cast<int>(length), 64);
                }
            }
        } else if (cnn_layer->type == "FullyConnected") {
            for (auto blob : cnn_layer->blobs) {
                if (blob.first == "weights") {
                    // ColinToDo : Need to Implement for DwConv
                    size_t length = _shuffledWeightSizeMap[layerName];
                    // name and length
                    wptr += writeNameNLength(wptr, layerName, static_cast<int>(length));
                    memset(wptr, 0x00, length);

                    size_t out_num = blob.second->getTensorDesc().getDims()[0];
                    size_t in_num = blob.second->getTensorDesc().getDims()[1];
                    // Shuffle weights and write to buffer
                    ShuffleWeight_PadtoC16(
                        wptr, blob.second->cbuffer().as<const unsigned char *>(), 1, 1, in_num, out_num);
                    wptr += length;

                    // 64 byte align padding
                    wptr += padForAlign(wptr, length, 64);
                } else if (blob.first == "biases") {
                    size_t length = _shuffledBiasSizeMap[layerName];
                    // name and length
                    bptr += writeNameNLength(bptr, layerName, static_cast<int>(length));

                    size_t bias_length = blob.second->getTensorDesc().getDims()[0];
                    size_t padding_length = PADTOSIZE(bias_length, 16);

                    // bias data
                    memcpy(bptr, blob.second->cbuffer().as<const char *>(), blob.second->byteSize());
                    bptr += blob.second->byteSize();
                    // padding to 16
                    memset(bptr, 0x0, padding_length * sizeof(int));
                    bptr += padding_length * sizeof(int);

                    // accum shift data
                    memset(bptr, 0x0, bias_length);  // ColinToDo : Need to save proper accurshift values
                    bptr += bias_length;
                    // padding to 16
                    memset(bptr, 0x0, padding_length * sizeof(char));
                    bptr += padding_length * sizeof(char);

                    // 64 byte align padding
                    bptr += padForAlign(bptr, static_cast<int>(length), 64);
                }
            }
        }
        if (cnn_layer->type == "Concat") {
            size_t length = _shuffledBiasSizeMap[layerName];
            // name and length
            bptr += writeNameNLength(bptr, layerName, static_cast<int>(length));
            // accum shift data
            memset(bptr, 0x0, length);  // ColinToDo : Need to save proper accurshift values
            bptr += length;

            // 64 byte align padding
            bptr += padForAlign(bptr, length, 64);
        }
    }
}
