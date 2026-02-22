#ifndef __SEVA_LOG_HPP
#define __SEVA_LOG_HPP

#include <cstdarg>

/**
 * @file log.hpp
 * @brief Log class header file
 */

namespace seva {
namespace system {
/**
 * @brief represents a Log object.
 *
 * Used for Logging functions.
 * Call V(), D(), I(), W(), E() functions depending on the log level.
 */
class Log {
  public:
    /**
     * @brief represents log levels.
     *
     */
    enum class Level {
        /*! \brief Disable logging. */
        DISABLE = 0,
        /*! \brief Error log level. */
        ERR = 1,
        /*! \brief Warning log level. */
        WARN = 2,
        /*! \brief Information log level. */
        INFO = 3,
        /*! \brief Debug log level. */
        DEBUG = 4,
        /*! \brief Verbose log level. */
        VERBOSE = 5,
    };

  public:
    /// @name Verbose log level functions.
    /// @{
    static void V(const char *tag, const char *format, ...);
    ///@}

    /// @name Debug log level functions.
    /// @{
    static void D(const char *tag, const char *format, ...);
    ///@}

    /// @name Information log level functions.
    /// @{
    static void I(const char *tag, const char *format, ...);

    /// @name Warning log level functions.
    /// @{
    static void W(const char *tag, const char *format, ...);

    /// @name Error log level functions.
    /// @{
    static void E(const char *tag, const char *format, ...);
    ///@}

    /**
     * @brief Set log level.
     */
    static void SetLevel(Log::Level level);

  private:
    static Log::Level sLevel;
    static Log::Level GetLevel();

  private:
    Log();
    Log(const Log &that);
    Log &operator=(const Log &that);
    static void PrintlnFormat(Log::Level level, const char *tag, const char *format, va_list params);
};
}  // namespace system
}  // namespace seva
#endif
