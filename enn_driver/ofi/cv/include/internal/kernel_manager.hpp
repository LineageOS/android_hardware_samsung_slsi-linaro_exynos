#ifndef __SEVA_KERNEL_MANAGER_HPP
#define __SEVA_KERNEL_MANAGER_HPP

#include "types.hpp"

namespace seva {
namespace system {
class KernelManager {
  public:
    KernelManager() = default;
    virtual ~KernelManager() = default;
    virtual bool Open() = 0;
    virtual bool Close() = 0;
    virtual KernelPtr Get(const char *kernelName) = 0;
};
}  // namespace system
}  // namespace seva
#endif
