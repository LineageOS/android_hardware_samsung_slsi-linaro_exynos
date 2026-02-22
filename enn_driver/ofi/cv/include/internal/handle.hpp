#ifndef __SEVA_HANDLE_HPP
#define __SEVA_HANDLE_HPP

#include <cassert>
#include <functional>  // std::hash
#include <memory>

#include "log.hpp"

namespace seva {
namespace graph {
// Non owning graph elements pointer
// Elements owned by graph
template <typename T> class Handle final {
    friend class Graph;
    friend class Node;

    std::weak_ptr<T> m_ptr;

    Handle(const std::shared_ptr<T> &obj) : m_ptr(obj) {
        if (obj == nullptr) {
            system::Log::E("Handle", "%s(): Handle is nullptr", __FUNCTION__);
            assert(0);
        }
    }

    static T *Check(T *val) {
        if (val == nullptr) {
            system::Log::E("Handle", "%s(): Handle is nullptr", __FUNCTION__);
            assert(0);
        }
        return val;
    }

  public:
    Handle() = default;
    Handle(std::nullptr_t) {}
    Handle(const Handle &) = default;
    Handle &operator=(const Handle &) = default;
    Handle &operator=(std::nullptr_t) {
        m_ptr.reset();
        return *this;
    }
    Handle(Handle &&) = default;
    Handle &operator=(Handle &&) = default;

    bool expired() const { return m_ptr.expired(); }

    // Graphs not intended for multithreaded modification so this should be safe
    T *get() const { return m_ptr.lock().get(); }

    T &operator*() const { return *Check(get()); }

    T *operator->() const { return Check(get()); }

    bool operator==(const Handle &other) const { return get() == other.get(); }

    bool operator!=(const Handle &other) const { return get() != other.get(); }

    friend bool operator==(std::nullptr_t, const Handle &other) { return nullptr == other.get(); }

    friend bool operator==(const Handle &other, std::nullptr_t) { return nullptr == other.get(); }

    friend bool operator!=(std::nullptr_t, const Handle &other) { return nullptr != other.get(); }

    friend bool operator!=(const Handle &other, std::nullptr_t) { return nullptr != other.get(); }
};

template <typename T> inline std::ostream &operator<<(std::ostream &os, const Handle<T> &arg) {
    os << arg.get();
    return os;
}
}  // namespace graph
}  // namespace seva
#endif
