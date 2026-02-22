#ifndef __SEVA_PYRAMIDIMAGE_HPP
#define __SEVA_PYRAMIDIMAGE_HPP

#include "buffer.hpp"
#include "image.hpp"

/**
 * @file pyramid.hpp
 * @brief Pyramid class header file
 */

namespace seva {
namespace graph {
/**
 * @brief represents a pyramid object.
 *
 * Can be used as input/output buffer of Node.
 */
class Pyramid : public Buffer {
  public:
    /**
     * @brief Constructor.
     * @param [in] level number of levels.
     * @param [in] sacle scale factor between each level.
     * @param [in] width width of base image.
     * @param [in] height height of base image.
     * @param [in] format format of base image.
     */
    Pyramid(int level, float scale, uint32_t width, uint32_t height, Image::Format format);

    /**
     * @brief Virtual object Constructor.
     * @param [in] level number of levels.
     * @param [in] scale scale factor between each level.
     *
     */
    Pyramid(int level, float scale);
    /**
     * @brief Destructor.
     */
    ~Pyramid();

    /**
     * @brief Get level.
     * @return Pyramid level.
     */
    int GetLevel() const { return mLevel; }
    /**
     * @brief Get scale.
     * @return Pyramid scale.
     */
    float GetScale() const { return mScale; }
    /**
     * @brief Get base image's width.
     * @return base image's width.
     */
    uint32_t GetWidth() const { return mWidth; }
    /**
     * @brief Get base image's height.
     * @return base image's height.
     */
    uint32_t GetHeight() const { return mHeight; }
    /**
     * @brief Get base image's format.
     * @return base image's format.
     */
    Image::Format GetFormat() const { return mFormat; }

    /**
     * @brief Get given level's image.
     * @param [in] level level want to get image.
     * @return a shared pointer of Image which is given level.
     */
    std::shared_ptr<Image> GetImage(int level);

    /**
     * @brief Get all images in pyramid.
     * @return Image shared pointer vector in pyramid.
     */
    std::vector<std::shared_ptr<Image>> GetImageVector() { return mImages; }

    bool SetInfo(int level, float scale, uint32_t width, uint32_t height, Image::Format format);

    /**
     * @cond
     * internal
     */
    void AllocateMemory(void);
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
    Pyramid() = delete;
    Pyramid(const Pyramid &) = delete;
    Pyramid &operator=(const Pyramid &) = delete;
    void Initialize(int level, float scale, uint32_t width, uint32_t height, Image::Format format);
    static std::shared_ptr<Pyramid> CloneMeta(Pyramid &source);

  private:
    int mLevel;
    float mScale;
    /* base image meta information */
    Image::Format mFormat;
    uint32_t mWidth;
    uint32_t mHeight;
    /* base image meta information */
    std::vector<std::shared_ptr<Image>> mImages;
    static const char *TAG;
};
}  // namespace graph
}  // namespace seva
#endif
