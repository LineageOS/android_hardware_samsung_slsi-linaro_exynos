#ifndef __SEVA_BUFFER_HPP
#define __SEVA_BUFFER_HPP


#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include "device.hpp"
#include "memory.hpp"

/**
 * @file buffer.hpp
 * @brief Buffer class header file
 */

namespace seva {
namespace graph {
/**
 * @cond
 * internal
 */
enum seva_dim_e {
    /* Channel dimension. */
    SEVA_DIM_C = 0,
    /* X dimension. */
    SEVA_DIM_X,
    /* Y dimension. */
    SEVA_DIM_Y,
    /* Max dimension value. */
    SEVA_DIM_MAX,
};

class MetaData {
  public:
    MetaData();
    unsigned int GetNumDataInfos() { return mNumDataInfos; }
    unsigned int GetNumDimensions() { return mNumDimensions; }
    unsigned int GetNumPlanes() { return mNumPlanes; }
    std::vector<std::vector<unsigned int>> &GetStrides() { return mStrides; }
    std::vector<std::vector<unsigned int>> &GetDimensions() { return mDimensions; }
    void SetNumDimensions(unsigned int dim);
    void SetNumDataInfos(unsigned int ninfos);
    void SetNumPlanes(unsigned int planes);
    void SetDimension(std::size_t plane, std::size_t dim, size_t val);
    void SetStride(std::size_t plane, std::size_t dim, size_t val);

  private:
    unsigned int mNumDataInfos;
    unsigned int mNumDimensions;
    unsigned int mNumPlanes;
    std::vector<std::vector<unsigned int>> mStrides;
    std::vector<std::vector<unsigned int>> mDimensions;
};

class RingBuffer;
class Graph;
/**
 * @endcond
 */

/**
 * @brief represents a buffer object.
 *
 * A base class of Image, Pyramid, Array, DataBuffer class.
 * All derived classes are used as a input or output buffer of node.
 */
class Buffer {
  public:
    /**
     * @cond
     * internal
     */
    enum class Type {
        /* Image type buffer. */
        IMAGE,
        /* Array type buffer. */
        ARRAY,
        /* DataBuffer type buffer. */
        DATABUFFER,

        TENSOR,
        /* Pyramid type buffer. */
        PYRAMID,
    };

    static const int MAX_PLANES = 4;

    Buffer();
    virtual ~Buffer();
    Buffer::Type GetType();

    /* Check whether memory is allocated, if not allocate memory
       This function is usually called when user's first trial to data access
    */
    void AllocateMemory(void);
    void FreeMemory(void);
    unsigned int GetAllocatedMemorySize(unsigned int index) {
        if (index < mMemoryAllocatedSize.size()) {
            return mMemoryAllocatedSize[index];
        }
        return 0;
    }
    unsigned int GetAllocatedMemorySize();

    MetaData &GetMetaData() { return mMetaData; }
    void *GetBufferPtr();
    system::MemoryInfo GetBufferInfoPtr();
    void *GetPlaneBufferPtr(unsigned int plane);
    void SetInternal() { mIsExternalIO = false; }
    void SetExternal() { mIsExternalIO = true; }
    bool IsExternalIO() { return (mIsExternalIO == true); }
    bool IsWithinRingBuffer() const { return mRingBuffer != nullptr; }
    RingBuffer *GetRingBuffer() { return mRingBuffer; }
    bool IsVirtual() { return mIsVirtual; }

  protected:
    friend class RingBuffer;
    friend class Graph;
    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;
    static std::shared_ptr<Buffer> CloneMeta(Buffer &source);
    void SetRingBuffer(RingBuffer *ptr);
    bool GetChanged() { return mChanged; }
    void SetChanged(bool changed) { mChanged = changed; }
    /**
     * @endcond
     */

  protected:
    MetaData mMetaData;
    system::MemoryInfo mDataInfo;
    /* temporary ptr member. will be replaced by ION buffer */
    // uint8_t * mPtrs[MAX_PLANES];
    Buffer::Type mType;
    // the buffer that should be allocated externally or internally
    bool mIsExternalIO;
    /* Is memory allocated for this buffer */
    std::vector<unsigned int> mMemoryAllocatedSize;
    bool mIsVirtual;
    bool mAccessOK;
    RingBuffer *mRingBuffer;
    static const char *TAG;
    /* buffer is changed after verified */
    bool mChanged;
    int mFd;
    unsigned int mOffset;
    unsigned int mSize;
    void *mVa;
};

/**
 * @cond
 * internal
 */
class UserParam {
  public:
    /**
     * @brief Constructor.
     *
     * Create an UserParam object with given size and ptr.
     */
    template <class T> UserParam(std::size_t size, T &&ptr) : mChanged(false) {
        mPtr = (system::MemoryInfo)system::Device::GetInstance().GetEngine()->CreateMemory(
            (int)size, system::Memory::Type::DEPEND_PLATFORM);
        if (mPtr && mPtr->mVa) {
            std::memcpy(mPtr->mVa, &ptr, size);
        }
    }

    /**
     * @brief Deconstructor.
     */
    ~UserParam();

    /**
     * @brief Returns UserParam's data area pointer.
     */
    uint8_t *GetPtr() { return (uint8_t *)mPtr->mVa; }

    /**
     * @brief Returns UserParam's pointer.
     */
    system::MemoryInfo GetUserParamPtr() { return mPtr; }

    /**
     * @brief Returns UserParam's data size.
     */
    std::size_t GetSize() {
        if (mPtr)
            return (std::size_t)mPtr->mSize;
        else
            return 0;
    }

    /**
     * @brief Copy from given user buffer to user param's buffer.
     * @param [in] size size of given buffer.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool ReadFromBuffer(uint32_t size, void *ptr);
    bool GetChanged();
    void SetChanged(bool changed);

  private:
    UserParam(const UserParam &) = delete;
    UserParam &operator=(const UserParam &) = delete;
    system::MemoryInfo mPtr;
    static const char *TAG;
    bool mChanged;
};
/**
 * @endcond
 */
}  // namespace graph
}  // namespace seva
#endif
