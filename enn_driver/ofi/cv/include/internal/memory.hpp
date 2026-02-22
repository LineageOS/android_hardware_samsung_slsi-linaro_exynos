#ifndef __SEVA_MEMORY_HPP
#define __SEVA_MEMORY_HPP

#include <cstdint>

namespace seva {
namespace system {
struct MemInfo {
    int64_t mId;            // Memory object Id.
    int32_t mSize;          // Request memory size.
    int32_t mOffset;        // how far from the base address.
    void *mVa;              // Virtual address for cpu. mVa may be not used in
    uint32_t mMagicNumber;  // To check integrity.
};

typedef MemInfo *MemoryInfo;

class Memory {
  public:
    enum class Type {
        DEPEND_PLATFORM = 0,
        MALLOC,
        ION_INTERNAL,
        ION_INTERNAL_CACHE_FORCE,
        MAX,
    };
};
}  // namespace system
}  // namespace seva
#endif
