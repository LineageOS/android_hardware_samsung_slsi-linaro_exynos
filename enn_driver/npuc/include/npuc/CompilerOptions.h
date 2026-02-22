#ifndef __NPUC_COMPILER_OPTIONS_H__
#define __NPUC_COMPILER_OPTIONS_H__

#include "nnmodel/NNNetwork.h"
#include "capability/NPUCtype.h"

namespace NPUC
{

/**
 * @brief A class to set options for NPU compiler
 * EDEN Driver should pass SOCType (makalu, 2020, KITT etc.) to NPU copmpiler.
 */
class NPUCCompilerOptions
{
public:
  NPUCCompilerOptions() = default;

  void setSOCType(NPUC::SOCType socType) { _socType = socType; }
  NPUC::SOCType getSOCType() const { return _socType; }

  /**
   * @brief Quantization mode to configure Symmetric or Asymmetric mode
   */
  void setQuantizationMode(QuantizationMode quantizationMode)
  {
    _quantizationMode = quantizationMode;
  }
  NPUC::QuantizationMode getQuantizationMode() { return _quantizationMode; }

  /**
   * @brief WeightDataFormat descrbes data format (NCHW or NHWC) stored at data array
   */
  DataFormat getWeightDataFormatOfArray() const { return _weightDataFormat; }

  void setWeightDataFormatOfArray(DataFormat weightDataFormat)
  {
    _weightDataFormat = weightDataFormat;
  }

  const std::string &getInputFormat() const { return _input_format; }

  void setInputFormat(const std::string &value) { _input_format = value; }

private:
  // Need to set default value
  // Note that this header is exported only for ondevice compiler
  NPUC::SOCType _socType = NPUC::SOCType::ST_2020;
  NPUC::QuantizationMode _quantizationMode = NPUC::QuantizationMode::ASYM;
  // Weight Data format in array
  DataFormat _weightDataFormat = DataFormat::NCHW;
  std::string _input_format = "NO_FORMAT";
};

} // namespace NPUC

#endif //__NPUC_COMPILER_OPTIONS_H__
