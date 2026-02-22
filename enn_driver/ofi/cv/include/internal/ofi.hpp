#ifndef __SEVA_OFI_HPP
#define __SEVA_OFI_HPP

#include "connection.hpp"
#include "node.hpp"
#include "types.hpp"

#include <mutex>
#include <stack>
#include <string>

namespace seva {
namespace graph {
typedef struct BufferIR_t {
    int type;  // image, array, pyramid, databuffer
    unsigned int ndims;
    uint16_t portno;
    float scale;  // for pyramid
    int level;    // for pyramid
    std::vector<std::vector<unsigned int>> dims;
    std::vector<std::vector<unsigned int>> strides;
} BufferIR;

typedef struct UserParamIR_t {
    unsigned int size;
} UserParamIR;

enum KernelType {
    DSP,
    CPU,
};

typedef struct UserKernelIR_t {
    int kernelid;
    KernelType type;
    std::string binpath;
} UserKernelIR;

typedef struct ValidationIR_t {
    KernelType type;
    bool valid;
} ValidationIR;

typedef struct NodeIR_t {
    int kernelid;
    int nodeid;
    int affinity;
    std::vector<BufferIR> inbufs;
    std::vector<BufferIR> outbufs;
    std::vector<UserParamIR> params;
    std::vector<ValidationIR> valids;
} NodeIR;

typedef struct PortIR_t {
    int nodeid;
    int portno;
} PortIR;

typedef struct ConnectionIR_t {
    PortIR input;
    PortIR output;
} ConnectionIR;

typedef struct GCIR_t {
    /* Graph Header info */
    int graphid;
    int pid;

    std::vector<NodeIR> nodes;
    std::vector<ConnectionIR> connections;
    std::vector<UserKernelIR> userkernels;
} GCIR;
}  // namespace graph
}  // namespace seva
#endif
