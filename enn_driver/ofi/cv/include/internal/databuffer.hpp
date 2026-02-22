#ifndef __SEVA_DATABUFFER_HPP
#define __SEVA_DATABUFFER_HPP

#include <memory>

#include "buffer.hpp"

/**
 * @file databuffer.hpp
 * @brief Databuffer class header file
 */

namespace seva {
namespace graph {
/**
 * @brief represents a DataBuffer object.
 *
 * DataBuffer object can represent general data structure and
 * can be used as input/output buffer of Node.
 * For the user defined data structure, set the size member, and copy
 * it's data to the DataBuffer using memcpy.
 */
class DataBuffer : public Buffer {
  public:
    /**
     * @brief Constructor.
     * @param [in] size size of buffer in bytes.
     */
    DataBuffer(uint32_t size);

    /**
     * @brief Constructor.
     * @param [in] fd file descriptor for ion buffer.
     * @param [in] addr virtual address for ion buffer.
     * @param [in] size size of buffer in bytes.
     */
    DataBuffer(int fd, void *addr, uint32_t size);

    /**
     * @brief Constructor.
     * @param [in] fd file descriptor for ion buffer.
     * @param [in] addr virtual address for ion buffer.
     * @param [in] offset offset from start virtual address for ion buffer.
     * @param [in] size size of buffer in bytes.
     */
    DataBuffer(int fd, void *addr, uint32_t offset, uint32_t size);

    /**
     * @brief Virtual object constructor.
     */
    DataBuffer();
    /**
     * @brief Destructor.
     */
    ~DataBuffer();
    /**
     * @brief Get buffer size.
     * @return buffer size in bytes.
     */
    uint32_t GetSize() { return mRealSize; }

    /**
     * @brief Copy from user provided buffer.
     * @param [in] ptr user buffer pointer.
     * @param [in] size how many bytes to copy.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool ReadFromBuffer(void *ptr, uint32_t size);

    /**
     * @brief Copy from user provided buffer.
     * @param [in] ptr user buffer pointer.
     * @param [in] offset DataBuffer offset value.
     * @param [in] size how many bytes to copy.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool ReadFromBuffer(void *ptr, uint32_t offset, uint32_t size);

    /**
     * @brief Copy to user provided buffer.
     * @param [out] ptr user buffer pointer.
     * @param [in] size how many bytes to copy.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool WriteToBuffer(void *ptr, uint32_t size);

    /**
     * @brief Copy from given file.
     * @param [in] fileName filename.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool ReadFromFile(const char *fileName);

    bool ReadFromFile(const char *fileName, uint32_t size);

    /**
     * @brief Write to given file.
     * @param [in] fileName filename.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool WriteToFile(const char *fileName);

    /**
     * @brief Set buffer's size.
     * @param [in] size size of buffer.
     */
    bool SetMetaData(uint32_t size, bool allocate_memory);

    /**
     * @brief Create DataBuffer instance with std::shared_ptr
     * @param [in] size size of item in bytes.
     * @return std::shared_ptr<DataBuffer>.
     */
    static std::shared_ptr<DataBuffer> MakeDataBuffer(uint32_t size);

    /**
     * @brief Create DataBuffer instance with std::shared_ptr.
     * @param [in] fd file descriptor for ion buffer.
     * @param [in] addr virtual address for ion buffer.
     * @param [in] size size of buffer in bytes.
     */
    static std::shared_ptr<DataBuffer> MakeDataBuffer(int fd, void *addr, uint32_t size);

    /**
     * @brief Create DataBuffer instance with std::shared_ptr.
     * @param [in] fd file descriptor for ion buffer.
     * @param [in] addr virtual address for ion buffer.
     * @param [in] offset offset from start virtual address for ion buffer.
     * @param [in] size size of buffer in bytes.
     */
    static std::shared_ptr<DataBuffer> MakeDataBuffer(int fd, void *addr, uint32_t offset, uint32_t size);

    /**
     * @brief Create Virtual DataBuffer instance with std::shared_ptr
     * @return std::shared_ptr<DataBuffer> that is virtual.
     */
    static std::shared_ptr<DataBuffer> MakeDataBuffer();

  private:
    /**
     * @cond
     * internal
     */
    friend class RingBuffer;
    /**
     * @endcond
     */
    DataBuffer(const DataBuffer &) = delete;
    DataBuffer &operator=(const DataBuffer &) = delete;
    void Initialize(uint32_t size);
    static std::shared_ptr<DataBuffer> CloneMeta(DataBuffer &source);

  private:
    uint32_t mSize;
    uint32_t mRealSize;
    static const char *TAG;
};
}  // namespace graph
}  // namespace seva
#endif
