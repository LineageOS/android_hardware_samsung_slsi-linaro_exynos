#ifndef __NNCONVLAYER_H__
#define __NNCONVLAYER_H__

#include "NNLayer.h"

namespace NPUC
{

/**
 * @brief A class stores convolution attributes
 */
class ConvAttr : public LayerAttr
{
public:
/**
 * @brief ConvAttr constructor
 */
ConvAttr() :_group(0), _inChannel(0), _outChannel(0), _hasBias(false), _fusedAct(FusedActivation::FUSED_ACT_NONE) {}
/**
 * @brief Sets the pad to convolution layer
 * @param pad the pad of convolution layer
 */
void setPad(const std::vector<int> &pad) { _pad = pad; }
/**
 * @brief Sets the stride to convolution layer
 * @param stride the stride of convolution layer
 */
void setStride(const std::vector<int> &stride) { _stride = stride; }
/**
 * @brief Sets the dilation to convolution layer
 * @param dilation the dilation of convolution layer
 */
void setDilation(const std::vector<int> &dilation) { _dilation = dilation; }
/**
 * @brief Sets the group to convolution layer
 * @param group the group of convolution layer
 */
void setGroup(int group) { _group = group; }
/**
 * @brief Sets the type to convolution layer
 * @param type the type of convolution layer
 */
void setConvType(std::string type) { _convType = type; }
/**
 * @brief Sets the pad type to convolution layer
 * @param type the pad type of convolution layer
 */
void setPadType(std::string type) { _padType = type; }
/**
 * @brief Sets the kernel name to convolution layer
 * @param kernel the kernel name of convolution layer
 */
void setKernelName(std::string kernel) { _kernelName = kernel; }
/**
 * @brief Sets the name of bias to convolution layer
 * @param bias the name of bias of convolution layer
 */
void setBiasName(std::string bias) { _biasName = bias; }
/**
 * @brief Sets kernel information to convolution layer
 * @param inChannel the channel of input
 * @param outChannel the channel of output
 * @param kernel the information of kernel
 */
void setKernelInfo(int inChannel, int outChannel, const std::vector<int> &kernel)
{
  _inChannel = inChannel;
  _kernel = kernel;
  // for convolution and deconvolution
  if (_convType == "Conv2D" || _convType == "Conv2DBackpropInput")
  {
    _outChannel = outChannel;
    _group = 1;
  }
  // for depthwise convolution
  else
  {
    _outChannel = inChannel;
    _group = inChannel;
  }
}
/**
 * @brief Sets whether bias data exists in convolution layer
 * @param hasBias true if layer has bias data; otherwise, false
 */
void setHasBias(bool hasBias) { _hasBias = hasBias; }
/**
 * @brief Sets kernel data to convolution layer
 * @param data the point to kernel data
 */
void setkernelData(const std::shared_ptr<std::vector<float>> &data) { _kernelData = data; }
/**
 * @brief Sets quant kernel data to convolution layer
 * @param data the point to quant kernel data (S8)
 */
void setQuantKernelData(const std::shared_ptr<std::vector<int8_t>> &data) { _quantKernelData = data; }
/**
 * @brief Sets quant kernel data to convolution layer
 * @param data the point to quant kernel data (U8)
 */
void setQuantKernelData_U8(const std::shared_ptr<std::vector<uint8_t>> &data) { _quantKernelData_U8 = data; }
/**
 * @brief Sets bias data to convolution layer
 * @param data the point to bias data
 */
void setBiasData(const std::shared_ptr<std::vector<float>> &data) { _biasData = data; }
/**
 * @brief Sets quant bias data to convolution layer
 * @param data the point to quant bias data
 */
void setQuantBiasData(const std::shared_ptr<std::vector<int32_t>> &data) { _quantBiasData = data; }
/**
 * @brief Sets input scale data to convolution layer
 * @param data the point to scale data
 */
void setInputScale(const std::vector<float> inputScale) { _inputScale = inputScale; }
/**
 * @brief Sets input zeroPoint data to convolution layer
 * @param data the point to zeroPoint data
 */
void setInputZeroPoint(const std::vector<uint32_t> inputZeroPoint) { _inputZeroPoint = inputZeroPoint; }

/**
 * @brief Sets output scale data to convolution layer
 * @param data the point to scale data
 */
void setOutputScale(const std::vector<float> outputScale) { _outputScale = outputScale; }
/**
 * @brief Sets output zeroPoint data to convolution layer
 * @param data the point to zeroPoint data
 */
void setOutputZeroPoint(const std::vector<uint32_t> outputZeroPoint) { _outputZeroPoint = outputZeroPoint; }

/**
* @brief Sets weight scale data to convolution layer
* @param data the point to scale data
*/
void setWeightScale(const std::vector<float> weightScale) { _weightScale = weightScale; }
/**
 * @brief Sets output zeroPoint data to convolution layer
 * @param data the point to zeroPoint data
 */
void setWeightZeroPoint(const std::vector<uint32_t> weightZeroPoint) { _weightZeroPoint = weightZeroPoint; }

/**
* @brief Sets bias scale data to convolution layer
* @param data the point to scale data
*/
void setBiasScale(const std::vector<float> biasScale) { _biasScale = biasScale; }
/**
 * @brief Sets bias zeroPoint data to convolution layer
 * @param data the point to zeroPoint data
 */
void setBiasZeroPoint(const std::vector<uint32_t> biasZeroPoint) { _biasZeroPoint = biasZeroPoint; }
/**
 * @brief set FusedActivation Type of convolution layer
 */
void setFusedActivation(int32_t actType)
{
  switch (actType)
  {
  case FUSED_ACT_NONE:
    _fusedAct = NPUC::FusedActivation::FUSED_ACT_NONE;
    break;
  case FUSED_ACT_RELU:
    _fusedAct = NPUC::FusedActivation::FUSED_ACT_RELU;
    break;
  case FUSED_ACT_RELU1:
  case FUSED_ACT_RELU6:
  case FUSED_ACT_TANH:
  case FUSED_ACT_LOGISTIC:
    LOG_AT(NN_MODEL, ERROR)<< "Unsupported FusedActivation type : " << actType << std::endl;
    break;
  default:
    LOG_AT(NN_MODEL, ERROR)<< "Unsupported FusedActivation type : " << actType << std::endl;
    break;
  }
}

/**
 * @brief Gets input scale data to convolution layer
 * @return scale data
 */
std::vector<float> getInputScale() {   return _inputScale; }
/**
 * @brief Gets input zeroPoint data to convolution layer
 * @return zeroPoint data
 */
std::vector<uint32_t> getInputZeroPoint() { return _inputZeroPoint; }

/**
 * @brief Gets output scale data to convolution layer
 * @return scale data
 */
std::vector<float> getOutputScale() {   return _outputScale; }
/**
 * @brief Gets output zeroPoint data to convolution layer
 * @return zeroPoint data
 */
std::vector<uint32_t> getOutputZeroPoint() { return _outputZeroPoint; }
/**
 * @brief Gets weight scale data to convolution layer
 * @return scale data
 */
std::vector<float> getWeightScale() {   return _weightScale; }
/**
 * @brief Gets weight zeroPoint data to convolution layer
 * @return zeroPoint data
 */
std::vector<uint32_t> getWeightZeroPoint() { return _weightZeroPoint; }
/**
 * @brief Gets bias scale data to convolution layer
 * @return scale data
 */
std::vector<float> getBiasScale() {   return _biasScale; }
/**
 * @brief Gets bias zeroPoint data to convolution layer
 * @return zeroPoint data
 */
std::vector<uint32_t> getBiasZeroPoint() { return _biasZeroPoint; }
/**
 * @brief Gets kernel to convolution layer
 * @return scale data
 */
const std::vector<int> &getKernel() { return _kernel; }
/**
 * @brief Gets the stride of convolution layer
 * @return the stride
 */
const std::vector<int> &getStride() { return _stride; }
/**
 * @brief Gets the dilation of convolution layer
 * @return the dilation
 */
const std::vector<int> &getDilation() { return _dilation; }
/**
 * @brief Gets the pad of convolution layer
 * @return the pad
 */
const std::vector<int> &getPad() { return _pad; }
/**
 * @brief Gets the group in convolution layer
 * @return the group
 */
int getGroup() { return _group; }
/**
 * @brief Gets the channel of input shape
 * @return the channel of input shape
 */
int getInChannel() { return _inChannel; }
/**
 * @brief Gets the channel of output shape
 * @return the channel of output shape
 */
int getOutChannel() { return _outChannel; }
/**
 * @brief Gets the type of convolution
 * @return the type of convolution
 */
const std::string &getConvType() { return _convType; }
/**
 * @brief Gets the type of pad in convolution layer
 * @return the type of pad
 */
const std::string &getPadType() { return _padType; }
/**
 * @brief Gets the name of kernel in convolution layer
 * @return the name of kernel
 */
const std::string &getKernelName() { return _kernelName; }
/**
 * @brief Gets the name of bias in convolution layer
 * @return the name of bias
 */
const std::string &getBiasName() { return _biasName; }
/**
 * @brief Check whether bias data exists in convolution layer
 * @return true if layer has bias data; otherwise, false
 */
bool getHasBias() { return _hasBias; }
/**
 * @brief Gets kernel data of convolution layer
 * @return the point to kernel data
 */
std::shared_ptr<std::vector<float>> getKernelData() { return _kernelData; }
/**
 * @brief Gets quant kernel data of convolution layer
 * @return the point to quant kernel data (S8)
 */
std::shared_ptr<std::vector<int8_t>> getQuantKernelData() { return _quantKernelData; }
/**
 * @brief Gets quant kernel data of convolution layer
 * @return the point to quant kernel data (U8)
 */
std::shared_ptr<std::vector<uint8_t>> getQuantKernelData_U8() { return _quantKernelData_U8; }
/**
 * @brief Gets bias data of convolution layer
 * @return the point to bias data
 */
std::shared_ptr<std::vector<float>> getBiasData() { return _biasData; }
/**
 * @brief Gets quant bias data of convolution layer
 * @return the point to quant bias data
 */
std::shared_ptr<std::vector<int32_t>> getQuantBiasData() { return _quantBiasData; }
/**
 * @brief get FusedActivation Type of convolution layer
 */
NPUC::FusedActivation getFusedActivation() { return _fusedAct; }

/**
 * @brief Prints the attribute of convolution layer
 */
void printAttr()
{
  LOG_AT(NN_MODEL, VERBOSE) << "    PadType is   " << _padType << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "    Pad are      " << _pad[0] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _pad[1] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _pad[2] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _pad[3] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "    Stride are   " << _stride[0] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _stride[1] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _stride[2] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _stride[3] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "    Dilation are " << _dilation[0] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _dilation[1] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _dilation[2] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _dilation[3] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "    Kernel are   " << _kernel[0] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "                 " << _kernel[1] << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "    Group is     " << _group << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "    Kernel is    " << _kernelName << std::endl;
  LOG_AT(NN_MODEL, VERBOSE) << "    Bias is      " << _biasName << std::endl;
  if (_inputScale.size() > 0)
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Input scale are" << std::endl;
    for (float inscale : _inputScale)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << inscale << std::endl;
    }
  }
  if (_inputZeroPoint.size() > 0)
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Input zp are   " << std::endl;
    for (uint32_t inzp : _inputZeroPoint)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << inzp << std::endl;
    }
  }
  if (_weightScale.size() > 0)
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Weight scale are" << std::endl;
    for (float weightscale : _weightScale)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << weightscale << std::endl;
    }
  }
  if (_weightZeroPoint.size() > 0)
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Weight zp are  " << std::endl;
    for (uint32_t weightzp : _weightZeroPoint)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << weightzp << std::endl;
    }
  }
  if (_biasScale.size() > 0)
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Bias scale are" << std::endl;
    for (float biasscale : _biasScale)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << biasscale << std::endl;
    }
  }
  if (_biasZeroPoint.size() > 0)
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Bias zp are  " << std::endl;
    for (uint32_t biaszp : _biasZeroPoint)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << biaszp << std::endl;
    }
  }
  if (_outputScale.size() > 0)
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Output scale are" << std::endl;
    for (float outscale : _outputScale)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << outscale << std::endl;
    }
  }
  if (_outputZeroPoint.size() > 0)
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Output zp are  " << std::endl;
    for (uint32_t outzp : _outputZeroPoint)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << outzp << std::endl;
    }
  }
  LOG_AT(NN_MODEL, VERBOSE) << "    FusedActivation is   " << _fusedAct << std::endl;
}

private:
/** @brief the pad of convolution layer*/
std::vector<int> _pad;
/** @brief the stride of convolution layer*/
std::vector<int> _stride;
/** @brief the dilation of convolution layer*/
std::vector<int> _dilation;
/** @brief the group of convolution layer*/
int _group;
/** @brief the input channel of convolution layer*/
int _inChannel;
/** @brief the output channel of convolution layer*/
int _outChannel;
/** @brief the kernel of convolution layer*/
std::vector<int> _kernel;
/** @brief the convolution type of convolution layer*/
std::string _convType;
/** @brief the pad type of convolution layer*/
std::string _padType;
/** @brief the kernel name of convolution layer*/
std::string _kernelName;
/** @brief the bias name of convolution layer*/
std::string _biasName;
/** @brief check whether bias exits in convolution layer*/
bool _hasBias;
/** @brief the kernel data of convolution layer*/
std::shared_ptr<std::vector<float>> _kernelData = nullptr;
/** @brief the quant kernel data of convolution layer (S8)*/
std::shared_ptr<std::vector<int8_t>> _quantKernelData = nullptr;
/** @brief the quant kernel data of convolution layer (U8)*/
std::shared_ptr<std::vector<uint8_t>> _quantKernelData_U8 = nullptr;
/** @brief the bias data of convolution layer*/
std::shared_ptr<std::vector<float>> _biasData = nullptr;
/** @brief the quant bias data of convolution layer*/
std::shared_ptr<std::vector<int32_t>> _quantBiasData = nullptr;
/** @brief the input quantization scale data of convolution layer*/
std::vector<float> _inputScale;
/** @brief the input quantization zeroPoint data of convolution layer*/
std::vector<uint32_t> _inputZeroPoint;
/** @brief the output quantization scale data of convolution layer*/
std::vector<float> _outputScale;
/** @brief the output quantization zeroPoint data of convolution layer*/
std::vector<uint32_t> _outputZeroPoint;
/** @brief the weight quantization scale data of convolution layer*/
std::vector<float> _weightScale;
/** @brief the weight quantization zeroPoint data of convolution layer*/
std::vector<uint32_t> _weightZeroPoint;
/** @brief the bias quantization scale data of convolution layer*/
std::vector<float> _biasScale;
/** @brief the bias quantization zeroPoint data of convolution layer*/
std::vector<uint32_t> _biasZeroPoint;
/** @brief FusedActivation of convolution layer*/
NPUC::FusedActivation _fusedAct;
};

/**
 * @brief A class of Neural Network convolution layer
 */
class NNConvLayer : public NNLayer
{
public:
/**
 * @brief NNConvLayer constructor
 * @param name the name of the layer
 * @param type the type of the layer
 * @param inputs the all the name of inputs to the layer
 * @param outputs the all the name of outputs from the layer
 * @param attr the attribute of the layer
 */
NNConvLayer(std::string name, std::string type, std::vector<std::string> inputs,
            std::vector<std::string> outputs, std::shared_ptr<NPUC::ConvAttr> attr)
        : NNLayer(name, type, inputs, outputs, attr)
{
}

/**
 * @brief Computes the shape of output with the shape of inputs
 * @param inputShape the shape of inputs
 */
void computeOutputShape(const std::vector<std::vector<int>> &inputShape)
{
  bool dataformat_NCHW = (CompileOptions::GetInstance()->getDataFormat() == DataFormat::NCHW);
  std::vector<int> inShape = inputShape[0];
  std::vector<int> outShape;
  std::shared_ptr<NPUC::ConvAttr> attr = std::static_pointer_cast<NPUC::ConvAttr>(_attr);
  std::string padType = attr->getPadType();
  std::vector<int> stride = attr->getStride();
  std::vector<int> dilation = attr->getDilation();
  std::vector<int> kernel = attr->getKernel();
  int outChannel = attr->getOutChannel();
  int eFilterHeight = 0;
  int eFilterWidth = 0;
  if (dataformat_NCHW)
  {
    eFilterHeight = (kernel[0] - 1) * dilation[2] + 1;
    eFilterWidth = (kernel[1] - 1) * dilation[3] + 1;
  }
  else
  {
    eFilterHeight = (kernel[0] - 1) * dilation[1] + 1;
    eFilterWidth = (kernel[1] - 1) * dilation[2] + 1;
  }

  if (padType == "VALID")
  {
    if (dataformat_NCHW)
    {
      int outHeight = (inShape[2] - eFilterHeight + stride[2]) / stride[2];
      int outWidth = (inShape[3] - eFilterWidth + stride[3]) / stride[3];
      outShape.push_back(inShape[0]);
      outShape.push_back(outChannel);
      outShape.push_back(outHeight);
      outShape.push_back(outWidth);

      std::vector<int> pad;
      pad.push_back(0);
      pad.push_back(0);
      pad.push_back(0);
      pad.push_back(0);
      attr->setPad(pad);

      if ((inShape[2] - eFilterHeight + stride[2]) != stride[2] * outHeight ||
          (inShape[3] - eFilterWidth + stride[3]) != stride[3] * outWidth)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "Convolution: " << _name << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "Padding in Convolution has negative value! " << std::endl;
      }
    }
    else
    {
      int outHeight = (inShape[1] - eFilterHeight + stride[1]) / stride[1];
      int outWidth = (inShape[2] - eFilterWidth + stride[2]) / stride[2];
      outShape.push_back(inShape[0]);
      outShape.push_back(outHeight);
      outShape.push_back(outWidth);
      outShape.push_back(outChannel);

      std::vector<int> pad;
      pad.push_back(0);
      pad.push_back(0);
      pad.push_back(0);
      pad.push_back(0);
      attr->setPad(pad);

      if ((inShape[1] - eFilterHeight + stride[1]) != stride[1] * outHeight ||
          (inShape[2] - eFilterWidth + stride[2]) != stride[2] * outWidth)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "Convolution: " << _name << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "Padding in Convolution has negative value! " << std::endl;
      }
    }
  }
  else if (padType == "SAME")
  {
    if (dataformat_NCHW)
    {
      int outHeight = (inShape[2] + stride[2] - 1) / stride[2];
      int outWidth = (inShape[3] + stride[3] - 1) / stride[3];
      outShape.push_back(inShape[0]);
      outShape.push_back(outChannel);
      outShape.push_back(outHeight);
      outShape.push_back(outWidth);

      std::vector<int> pad;
      int padNeedHeight = (outHeight - 1) * stride[2] + eFilterHeight - inShape[2];
      pad.push_back(padNeedHeight / 2);
      pad.push_back(padNeedHeight - pad[0]);
      int padNeedWidth = (outWidth - 1) * stride[3] + eFilterWidth - inShape[3];
      pad.push_back(padNeedWidth / 2);
      pad.push_back(padNeedWidth - pad[2]);

      if (pad[0] != pad[1] || pad[2] != pad[3])
      {
        LOG_AT(NN_MODEL, VERBOSE) << "Convolution: " << _name << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << pad[0] << " " << pad[1] << " " << pad[2] << " " << pad[3]
                                  << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "Padding in Convolution is Not Symmetrical! " << std::endl;
      }

      attr->setPad(pad);
    }
    else
    {
      int outHeight = (inShape[1] + stride[1] - 1) / stride[1];
      int outWidth = (inShape[2] + stride[2] - 1) / stride[2];
      outShape.push_back(inShape[0]);
      outShape.push_back(outHeight);
      outShape.push_back(outWidth);
      outShape.push_back(outChannel);

      std::vector<int> pad;
      int padNeedHeight = (outHeight - 1) * stride[1] + eFilterHeight - inShape[1];
      pad.push_back(padNeedHeight / 2);
      pad.push_back(padNeedHeight - pad[0]);
      int padNeedWidth = (outWidth - 1) * stride[2] + eFilterWidth - inShape[2];
      pad.push_back(padNeedWidth / 2);
      pad.push_back(padNeedWidth - pad[2]);

      if (pad[0] != pad[1] || pad[2] != pad[3])
      {
        LOG_AT(NN_MODEL, VERBOSE) << "Convolution: " << _name << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << pad[0] << " " << pad[1] << " " << pad[2] << " " << pad[3]
                                  << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "Padding in Convolution is Not Symmetrical! " << std::endl;
      }

      attr->setPad(pad);
    }
  }
  else if (padType == "EXPLICIT")
  {
    if (dataformat_NCHW)
    {
      int outHeight = (inShape[2] - eFilterHeight + stride[2]) / stride[2];
      int outWidth = (inShape[3] - eFilterWidth + stride[3]) / stride[3];
      outShape.push_back(inShape[0]);
      outShape.push_back(outChannel);
      outShape.push_back(outHeight);
      outShape.push_back(outWidth);
      LOG_AT(NN_MODEL, VERBOSE) << "DataFormat: NCHW " << std::endl;
    }
    else
    {
      int outHeight = (inShape[1] - eFilterHeight + stride[1]) / stride[1];
      int outWidth = (inShape[2] - eFilterWidth + stride[2]) / stride[2];
      outShape.push_back(inShape[0]);
      outShape.push_back(outHeight);
      outShape.push_back(outWidth);
      outShape.push_back(outChannel);
      LOG_AT(NN_MODEL, VERBOSE) << "DataFormat: NHWC " << std::endl;    
    }
  }
  else
  {
    LOG_AT(NN_MODEL, VERBOSE) << "pad type error! " << std::endl;
  }

  _shapeMap[_outputs[0]] = outShape;

  for (size_t i = 0; i < inputShape.size(); i++)
  {
    _shapeMap[_inputs[i]] = inputShape[i];
  }
}
};

} // namespace NPUC
#endif // __NNCONVLAYER_H__
