#ifndef __COMPILE_OPTIONS_H__
#define __COMPILE_OPTIONS_H__

#include <string>
#include <cstdint>
#include <map>
#include "capability/NPUCtype.h"
#include "capability/DataType.h"

namespace NPUC
{

/**
 * @brief Compile options are used at NPU compiler
 */
class CompileOptions
{
private:
  CompileOptions();

public:
  static CompileOptions *GetInstance();

  virtual ~CompileOptions() = default;

public:
  bool getVerbose() const { return _verbose; }

  bool getGenJson() const { return _gen_json; }

  const std::string &getOptCompiler() const { return _opt_compiler; }

  void setVerbose(const bool value) { _verbose = value; }

  void setGenJson(const bool value) { _gen_json = value; }

  void setOptCompiler(const std::string &value) { _opt_compiler = value; }

  const std::string &getMLFramework() const { return _ml_framework; }

  const std::string &getObjectLayer() const { return _objectlayer; }

  const std::string &getSavePath() const { return _savepath; }

  const std::string &getMode() const { return _mode; }

  const std::string &getTargetLayer() const { return _targetlayer; }

  const std::string &getBinaryDataPath() const { return _binary_data_path; }

  const std::string &getIntermediateOutput() const { return _intermediate_output; }

  const std::string &getInputFormat() const { return _input_format; }

  /**
   * @brief get overall data type of input, weight, output.
   * @details
   * There are generally two quantization modes: symmetric and asymmetric.
   * But NPUC has two more modes:  QUASI_SYM and PERF_SYM because of SOC type constrains
   * (for example, makalu does not support asymmetric mode.
   * To do it, we make a fake symmetric mode.), or performance
   * (for example, TFLite-gen(EDEN) skips normalization pre-processing to improve performance).
   *  - ASYM and SYM mode: the input, weight, output's data types are same (U08, S08, S16)
   *  - QUASI_SYM mode: the data types are same except the first input data type
   *    (U08: input, S08: weight/output)
   *    (for example, used only in makalu for ondevice compiler)
   *  - PERF_SYM mode: the data types are same except the weight data type,
   *    but weight's data type is converted in order to make it same as the others.
   *    (S08: input/output, U08: weight)
   *    (for example, InceptionV3_U8 test case)
   */
  const NNDataType &getDataType() const { return _data_type; }

  /**
   * @brief get data type of the first input layer (only for used in PERF_SYM)
   */
  const NNDataType &getFirstInputDataType() const { return _input_data_type; }

  bool getGenData() const { return _gen_data; }

  bool getBuildReference() const { return _buildReference; }

  void setMLFramework(const std::string &value) { _ml_framework = value; }

  void setObjectLayer(const std::string &value) { _objectlayer = value; }

  void setSavePath(const std::string &value) { _savepath = value; }

  void setMode(const std::string &value) { _mode = value; }

  void setTargetLayer(const std::string &value) { _targetlayer = value; }

  void setBinaryDataPath(const std::string &value) { _binary_data_path = value; }

  void setIntermediateOutput(const std::string &value) { _intermediate_output = value; }

  void setInputFormat(const std::string &value) { _input_format = value; }

  void setFirstInputDataType(const NNDataType &value) { _input_data_type = value; }

  void setDataType(const NNDataType &value) { _data_type = value; }

  void setGenData(const bool value) { _gen_data = value; }

  void setBuildReference(const bool value) { _buildReference = value; }

  uint32_t getBitWidth() const { return _bitwidth; }

  int getSRAMSize() const { return _sramsize; }

  void setBitWidth(const uint32_t value) { _bitwidth = value; }

  void setSRAMSize(const int value) { _sramsize = value; }

  const std::string &getProto() const { return _proto; }

  const std::string &getModel() const { return _model; }

  void setProto(const std::string &value) { _proto = value; }

  void setModel(const std::string &value) { _model = value; }

  /**
   * @brief SOCType is used to distinguish NPU hardware types (such as, makalu, 2020, KITT, NEUS,
   * etc.)
   */
  SOCType getSOCType() const { return _socType; }

  void setSOCType(SOCType socType) { _socType = socType; }

  /**
   * @brief SOCRev is used to distinguish NPU hardware revisions (EVT0, EVT1, etc)
   */
  SOCRev getSOCRev() const { return _socRev; }

  void setSOCRev(SOCRev socRev) { _socRev = socRev; }

  /**
   * @brief CompilerType is used to distinguish compiler types (such as, ondevice or offline)
   */
  CompilerType getCompilerType() const { return _compilerType; }

  void setCompilerType(CompilerType compilerType) { _compilerType = compilerType; }

  /**
   * @brief DataFormat is used to distinguish data format (NCHW or NHWC)
   */
  DataFormat getDataFormat() const { return _dataFormat; }

  void setDataFormat(DataFormat dataFormat) { _dataFormat = dataFormat; }

  /**
   * @brief WeightDataFormat descrbes data format (NCHW or NHWC) stored at data array
   */
  DataFormat getWeightDataFormatOfArray() const { return _weightDataFormat; }

  void setWeightDataFormatOfArray(DataFormat weightDataFormat)
  {
    _weightDataFormat = weightDataFormat;
  }

  /**
   * @brief Quantization mode is used to configure symmetric or asymmetric modes
   */
  QuantizationMode getQuantizationMode() const { return _quantizationMode; }

  void setQuantizationMode(QuantizationMode quantizationMode)
  {
    _quantizationMode = quantizationMode;
  }

private:
  /**
   * @brief The following compile options are to generate NNGraphIR (NNGraphCgObject)
   * @details These options are set by parsing command line arguments.
   * Model parser, NNGraph, NNGraphFiller, NNGraphCgObject use these options.
   */
  // General options
  bool _verbose;
  bool _gen_json;
  std::string _opt_compiler;

  // Frontend options
  std::string _ml_framework;
  std::string _objectlayer;
  std::string _savepath;
  std::string _mode;
  std::string _targetlayer;
  std::string _binary_data_path;
  std::string _intermediate_output;
  std::string _input_format;

  /**
   * @brief The following compile options are to configure input, weight, output data type(dtype)
   */
  NNDataType _input_data_type;                   // only used for perf_symmetric mode
  NNDataType _data_type = NNDataType::DT_SINT08; // overall data type (Originally,
                                                 // input/weight/output datatypes are same)

  bool _gen_data;
  bool _buildReference;

  // Configuration options
  uint32_t _bitwidth;
  int _sramsize;

  // Model options
  std::string _proto;
  std::string _model;

  /**
   * @brief The following options are used at all components.
   */

  // SOC type
  SOCType _socType;
  SOCRev _socRev;

  // Compiler type
  CompilerType _compilerType;

  // Data format
  DataFormat _dataFormat;

  // Weight Data format in array
  DataFormat _weightDataFormat = DataFormat::NCHW;

  // Quantization mode
  QuantizationMode _quantizationMode;

public:
  /**
   * @brief The following options are used at backend side.
   * @details Configuration's values are set by parsing command line arguments.
   * enable_npuc_be and Arguments are used only in backend component.
   */
  bool enable_npuc_be;
  std::map<std::string, std::string> Arguments;
};

} // namespace NPUC

#endif // __COMPILE_OPTIONS_H__
