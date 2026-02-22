
#include <cstdint>  // uint32_t
#include <memory>   // nullptr
#include <sstream>

#include <log/log.h>

#include <ofi_gc_if.hpp>
#include <ofi_gc_api.hpp>

#include "DebugUtil.hpp"
#include "OfiAnnCompiler.h"
#include "IRConverter.h"

namespace android {
namespace nn {
namespace eden_driver {
namespace ofi {

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "OfiAnnCompiler"
#define LOGD(fmt, ...) ALOGD("[OFI][%s] %s %d: " fmt, LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_ERR(msg) ALOGE("[OFI][%s] %s %d: %s", LOG_TAG, __FUNCTION__, __LINE__, msg);

inline bool GraphCompile(std::string xml, BlobPair blobs, std::string target, char *&buffer, unsigned int &size);

OfiAnnCompiler::OfiAnnCompiler() {}
OfiAnnCompiler::~OfiAnnCompiler() {}

bool OfiAnnCompiler::isSupportedModel(const V1_3::Model &model) {
    LOGD("%s(+)", __func__);

    IRConverter converter(model);
    bool supported = converter.parse();

    LOGD("%s(-) %c", __func__, supported ? 'Y' : 'N');
    return supported;
}

bool OfiAnnCompiler::Compile(const V1_3::Model &model, char *&buffer, unsigned int &size) {
    LOGD("%s(+)", __func__);

    IRConverter converter(model);
    // Parse model
    if (converter.parse() != true) {
        LOG_ERR(converter.getError().c_str());
        return false;
    }

    // Generate IR
    std::string modelXml = converter.getIRXml();
    BlobPair blobPtr = converter.getShuffledBlobs();

    // Save IR for debug
    _saveCvtOut(modelXml, blobPtr);

    // Compile graph with IR
    if (GraphCompile(modelXml, blobPtr, "990", buffer, size) != true) {
        return false;
    }

    // Save CGO for debug
    _saveGcOut(buffer, size);

    LOGD("%s(-)", __func__);
    return true;
}

bool GraphCompile(std::string xml, BlobPair blobs, std::string target, char *&buffer, unsigned int &size) {
    LOGD("%s(+)", __func__);

    try {
        ::ofi::gc::NnCompiler nnCompiler;
        nnCompiler.setIr(xml);
        nnCompiler.setWeightsBias(blobs.first->cbuffer(), blobs.first->byteSize(),
                blobs.second->cbuffer(), blobs.second->byteSize());

        nnCompiler.setOutput(&buffer, &size);
        nnCompiler.setMode(::ofi::gc::COMPILER_MODE_MEM | ::ofi::gc::COMPILER_MODE_FULL);

        nnCompiler.addConfig({ { ::ofi::gc::KEY_DEVICE_LIST, "{DSP,CPU}" } });
        nnCompiler.addConfig({ { ::ofi::gc::KEY_GRAPH_OUT_CONF, "ALL" } });
        nnCompiler.addConfig({ { ::ofi::gc::KEY_DSP_TARGET, target } });
        if (_isDbgFileOut()) {
            nnCompiler.addConfig({ { ::ofi::gc::KEY_OUTPUT_PATH, "/data/test" } });
        }

        if (nnCompiler.process() == false) {
            LOG_ERR("Compilation Failed!!!!");
            throw "Failed";
        }

        if (buffer == nullptr || size == 0) {
            LOG_ERR("Graph compile failed : Compiled, but cgo was not generated.");
            throw "Failed";
        }

        LOGD("%s(-): Success", __func__);
        return true;
    } catch (const std::exception &error) {
        LOG_ERR(error.what());
    } catch (...) {
        LOG_ERR("exception while compile : Unknown Exception");
    }

    LOG_ERR("GraphCompile(-): Failed");
    return false;
}

}  // namespace ofi
}  // namespace eden_driver
}  // namespace nn
}  // namespace android
