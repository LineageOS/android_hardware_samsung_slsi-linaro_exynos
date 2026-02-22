#ifndef __SEVA_TENSOR_HPP
#define __SEVA_TENSOR_HPP

#include "buffer.hpp"
#include "data.hpp"

/**
 * @file tensor.hpp
 * @brief Tensor class header file
 */

namespace seva {
namespace graph {
/**
 * @brief represents an image object.
 *
 * Can be used as input/output buffer of Node.
 */
class Tensor : public Buffer {
  public:
    Tensor(uint32_t width, uint32_t height, uint32_t channel, Data::Type type);

    Tensor(int fd, void *addr, uint32_t width, uint32_t height, uint32_t channel, Data::Type type);

    Tensor(int fd,
           void *addr,
           uint32_t offset,
           uint32_t size,
           uint32_t width,
           uint32_t height,
           uint32_t channel,
           Data::Type type);

    /**
     * @brief Virtual object constructor.
     */
    Tensor();
    /**
     * @brief Destructor.
     */
    ~Tensor();

    /**
     * @brief Get dimension information.
     * @return Image width in bytes.
     */
    uint32_t GetWidth() const { return mWidth; }
    uint32_t GetHeight() const { return mHeight; }
    uint32_t GetChannel() const { return mChannel; }

    Data::Type GetType() const { return mDataType; }

    /**
     * @brief Read tensor data from file.
     * @param [in] fileName image filename (.pgm or .rgb rawfile).
     * @return on Success, true is returned. On failure false is returned.
     */
    bool ReadFromFile(const char *fileName);

    /**
     * @brief Copy pixel data from given user buffer.
     * @param [in] buffer user provided buffer.
     * @param [in] size number of bytes buffer.
     * @return how many bytes are copied.
     */
    size_t ReadFromBuffer(void *buffer, size_t size);

    /**
     * @brief Make an raw file for Tensor data.
     * @param [in] fileName.
     * @return on Success, true is returned. On failure false is returned.
     */
    bool WriteToFile(const char *fileName);

    /**
     * @brief Copy Tensor data to given user buffer.
     * @param [in] ptr user buffer.
     * @param [in] size buffer size.
     * @return how many bytes are copied.
     */
    size_t WriteToBuffer(void *ptr, size_t size);

    /**
     * @brief Set Tensor metadata.
     * @return on Success, true is returned. On failure false is returned.
     */
    bool SetMetaData(uint32_t width, uint32_t height, uint32_t channel, Data::Type type);

    bool SetMetaData(uint32_t width, uint32_t height, uint32_t channel);

    static std::shared_ptr<Tensor> MakeTensor(uint32_t width, uint32_t height, uint32_t channel, Data::Type type);

    static std::shared_ptr<Tensor>
    MakeTensor(int fd, void *addr, uint32_t width, uint32_t height, uint32_t channel, Data::Type type);

    static std::shared_ptr<Tensor> MakeTensor(int fd,
                                              void *addr,
                                              uint32_t offset,
                                              uint32_t size,
                                              uint32_t width,
                                              uint32_t height,
                                              uint32_t channel,
                                              Data::Type type);

    static std::shared_ptr<Tensor> MakeTensor();

  private:
    Tensor(const Tensor &) = delete;
    Tensor &operator=(const Tensor &) = delete;
    bool Initialize(uint32_t width, uint32_t height, uint32_t channel, Data::Type type);
    void SetBufferMetaData(uint32_t width, uint32_t height, uint32_t channel);
    void AllocateMemory(void);

  private:
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mChannel;
    Data::Type mDataType;
    static const char *TAG;
};
}  // namespace graph
}  // namespace seva
#endif
