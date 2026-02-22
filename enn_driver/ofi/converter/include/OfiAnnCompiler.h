
#ifndef OFI_CONVERTER_H
#define OFI_CONVERTER_H

#include <string>

#include "HalInterfaces.h"  // IDevice, Return, ErrorStatus, IPreparedModelCallback, getCapabilities_cb etc
// #include <exynosNN_model_converter.h>

namespace android {
namespace nn {
namespace eden_driver {
namespace ofi {

class OfiAnnCompiler {
  public:
    OfiAnnCompiler();
    virtual ~OfiAnnCompiler();
    bool isSupportedModel(const V1_3::Model &model);
    bool Compile(const V1_3::Model &model, char *&buffer, unsigned int &size);
};

}  // namespace ofi
}  // namespace eden_driver
}  // namespace nn
}  // namespace android

#endif  // OFI_CONVERTER_H
