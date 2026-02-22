#ifndef __SEVA_CONNECTION_HPP
#define __SEVA_CONNECTION_HPP

#include "types.hpp"

namespace seva {
namespace graph {
class Node;
struct PortInfo {
    NodeHandle mNode;
    std::size_t mPortId;

    NodeHandle GetNode() const { return mNode; }
    std::size_t GetPortId() const { return mPortId; }
};

class Connection {
  public:
    const PortInfo &GetDestination() const { return mDestination; }
    const PortInfo &GetSource() const { return mSource; }

    unsigned int GetId() { return mId; }

  private:
    friend class Graph;
    friend class Node;

    Connection(const PortInfo &source, const PortInfo &destination);
    Connection(const Connection &) = delete;
    Connection &operator=(const Connection &) = delete;
    ~Connection();

    void ResetSourceNode();
    void ResetDestinationNode();

  private:
    unsigned int mId;
    static unsigned int sCurrentId;

    PortInfo mSource;
    PortInfo mDestination;

    static const char *TAG;
};
}  // namespace graph
}  // namespace seva
#endif
