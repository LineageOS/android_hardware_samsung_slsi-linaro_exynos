
#pragma once

#include <cstdlib>
#include "ofi_gc_api.hpp"

class GRAPH_COMPILER_API_CLASS(GraphGenDebuger) {
  public:
    bool debug(char* graphIr, char* sharedMemInfo = nullptr, uint32_t sharedMemInfoSize = 0);
};

