#ifndef __SEVA_RINGBBUFFER_HPP
#define __SEVA_RINGBBUFFER_HPP

#include <mutex>

#include "buffer.hpp"
#include "types.hpp"

namespace seva {
namespace graph {
class Node;
class RingBuffer final {
    class NodeBufferInfo {
      public:
        NodeBufferInfo(NodeHandle node, bool input, uint16_t port);
        void UpdateNodeBuffer(BufferPtr buf);
        NodeHandle GetNodeHandle();
        uint16_t GetPort();
        bool IsInput();

      private:
        NodeHandle mNode;
        bool mInput;
        uint16_t mPort;
    };

    using NodeInfoList = std::vector<NodeBufferInfo>;
    using BufferEntry = std::pair<BufferPtr, NodeInfoList>;

  public:
    RingBuffer(Buffer &reference, size_t count);
    void age();
    const BufferPtr &GetBufferAt(size_t index);

  private:
    friend class Node;
    void RegisterNodeInfo(NodeHandle handle, bool input, std::size_t port, const BufferPtr &buf);
    void UnregisterNodeInfo(NodeHandle handle, bool input, std::size_t port, const BufferPtr &buf);
    void dump();

  private:
    Buffer::Type mType;
    size_t mSize;
    std::vector<BufferEntry> mBufferEntries;
    std::mutex mLock;
    static const char *TAG;
};
}  // namespace graph
}  // namespace seva
#endif
