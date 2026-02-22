#ifndef __LOG_HPP
#define __LOG_HPP

#include <mutex>
#include <cstdarg>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <cstring>
#ifdef ANDROID
#include <android/log.h>
#include <cutils/properties.h>
#define OFI_GC_ANDROID_LOG_PROPERTY_NAME "vendor.ofi.gc.loglevel"
#endif


/**
 * @file log.hpp
 * @brief Log class header file
 */

/**
 * @brief represents a Log object.
 *
 * Used for Logging functions.
 * Call V(), D(), I(), W(), E() functions depending on the log level.
 */
namespace OfiLog {
/**
 * @brief represents log levels.
 *
 */

enum Level {
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

static Level sLevel = Level::INFO;

static Level GetLevel() {
#ifdef ANDROID
    char loglevelstring[PROPERTY_VALUE_MAX];
    int ret = property_get(OFI_GC_ANDROID_LOG_PROPERTY_NAME, loglevelstring, "ERR");
    if (ret < 0) {
        property_set(OFI_GC_ANDROID_LOG_PROPERTY_NAME, "INFO");
        sLevel = Level::INFO;
    } else {
        if (!std::strcmp(loglevelstring, "VERBOSE"))
            sLevel = Level::VERBOSE;
        else if (!std::strcmp(loglevelstring, "DEBUG"))
            sLevel = Level::DEBUG;
        else if (!std::strcmp(loglevelstring, "INFO"))
            sLevel = Level::INFO;
        else if (!std::strcmp(loglevelstring, "WARN"))
            sLevel = Level::WARN;
        else if (!std::strcmp(loglevelstring, "ERR"))
            sLevel = Level::ERR;
        else if (!std::strcmp(loglevelstring, "DISABLE"))
            sLevel = Level::DISABLE;
        else
            sLevel = Level::INFO;
    }
#else
    char *logvelstring =  getenv("OFI_GC_LOG_LEVEL");
    if (logvelstring != nullptr) {
        if (!strcmp(logvelstring, "DISABLE")) {
                sLevel = Level::DISABLE;
        } else if (!strcmp(logvelstring, "ERR")) {
                sLevel = Level::ERR;
        } else if (!strcmp(logvelstring, "WARN")) {
                sLevel = Level::WARN;
        } else if (!strcmp(logvelstring, "INFO")) {
                sLevel = Level::INFO;
        } else if (!strcmp(logvelstring, "DEBUG")) {
                sLevel = Level::DEBUG;
        } else if (!strcmp(logvelstring, "VERBOSE")) {
                sLevel = Level::VERBOSE;
        } else {
                sLevel = Level::DEBUG;
        }
    } else {
            sLevel = Level::DEBUG;
    }
#endif
    return sLevel;
}

#ifndef ANDROID
static std::string GetTimeStamp() {
    // get a precise timestamp as a string
    const auto now = std::chrono::system_clock::now();
    const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer),"%Y-%m-%d %H:%M:%S", timeinfo);

    std::stringstream ss;
    ss << buffer << '.' << std::setfill('0') << std::setw(3) << nowMs.count();
    return ss.str();
}

static std::string _GetString(Level level) {
    switch (level) {
        case Level::VERBOSE:
            return "VERBOSE";
        case Level::DEBUG:
            return "DEBUG";
        case Level::INFO:
            return "INFO";
        case Level::WARN:
            return "WARN";
        case Level::ERR:
            return "ERROR";
        default:
            return "DISABLE";
    }
}
#endif

#ifdef __ANDROID__
static void PrintlnFormat(Level level, std::string& tag, const char* format, va_list params) {

    if ((int)level > (int)GetLevel()) {
        return;
    }

    char buf[1024] = {0};
    vsnprintf(buf, sizeof(buf), format, params);

    switch (level) {
        case Level::VERBOSE:
            __android_log_print(ANDROID_LOG_VERBOSE, tag.c_str(), "%s", buf);
            break;
        case Level::DEBUG:
            __android_log_print(ANDROID_LOG_DEBUG, tag.c_str(), "%s", buf);
            break;
        case Level::INFO:
            __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", buf);
            break;
        case Level::WARN:
            __android_log_print(ANDROID_LOG_WARN, tag.c_str(), "%s", buf);
            break;
        case Level::ERR:
            __android_log_print(ANDROID_LOG_ERROR, tag.c_str(), "%s", buf);
            break;
        default:
            __android_log_print(ANDROID_LOG_DEFAULT, tag.c_str(), "%s", buf);
            break;
    }
}
#else
static void PrintlnFormat(Level level, std::string& tag, const char* format, va_list params) {
    if ((int)level > (int)GetLevel()) {
        return;
    }
    char buf[1024] = {0};
    vsnprintf(buf, sizeof(buf), format, params);
    std::cout << GetTimeStamp() << "  [" << std::setw(7) << _GetString(level) << "]  " << tag << "\t" << buf << std::endl;
}
#endif

static inline void log(Level severity, const char* tag, const char* format, ...) {
    std::string t(tag);
    va_list params;

    va_start(params, format);
    PrintlnFormat(severity, t, format, params);
    va_end(params);
}

/*
    static void V(const char* tag, const char* message) {
        std::string t(tag);
        std::string msg(message);
        Println(Level::VERBOSE, t, msg);
    }
    static void V(std::string tag, std::string message) {
        Println(Level::VERBOSE, tag, message);
    }
    static void V(std::string tag, const char* format, ...) {
        va_list params;

        va_start(params, format);
        PrintlnFormat(Level::VERBOSE, tag, format, params);
        va_end(params);
    }

    static void D(const char* tag, const char* message) {
        std::string t(tag);
        std::string msg(message);
        Println(Level::DEBUG, t, msg);
    }
    static void D(std::string tag, std::string message) {
        Println(Level::DEBUG, tag, message);
    }
    static void D(std::string tag, const char* format, ...) {
        va_list params;

        va_start(params, format);
        PrintlnFormat(Level::DEBUG, tag, format, params);
        va_end(params);
    }

    static void I(const char* tag, const char* message) {
        std::string t(tag);
        std::string msg(message);
        Println(Level::INFO, t, msg);
    }
    static void I(std::string tag, std::string message) {
        Println(Level::INFO, tag, message);
    }
    static void I(std::string tag, const char* format, ...) {
        va_list params;

        va_start(params, format);
        PrintlnFormat(Level::INFO, tag, format, params);
        va_end(params);
    }

    static void W(const char* tag, const char* message) {
        std::string t(tag);
        std::string msg(message);
        Println(Level::WARN, t, msg);
    }
    static void W(std::string tag, std::string message) {
        Println(Level::WARN, tag, message);
    }
    static void W(std::string tag, const char* format, ...) {
        va_list params;

        va_start(params, format);
        PrintlnFormat(Level::WARN, tag, format, params);
        va_end(params);
    }

    static void E(const char* tag, const char* message) {
        std::string t(tag);
        std::string msg(message);
        Println(Level::ERR, t, msg);
    }
    static void E(std::string tag, std::string message) {
        Println(Level::ERR, tag, message);
    }
    static void E(std::string tag, const char* format, ...) {
        va_list params;

        va_start(params, format);
        PrintlnFormat(Level::ERR, tag, format, params);
        va_end(params);
    }
*/
};

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG(severity, fmt, ...) { \
    char logformat[1024] = "%s"; \
    strcat(logformat, fmt); \
    log(severity, "OFI_GC", logformat," ", ## __VA_ARGS__); \
}
/*
#define LOGV(fmt,...) { \
    char format[1024] = "[%s:%d] "; \
    strcat(format, fmt); \
    log(Level::VERBOSE, __FILENAME__, format, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
}

#define LOGD(fmt,...) { \
    char format[1024] = "[%s:%d] "; \
    strcat(format, fmt); \
    log(Level::DEBUG, __FILENAME__, format, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
}

#define LOGI(fmt,...) { \
    char format[1024] = "[%s:%d] "; \
    strcat(format, fmt); \
    log(Level::INFO, __FILENAME__, format, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
}

#define LOGW(fmt,...) { \
    char format[1024] = "[%s:%d] "; \
    strcat(format, fmt); \
    log(Level::WARN, __FILENAME__, format, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
}

#define LOGE(fmt,...) { \
    char format[1024] = "[%s:%d] "; \
    strcat(format, fmt); \
    log(Level::ERROR, __FILENAME__, format, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
}
*/
#endif
