#ifndef __SUPPORTED_OPERATION_OPTIONS_H__
#define __SUPPORTED_OPERATION_OPTIONS_H__

#include "nnmodel/NNNetwork.h"
#include "capability/NPUCtype.h"

namespace NPUC
{

/**
 * @brief A class to set SupportedOperation options for NPU compiler
 * EDEN Driver should pass SupportedOperationOptions (kernel size, stride size, padding size etc.)
 * to NPU copmpiler
 * to check if a operation is supported by NPU compiler.
 * If a operation does not have those values, there is no need to set them.
 */
class SupportedOperationOptions
{
public:
  SupportedOperationOptions() = default;

  /**
   * @brief set kernel size to check if operation is supported according kerenl size
   * @param kernel size
   */
  void setKernelSize(uint32_t kernelSize) { _kernelSize = kernelSize; }

  /**
   * @brief set stride size if operation is supported according stride size
   * @param stride size
   */
  void setStrideSize(uint32_t strideSize) { _strideSize = strideSize; }

  /**
   * @brief set padding size if operation is supported according padding size
   * @param padding size : the biggest one
   */
  void setPaddingSize(uint32_t paddingSize) { _paddingSize = paddingSize; }

  /**
   * @brief set zero point if operation is supported according zero point size
   * @param zero point
   */
  void setZeroPoint(uint32_t zeroPoint) { _zeroPoint = zeroPoint; }

  /**
   * @brief set axis in order to check if operation is supported according to axis (for example,
   * concat)
   * @param axis
   */
  void setAxis(uint32_t axis) { _axis = axis; }

  uint32_t getKernelSize() { return _kernelSize; }

  uint32_t getStrideSize() { return _strideSize; }

  uint32_t getPaddingSize() { return _paddingSize; }

  uint32_t getZeroPoint() { return _zeroPoint; }

  uint32_t getAxis() { return _axis; }

private:
  uint32_t _kernelSize = 0;
  uint32_t _strideSize = 0;
  uint32_t _paddingSize = 0;
  uint32_t _zeroPoint = 0;
  uint32_t _axis = 0;
};

} // namespace NPUC

#endif //__SUPPORTED_OPERATION_OPTIONS_H__
