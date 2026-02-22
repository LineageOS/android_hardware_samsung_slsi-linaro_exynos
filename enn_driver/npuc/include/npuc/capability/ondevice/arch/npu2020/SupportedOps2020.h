#ifndef __2020_SUPPORTED_OPERATION_H__
#define __2020_SUPPORTED_OPERATION_H__

#include <vector>
#include "Logging.h"

#include "capability/SupportedOps.h"

namespace NPUC
{

/**
 * @brief A derived SupportedOperation class for makalu
 */

class SupportedOperation2020 : public SupportedOperation
{
public:
  SupportedOperation2020() = default;

  ~SupportedOperation2020() = default;

  /**
   * @brief A function to check if it is supported or not
   * @param an operation type in enum OpType
   * @retval bool
   */
  bool isSupportedOp(NPUC::OpType opType) override;

  /**
   * @brief set kernel size in order to check if operation is supported according to kerenl size
   * @param kernel size
   * @retval SupportedOperationMakalu pointer
   */
  void setKernelSize(uint32_t kernelSize) override;

  /**
   * @brief set stride size in order to check if operation is supported according to stride size
   * @param stride size
   * @retval SupportedOperationMakalu pointer
   */
  void setStrideSize(uint32_t strideSize) override;

  /**
   * @brief set padding size in order to check if operation is supported according to padding size
   * @param padding size : the biggest one
   * @retval SupportedOperationMakalu pointer
   */
  void setPaddingSize(uint32_t paddingSize) override;

  /**
   * @brief set zero point in order to check if operation is supported according to zero point size
   * @param zero point
   */
  void setZeroPoint(uint32_t zeroPoint) override;

  /**
   * @brief set axis in order to check if operation is supported according to axis (for example,
   * concat)
   * @param axis
   */
  void setAxis(uint32_t axis) override;

  /**
   * @brief initialize all variables
   */
  void reset() override;

private:
  uint32_t _kernelSize = 0;
  uint32_t _strideSize = 0;
  uint32_t _paddingSize = 0;
  uint32_t _axis = 0;
};

} // namespace NPUC
#endif // __2020_SUPPORTED_OPERATION_H__
