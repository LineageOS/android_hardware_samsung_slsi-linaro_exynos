#ifndef __SUPPORTED_OPS_FACTORY_H__
#define __SUPPORTED_OPS_FACTORY_H__

#include <vector>

#include "capability/SupportedOps.h"

namespace NPUC
{

/**
 * @brief A factory class of SupportedOperation
 */
class SupportedOperationFactory
{
public:
  /**
   * @brief set SOC type
   */
  SupportedOperationFactory(NPUC::SOCType socType) : _socType(socType) {}

  ~SupportedOperationFactory() = default;

  NPUC::SOCType getSOCType() const;

  /**
   * @brief generate SupportedOperation instance according to SOC type
   * @retval  SupportedOperation
   */
  SupportedOperation *getSupportedOpInstance();

  /**
   * @brief delete SupportedOperation instance according to SOC type
   */
  void deleteSupportedOpInstance(SupportedOperation *instance);

private:
  /**
   * @brief SOC type (hareware type): for example, makalu, 2020, neus, etc.
   */
  const NPUC::SOCType _socType;
};

} // namespace NPUC
#endif // __SUPPORTED_OPS_FACTORY_H__
