#ifndef __SUPPORTED_OPS_H__
#define __SUPPORTED_OPS_H__

#include <vector>
#include "Logging.h"

#include "capability/NPUCtype.h"

namespace NPUC
{

/**
 * @brief A base class of Supported Operation
 */

class SupportedOperation
{
public:
  virtual ~SupportedOperation() = default;

  /**
   * @brief A function to check if it is supported or not
   * @param an operation type in enum OpType
   * @retval bool
   */
  virtual bool isSupportedOp(NPUC::OpType opType) = 0;

  /**
   * @brief set kernel size in order to check if operation is supported according to kerenl size
   * @param kernel size
   */
  virtual void setKernelSize(uint32_t kernelSize) = 0;

  /**
   * @brief set stride size in order to check if operation is supported according to stride size
   * @param stride size
   */
  virtual void setStrideSize(uint32_t strideSize) = 0;

  /**
   * @brief set padding size in order to check if operation is supported according to padding size
   * @param padding size : the biggest one
   */
  virtual void setPaddingSize(uint32_t paddingSize) = 0;

  /**
   * @brief set zero point in order to check if operation is supported according to zero point size
   * @param zero point
   */
  virtual void setZeroPoint(uint32_t zeroPoint) = 0;

  /**
   * @brief set axis in order to check if operation is supported according to axis (for example,
   * concat)
   * @param axis
   */
  virtual void setAxis(uint32_t axis) = 0;

  /**
   * @brief initialize all variables
   */
  virtual void reset() = 0;
};

} // namespace NPUC
#endif // __SUPPORTED_OPS_H__
