/*
 * Copyright (C) 2019 Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#include "DebugUtil.hpp"

#include <cutils/properties.h>
#include <fstream>

#define OFI_ANN_COMPILER_DEBUG_FILEOUT_NAME "vendor.ofi.anngc.fileout"

void _saveString(std::string path, std::string content) {
    if (_isDbgFileOut()) {
        std::fstream out;
        out.open(path, std::ios_base::out | std::ios_base::trunc);
        if (out.is_open()) {
            out << content;
            out.close();
        }
    }
}
void _saveBuffer(std::string path, const char *buffer, unsigned int size) {
    if (_isDbgFileOut()) {
        std::fstream out;
        out.open(path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
        if (out.is_open()) {
            out.write(buffer, size);
            out.close();
        }
    }
}

void _saveCvtOut(std::string xml, BlobPair bins) {
    std::string path = "/data/test/";
    _saveString(path + "model__test.xml", xml);
    _saveBuffer(
        path + "model__q_weight_bin_test.raw", bins.first->cbuffer().as<const char *>(), bins.first->byteSize());
    _saveBuffer(
        path + "model__q_bias_bin_test.raw", bins.second->cbuffer().as<const char *>(), bins.second->byteSize());
}

void _saveGcOut(const char *buffer, unsigned int size) {
    std::string path = "/data/test/";
    _saveBuffer(path + "compiled_graph_out.cgo", buffer, size);
}

bool _isDbgFileOut() {
    char prop[PROPERTY_VALUE_MAX];
    int ret = property_get(OFI_ANN_COMPILER_DEBUG_FILEOUT_NAME, prop, "0");
    if (ret < 0) {
        return false;
    } else {
        return (strncmp(prop, "1", 2) == 0);
    }
}
