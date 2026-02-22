#ifndef __NN_DATA_TYPE_H__
#define __NN_DATA_TYPE_H__

#include <cstdint>

#define ST_DT_SINT08 "sint8"
#define ST_DT_SINT16 "sint16"
#define ST_DT_UINT08 "uint8"

// In makaru h/w, S08 is #0, S16 is #16, U08 is #2.
// The order of dtype for makaru h/w is differnt from following NNDataType
enum NNDataType
{
  DT_SINT08, // Signed 8-bit integer
  DT_SINT16, // Signed 16-bit integer
  DT_UINT08, // Unsigned 8-bit integer
};

uint32_t sizeOf(const NNDataType &);

#endif
