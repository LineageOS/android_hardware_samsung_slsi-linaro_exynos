#ifndef __SEVA_KERNEL_HPP
#define __SEVA_KERNEL_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "types.hpp"

#ifdef ENABLE_KLM
namespace OfiKernelLibraryManager {
class KernelInfo;
}
#endif

/// Main namespace for Samsung Exynos Vision APIs.
namespace seva {
namespace system {
class Kernel {
  public:
    Kernel();
#ifdef ENABLE_KLM
    Kernel(int index, OfiKernelLibraryManager::KernelInfo *ki);
#endif
    virtual ~Kernel();
    int GetId() const { return mId; }
    void SetName(const char *name) { mName = name; }
    const char *GetName() const { return mName.c_str(); }
    bool IsUserKernel() const { return mIsUserKernel; }
    bool IsComposite() const { return mIsComposite; }
    void SetNumOfInputs(std::size_t &&numOfInputs) { mNumOfInputs = numOfInputs; }
    std::size_t GetNumOfInputs() const { return mNumOfInputs; }
    std::size_t GetNumOfMinimumInputs() const;
    void SetNumOfOutputs(std::size_t &&numOfOutputs) { mNumOfOutputs = numOfOutputs; }
    std::size_t GetNumOfOutputs() const { return mNumOfOutputs; }
    std::size_t GetNumOfMinimumOutputs() const;
    void SetUserParamSize(const std::size_t &size) { mParamSize = size; }
    std::size_t GetUserParamSize() const { return mParamSize; }
#ifdef ENABLE_KLM
    void SetInputFlags(std::vector<int> &flags) { mInputFlags = flags; }
    std::vector<int> GetInputFlags() const { return mInputFlags; }
    void SetOutputFlags(std::vector<int> &flags) { mOutputFlags = flags; }
    std::vector<int> GetOutputFlags() const { return mOutputFlags; }
#endif
    bool HasUserParam() const { return (mParamSize > 0); }
#ifdef ENABLE_KLM
    static KernelPtr Build(int index, OfiKernelLibraryManager::KernelInfo *ki);
#endif
    static KernelPtr InvalidKernel();
    int ApplyCompKernel(graph::Graph &graph, graph::NodeHandle nh);
#ifdef ENABLE_KLM
    OfiKernelLibraryManager::KernelInfo *GetKernelLib() { return mKernelInfo; }
#endif

  private:
    friend class Platform;
    void DumpKernel() const;

  private:
    int mId;
    std::string mName;
    bool mIsUserKernel;
    bool mIsComposite;
    std::size_t mNumOfInputs;
    std::size_t mNumOfOutputs;
    std::size_t mParamSize;
#ifdef ENABLE_KLM
    std::vector<int> mInputFlags;
    std::vector<int> mOutputFlags;
    OfiKernelLibraryManager::KernelInfo *mKernelInfo;
#endif
    static const char *TAG;
};
}  // namespace system
}  // namespace seva
#endif
