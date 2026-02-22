#ifndef __SEVA_PLATFORM_HPP
#define __SEVA_PLATFORM_HPP

#include <sys/types.h>
#include <unistd.h>

#include <mutex>
#include <string>

#include "executable.hpp"
#include "kernel_manager.hpp"
#include "types.hpp"

namespace seva {
namespace graph {
enum class Target {
    DSP = 0,
    CPU = 1,
    ANY,
};

class Platform {
  public:
    static Platform &GetPlatform();
    Executable *GetEngine();
    size_t GetNumberOfTargets();

  private:
    friend class Node;
    friend class OfiEngine;
    friend class SimulationEngine;
    friend class Graph;
    Platform();
    ~Platform();
    Platform(const Platform &) = delete;
    Platform &operator=(const Platform &) = delete;
    KernelPtr GetKernel(const char *kernelName);
    KernelManager *GetKernelManager() const { return mKernelManager; }

  private:
    int mSwVersion;
    pid_t mPid;
    Executable *mExec;
    KernelManager *mKernelManager;
    std::vector<std::string> mDeviceList;
    std::mutex mOfiKlmLock;
    static const char *TAG;
};
}  // namespace graph
}  // namespace seva
#endif
