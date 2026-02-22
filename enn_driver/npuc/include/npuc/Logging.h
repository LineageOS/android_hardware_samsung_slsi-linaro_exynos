#ifndef __LOGGING_H__
#define __LOGGING_H__

#include "Logging/Context.h"

#include <cstdint>
#include <iostream>
#include <ostream>

// Colors for console
#define _RED "\033[0;31m"
#define _YELLOW "\033[1;33m"
#define _NC "\033[0m"
#define _GREEN "\033[0;32m"
#define _LBLUE "\033[1;34m"

#ifndef OFFLINE_COMPILER
#include <sstream>
#ifndef X86_ONDEVICE
#include <android/log.h>
#endif // X86_ONDEVICE

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "NPUC"
#define NPUC_TAG "[Exynos][EdenDriver][NPUC_ONDEVICE]"
#endif // END of OFFLINE_COMPILER

namespace Logging
{

enum Level
{
  VERBOSE = 0,
  DBG = 1, // Note: DEBUG is conflicting with DEBUG macro which is defined for debug build
  INFO = 2,
  WARN = 3,
  ERROR = 4,
  FATAL = 5,
  LOG_LEVEL_MAX = 6,
};

class GlobalPoint
{
public:
  GlobalPoint();
  GlobalPoint(const Context &);
  ~GlobalPoint();

public:
  bool enabled(const Level &level) const;

public:
  std::ostream &stream(const Level &level);

public:
  static GlobalPoint &access(void);

private:
  uint32_t _mask;
  Level _level;
#ifndef OFFLINE_COMPILER
  std::ostringstream _buffer;
#endif // END of OFFLINE_COMPILER
};
}

#if defined(OFFLINE_COMPILER) || defined(X86_ONDEVICE)
#define NPUC_LOG(Level)                                       \
  if (Logging::GlobalPoint::access().enabled(Logging::Level)) \
  Logging::GlobalPoint::access().stream(Logging::Level)
#else // ondevice compiler
#define NPUC_LOG(Level) Logging::GlobalPoint().stream(Logging::Level)
#endif // END of OFFLINE_COMPILER

#include <string>

namespace Logging
{

class NamedPoint
{
public:
  NamedPoint() : _mask(0), _level(DBG){};
  NamedPoint(const Context &, const std::string &tag);
  ~NamedPoint();

public:
  bool enabled(const Level &level) const;

public:
  std::ostream &stream(const Level &level);

private:
  uint32_t _mask;
  Level _level;
#ifndef OFFLINE_COMPILER
  std::ostringstream _buffer;
#endif
};

#if defined(OFFLINE_COMPILER) || defined(X86_ONDEVICE)
#define KNOB(name) extern NamedPoint name;
#include "Logging/Knob.lst"
#undef KNOB
#endif // END of OFFLINE_COMPILER

} // namespace Logging

#if defined(OFFLINE_COMPILER) || defined(X86_ONDEVICE)
#define LOG_AT(Name, Level)                  \
  if (Logging::Name.enabled(Logging::Level)) \
  Logging::Name.stream(Logging::Level)
#define LOGP_AT(Name, Level, Fmt, ...)       \
  if (Logging::Name.enabled(Logging::Level)) \
  printf("[%s:%d] " Fmt, __func__, __LINE__, ##__VA_ARGS__)
#else // ondevice compiler
#define LOG_AT(Name, Level) Logging::NamedPoint().stream(Logging::Level)
#define LOGP_AT(Name, Level, Fmt, ...)                                                      \
  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s[%s:%d][%s] " Fmt, NPUC_TAG, __func__, \
                      __LINE__, ONDEVICE_VERSION, ##__VA_ARGS__)
#endif // END of OFFLINE_COMPILER

#endif //__LOGGING_H__
