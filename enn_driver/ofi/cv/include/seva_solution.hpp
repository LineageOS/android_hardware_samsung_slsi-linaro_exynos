/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SEVA_SOLUTION_HPP
#define SEVA_SOLUTION_HPP

#include <stdint.h>
#include <stddef.h>
#include <memory>

/**
 * @file seva_solution.hpp
 * @brief Seva Solution common class/data structure definition.
 */

namespace seva {
/// Main namespace for Solution APIs.
namespace sol {

/**
 * @brief Result of API call. RESULT be returned most of Seva Solution API.
 */
enum RESULT {
    R_PASS = 0, /*!< API calling was successful.*/
    R_FAIL = 1, /*!< API calling was not successful.*/
};

/**
 * @brief Class holds buffer information.
 */
class Util;

class _buf {
  public:
    /**
     * @brief Constructor.
     *
     * Initialize _buf class.
     */
    _buf();

    /**
     * @brief Construct and allocate memory.
     *
     * Initialize _buf class and allocate memory.
     */

    _buf(size_t size);

    /**
     * @brief Constructor with external fd.
     *
     * Initialize _buf class with external fd.
     * When shared fd was already mmap'd, pass vaddr.
     */
    _buf(size_t size, int fd, void *vaddr = nullptr);

    /**
     * @brief Constructor with external fd.
     *
     * Initialize _buf class with a piece of external fd.
     * When shared fd was already mmap'd, pass vaddr.
     */
    _buf(size_t size, int fd, size_t offset, void *vaddr = nullptr);

    /**
     * @brief Deonstructor.
     *
     * Deinitialize _buf class.\n
     * Allocated memory will be free.
     */
    ~_buf();

    /**
     * @brief Get FD of ION memory.
     *
     * returns File Descriptor when memory allocated on ION.
     */
    int32_t GetFd();

    /**
     * @brief Get offset from the base of continuously allocated memory space.
     *
     * Two or more buffer memory can be allocated on same ION FD to save the memory space.\n
     * GetOffset() returns offset from the base of allocated ION FD.\n
     * Users are allowed to use memory jump from the base of FD as offset.
     */
    size_t GetOffset();

    /**
     * @brief Get allocated memory size.
     *
     * returns size of allocated buffer memory.
     */
    size_t GetSize();

    /**
     * @brief Get pointer to allocated buffer memory
     *
     * returns start pointer of allocated buffer memory.
     */
    uint8_t *GetBufferPtr();

    /**
     * @brief Get width value if it is 2D array buffer.
     *
     * returns width value of buffer memory.
     */
    size_t GetWidth();

    /**
     * @brief Get height value if it is 2D array buffer.
     *
     * returns height value of buffer memory.
     */
    size_t GetHeight();

  private:
    friend Util;

  protected:
    struct _seva_v;
    std::unique_ptr<_seva_v> pv;

    _buf(_buf &&);
    _buf &operator=(_buf &&);
    _buf(_buf &) = delete;  // no copy allowed.
    _buf operator=(_buf &) = delete;
};

typedef std::shared_ptr<_buf> Buffer;

/**
 * @brief Create Buffer with external fd.
 *
 * Initialize _buf class with external fd.
 * When shared fd was already mmap'd, pass vaddr.
 */
Buffer CreateBufferFromFd(size_t size, int fd, void *vaddr = nullptr);

/**
 * @brief Create Buffer with a piece of external fd.
 *
 * Initialize _buf class with a piece of external fd.
 *
 * When the shared fd was already mmap'd, pass the pointer of data start.
 */
Buffer CreateBufferFromFdOffset(size_t bufsize, int fd, size_t offset, void *bufaddr = nullptr);

}  // namespace sol
}  // namespace seva

#endif  // SEVA_SOLUTION_HPP
