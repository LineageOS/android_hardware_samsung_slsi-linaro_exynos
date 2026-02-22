#ifndef __SEVA_DATA_HPP
#define __SEVA_DATA_HPP

#include <cstdint>

namespace seva {
namespace graph {
class Data {
  public:
    enum class Type {
        NONE = 0,
        U8 = 1,
        U16,
        U32,
        S8,
        S16,
        S32,
        F8,
        F16,
        F32,
        MAX,
    };

    static unsigned int GetSize(Type type) {
        unsigned int size = 0;
        switch (type) {
        case Type::U8:
        case Type::S8:
        case Type::F8: size = 1; break;
        case Type::U16:
        case Type::S16:
        case Type::F16: size = 2; break;
        case Type::U32:
        case Type::S32:
        case Type::F32: size = 4; break;
        default: size = 0; break;
        }

        return size;
    }
};
}  // namespace graph
}  // namespace seva
#endif
