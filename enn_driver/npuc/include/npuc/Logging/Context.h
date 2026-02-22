#ifndef __LOGGING_CONTEXT_H__
#define __LOGGING_CONTEXT_H__

#include <cstdint>
#include <string>

namespace Logging
{

class Context
{
public:
  uint32_t mask(void) const;
  uint32_t mask(const std::string &tag) const;

public:
  static const Context &access(void);
};

} // namespace Logging

#endif // __LOGGING_CONTEXT_H__
