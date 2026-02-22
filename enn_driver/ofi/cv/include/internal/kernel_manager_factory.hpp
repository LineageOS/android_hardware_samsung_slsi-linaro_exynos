#ifndef __SEVA_KERNEL_MANAGER_FACTORY_HPP
#define __SEVA_KERNEL_MANAGER_FACTORY_HPP

#include "kernel_manager.hpp"

namespace seva {
namespace system {
class KernelManagerFactory {
  public:
    static KernelManager *Creator(const char *name);

  private:
    static const char *TAG;
};
}  // namespace system
}  // namespace seva
#endif
