#ifndef __SEVA_TYPES_HPP
#define __SEVA_TYPES_HPP

#include <map>
#include <vector>

#include "handle.hpp"

namespace seva {
namespace system {
class Kernel;
using KernelPtr = std::unique_ptr<Kernel>;
}  // namespace system

namespace graph {
class Node;
class Graph;
class Buffer;
class UserParam;
class Connection;

using NodePtr = std::shared_ptr<Node>;
using NodeHandle = Handle<Node>;
using NodeList = std::vector<NodePtr>;

using BufferPtr = std::shared_ptr<Buffer>;
using BufferList = std::map<std::size_t, BufferPtr>;
using UserParamPtr = std::shared_ptr<UserParam>;

using ConnectionPtr = std::shared_ptr<Connection>;
using ConnectionHandle = Handle<Connection>;
using ConnectionList = std::vector<ConnectionPtr>;

using GraphPtr = std::shared_ptr<Graph>;

#define PARAMETER_REQUIRED 0
#define PARAMETER_OPTIONAL 1
#define TILETYPE_BTI 0
#define TILETYPE_CTILIST 1
}  // namespace graph
}  // namespace seva

#define GET_INPUT_BUFFER_IMAGE(I) std::static_pointer_cast<seva::graph::Image>(inputs.at(I))
#define GET_INPUT_BUFFER_ARRAY(I) std::static_pointer_cast<seva::graph::Array>(inputs.at(I))
#define GET_INPUT_BUFFER_DATABUFFER(I) std::static_pointer_cast<seva::graph::DataBuffer>(inputs.at(I))
#define GET_INPUT_BUFFER_PYRAMID(I) std::static_pointer_cast<seva::graph::Pyramid>(inputs.at(I))

#define GET_OUTPUT_BUFFER_IMAGE(I) std::static_pointer_cast<seva::graph::Image>(outputs.at(I))
#define GET_OUTPUT_BUFFER_ARRAY(I) std::static_pointer_cast<seva::graph::Array>(outputs.at(I))
#define GET_OUTPUT_BUFFER_DATABUFFER(I) std::static_pointer_cast<seva::graph::DataBuffer>(outputs.at(I))
#define GET_OUTPUT_BUFFER_PYRAMID(I) std::static_pointer_cast<seva::graph::Pyramid>(outputs.at(I))

#endif
