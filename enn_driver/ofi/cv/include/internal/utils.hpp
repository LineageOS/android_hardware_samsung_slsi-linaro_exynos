#ifndef __SEVA_UTILS_HPP
#define __SEVA_UTILS_HPP

#include <string>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * @file utils.hpp
 * @brief Utils class header file
 */

namespace seva {
namespace utils {
/**
 * @brief represents a Utils object.
 *
 * Used for utility functions like CreateDirectory
 */
class Utils {
  public:
    /**
     * @brief CreateDirectory
     * @param [in] path directory path which is included file name
     */
    static bool CreateDirectory(const char *path);
    /**
     * @brief GetSoC
     */
    static std::string GetSoC();

  private:
    static const char *TAG;
};
}  // namespace utils
}  // namespace seva
#endif
