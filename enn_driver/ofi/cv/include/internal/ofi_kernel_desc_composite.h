// Copyright (C) 2018-2019 Samsung Corporation
// SPDX-License-Identifier: Apache-2.0
//

/**
 * @brief
 * @file ofi_kernel_desc_composite.hpp
 */
#pragma once

#include "array.hpp"
#include "connection.hpp"
#include "databuffer.hpp"
#include "device.hpp"
#include "graph.hpp"
#include "image.hpp"
#include "log.hpp"
#include "pyramid.hpp"
#include "ringbuffer.hpp"
#include "types.hpp"
#include <ofi_kernel_desc.h>

namespace OfiKernelLibraryManager {

/**
 * @struct KernelInfoComposite
 * @brief
 */
class KernelInfoComp : public KernelInfo {
  public:
    /**
     * @brief
     */
    KernelInfoComp() {
        _info.kernel_category = "COMP";
        _info.target_device = "DSP";
    }
    virtual ~KernelInfoComp() {}

    virtual Status GetInTileSize(const KernelArgs &args, SizeList &its) { return OFI_FAIL; }
    virtual Status GetOutTileSize(const KernelArgs &args, SizeInfoList &ots) { return OFI_FAIL; }
    virtual Status GetTmpTileSize(const KernelArgs &args, TempBufInfoList &tts) { return OFI_FAIL; }
    virtual Status GetTileVmShape(const KernelArgs &args, TileVmShapeInfoList &ts) { return OFI_FAIL; }
    virtual Status GetOutputSize(const KernelArgs &args, SizeList &os) { return OFI_FAIL; }
    virtual Status GetInPaddingInfo(const KernelArgs &args, PaddingInfoList &pad) { return OFI_FAIL; }
    virtual Status GetInTileStride(const KernelArgs &args, SizeList &stride) { return OFI_FAIL; }

    virtual Status GenerateGraph(seva::graph::Graph &graph,
                                 const seva::graph::BufferList &inputs,
                                 const seva::graph::BufferList &outputs,
                                 const seva::graph::UserParamPtr &param) {
        return _GenerateGraph(graph, inputs, outputs, param);
    }
    std::shared_ptr<seva::graph::Graph> GetGraph(const seva::graph::BufferList &inputs,
                                                 const seva::graph::BufferList &outputs,
                                                 const seva::graph::UserParamPtr &param) {
        auto grp = std::make_shared<seva::graph::Graph>();
        GenerateGraph(*grp, inputs, outputs, param);
        return grp;
    }
    virtual Status _GenerateGraph(seva::graph::Graph &graph,
                                  const seva::graph::BufferList &inputs,
                                  const seva::graph::BufferList &outputs,
                                  const seva::graph::UserParamPtr &param) = 0;

    std::size_t mNumInputs;
    std::size_t mNumOutputs;
    std::size_t mSizeUserParam;
};

}  // namespace OfiKernelLibraryManager
