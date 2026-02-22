#ifndef __SEVA_DEVICE_HPP
#define __SEVA_DEVICE_HPP

#include <sys/types.h>
#include <unistd.h>

#include <future>
#include <map>
#include <mutex>
#include <string>

#include "executable.hpp"
#include "kernel_manager.hpp"
#include "types.hpp"

/**
 * @file device.hpp
 * @brief Device class header file
 */

namespace seva {
/// Main namespace for System APIs.
namespace system {
/**
 * @brief represents the target.
 */
enum class Target {
    DSP = 0,
    CPU = 1,
    ANY,
};

/**
 * @brief represents a device obejct.
 *
 * Can be used to construct, manipulate, verify, and run a graph.
 */

class Device {
  public:
    /**
     * @cond
     * internal
     */
    /**
     * @brief represents the mode of battery.
     */
    enum class BatteryMode {
        HIGH_PERFORMANCE,
        BALANCED,
        POWER_SAVER,
    };
    /**
     * @endcond
     */

  public:
    /**
     * @brief Get a device instance.
     * @return the instance of the device
     */
    static Device &GetInstance();
    /**
     * @brief Initialize a device.
     * @return the result of initializing a device
     */
    bool Init();
    /**
     * @brief De-initialize a device.
     * @return the result of de-initializing a device
     */
    bool Deinit();
    /**
     * @brief Load a graph on the device
     * @param [in] graph the instance of the graph
     * @return the result of loading a graph on device
     */
    bool LoadGraph(seva::graph::Graph &graph);
    /**
     * @brief Synchronously run the graph on the device
     * @param [in] graph the instance of the graph
     * @return the result of running the graph on device
     */
    bool RunGraph(seva::graph::Graph &graph);
    /**
     * @brief Asynchronously run the graph on the device
     * @param [in] graph the instance of the graph
     * @param [out] requestedId the id for identifying asynchronously running graph
     * @return the result of asynchronously running the graph on device
     */
    bool AsyncRunGraph(seva::graph::Graph &graph, long int *requestedId);
    /**
     * @brief Wait for the result of asynchronously runing the graph on the device
     * @param [in] graph the instance of the graph
     * @return the result of asynchronously running the graph on device
     */
    bool AsyncWaitForGraph(seva::graph::Graph &graph, long int requestedId);
    /**
     * @brief Unload a graph on the device
     * @param [in] graph the instance of the graph
     * @return the result of unloading a graph on device
     */
    bool UnloadGraph(seva::graph::Graph &graph);
    /**
     * @brief Set a presets for a solution
     * @param [in] solutionName name of solution
     * @return the result of setting a presets
     */
    bool SetSolutionPresets(const char *solutionName);
    /**
     * @brief Unset a presets for a solution
     * @param [in] solutionName name of solution
     * @return the result of unsetting a presets
     */
    bool UnsetSolutionPresets(const char *solutionName);

    /**
     * @cond
     * internal
     */
    Executable *GetEngine();

    // This API is not yet implemented.
    /**
     * @brief Set battery mode
     * @param [in] mode battery mode
     * @return the result of setting a battery mode
     */
    bool SetBatteryMode(BatteryMode mode);

    /**
     * @endcond
     */

  private:
    /**
     * @cond
     * internal
     */
    friend class seva::graph::Node;
    friend class OfiEngine;
    friend class SimulationEngine;
    friend class seva::graph::Graph;
    /**
     * @endcond
     */

    Device();
    ~Device();
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;
    KernelPtr GetKernel(const char *kernelName);
    KernelManager *GetKernelManager() const { return mKernelManager; }
    bool PrepareGraph(seva::graph::Graph &graph);

  private:
    int mSwVersion;
    pid_t mPid;
    Executable *mExec;
    KernelManager *mKernelManager;
    std::vector<int> mLoadedGraphLists;
    std::map<long int, std::future<bool>> mAsync;
    std::mutex mOfiKlmLock;
    BatteryMode mBatteryMode;
    static const char *TAG;
};
}  // namespace system
}  // namespace seva
#endif
