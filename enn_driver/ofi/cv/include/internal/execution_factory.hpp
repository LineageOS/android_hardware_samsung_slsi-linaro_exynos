#ifndef __SEVA_EXECUTION_FACTORY_HPP
#define __SEVA_EXECUTION_FACTORY_HPP

#include "executable.hpp"

namespace seva {
namespace system {
class ExecutionFactory {
  public:
    static Executable *Creator(const char *name);

  private:
    static const char *TAG;
};
}  // namespace system
}  // namespace seva
#endif
