#ifndef __SEVA_ARRAY_HPP
#define __SEVA_ARRAY_HPP

#include <memory>

#include "buffer.hpp"
#include "data.hpp"

/**
 * @file array.hpp
 * @brief Array class header file
 */

namespace seva {
namespace graph {
/**
 * @brief represents an array object.
 *
 * Can be used as input/output buffer of Node.
 */
class Array : public Buffer {
  public:
    /**
     * @brief Constructor.
     * @param [in] itemType type of item.
     * @param [in] numItems number of items.
     */
    Array(Data::Type itemType, uint32_t numItems);
    /**
     * @brief Constructor.
     * @param [in] fd file descriptor for ion buffer.
     * @param [in] addr virtual address for ion buffer.
     * @param [in] itemType type of item.
     * @param [in] numItems number of items.
     */
    Array(int fd, void *addr, Data::Type itemType, uint32_t numItems);
    /**
     * @brief Constructor.
     * @param [in] fd file descriptor for ion buffer.
     * @param [in] addr virtual address for ion buffer.
     * @param [in] offset offset from start virtual address for ion buffer.
     * @param [in] size size of ion buffer.
     * @param [in] itemType type of item.
     * @param [in] numItems number of items.
     */
    Array(int fd, void *addr, uint32_t offset, uint32_t size, Data::Type itemType, uint32_t numItems);
    /**
     * @brief Virtual object constructor.
     */
    Array();
    /**
     * @brief Destructor.
     */
    ~Array();
    /**
     * @brief Get item type.
     * @return item type.
     */
    Data::Type GetItemType() const;
    /**
     * @brief Get item size.
     * @return item size.
     */
    uint32_t GetItemSize() const;
    /**
     * @brief Get the number of items.
     * @return number of items.
     */
    uint32_t GetNums() const;
    /**
     * @brief Copy from given user buffer.
     * @param [in] ptr user buffer pointer.
     * @param [in] itemType type of item.
     * @param [in] numItems number of items to copy.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool ReadFromBuffer(void *ptr, Data::Type itemType, uint32_t numItems);

    /**
     * @brief Copy from given file.
     * @param [in] fileName filename.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool ReadFromFile(const char *fileName);

    bool ReadFromFile(const char *fileName, Data::Type itemType, uint32_t numItems);

    /**
     * @brief Copy to given user buffer.
     * @param [out] ptr user buffer pointer.
     * @param [in] itemType type of item.
     * @param [in] numItems number of items to copy.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool WriteToBuffer(void *ptr, Data::Type itemType, uint32_t numItems);

    /**
     * @brief Write to given file.
     * @param [in] fileName filename.
     * @return If succeeds, it returns true. If not, it return false.
     */
    bool WriteToFile(const char *fileName);

    /**
     * @brief Set array's size and number of elements
     * @param [in] type type of item.
     * @param [in] nums number of item.
     * @return true on success, false on failure.
     */
    bool SetMetaData(Data::Type type, uint32_t nums);

    bool SetMetaData(uint32_t nums);

    /**
     * @brief Create Array instance with std::shared_ptr
     * @param [in] type type of item.
     * @param [in] nums number of item.
     */
    static std::shared_ptr<Array> MakeArray(Data::Type type, uint32_t nums);

    /**
     * @brief Create Array instance with std::shared_ptr.
     * @param [in] fd file descriptor for ion buffer.
     * @param [in] addr virtual address for ion buffer.
     * @param [in] itemType type of item.
     * @param [in] numItems number of items.
     */
    static std::shared_ptr<Array> MakeArray(int fd, void *addr, Data::Type itemType, uint32_t numItems);

    /**
     * @brief Create Array instance with std::shared_ptr.
     * @param [in] fd file descriptor for ion buffer.
     * @param [in] addr virtual address for ion buffer.
     * @param [in] offset offset from start virtual address for ion buffer.
     * @param [in] size size of ion buffer.
     * @param [in] itemType type of item.
     * @param [in] numItems number of items.
     */
    static std::shared_ptr<Array>
    MakeArray(int fd, void *addr, uint32_t offset, uint32_t size, Data::Type itemType, uint32_t numItems);

    /**
     * @brief Create Virtual Array instance with std::shared_ptr
     * @return std::shared_ptr<Array> that is virtual.
     */
    static std::shared_ptr<Array> MakeArray();

    /**
     * @cond
     * internal
     */
    void AllocateMemory(void);
    Data::Type GetItemType() { return mItemType; }
    /**
     * @endcond
     */

  private:
    /**
     * @cond
     * internal
     */
    friend class RingBuffer;
    /**
     * @endcond
     */
    Array(const Array &) = delete;
    Array &operator=(const Array &) = delete;
    void Initialize(Data::Type itemType, uint32_t numItems);

  private:
    static std::shared_ptr<Array> CloneMeta(Array &source);
    Data::Type mItemType;
    uint32_t mNumItems;
    uint32_t mRealNumItems;
    static const char *TAG;
};
}  // namespace graph
}  // namespace seva
#endif
