#ifndef __NNPOOLLAYER_H__
#define __NNPOOLLAYER_H__

#include "NNLayer.h"

namespace NPUC
{

/**
 * @brief   A class for pooling layer attribute
 *          It has kernel, stride, pad and type information
 */
class PoolAttr : public LayerAttr
{
public:
  /**
   * @brief   Default construction of PoolAttr
   */
  PoolAttr() : _fusedAct(FusedActivation::FUSED_ACT_NONE) {}

  /**
   * @brief     Set the pooling type(max, ave, or ...) of pooling layer
   * @param     type    The pooling type
   */
  void setPoolType(std::string type) { _poolType = type; }

  /**
   * @brief     Set the pad size of pooling layer
   * @param     pad    The pad size
   */
  void setPad(const std::vector<int> &pad) { _pad = pad; }

  /**
   * @brief     Set the stride size of pooling layer
   * @param     stride    The stride size
   */
  void setStride(const std::vector<int> &stride) { _stride = stride; }

  /**
   * @brief     Set the kernel size of pooling layer
   * @param     kernel    The kernel size
   */
  void setKernel(const std::vector<int> &kernel) { _kernel = kernel; }

  /**
   * @brief     Set the pad type(valid or same) of pooling layer
   * @param     type    The pad type
   */
  void setPadType(std::string type) { _padType = type; }

  /**
   * @brief Sets input scale data to pooling layer
   * @param data the point to scale data
   */
  void setInputScale(const std::vector<float> inputScale) { _inputScale = inputScale; }
  /**
   * @brief Sets input zeroPoint data to pooling layer
   * @param data the point to zeroPoint data
   */
  void setInputZeroPoint(const std::vector<uint32_t> inputZeroPoint) { _inputZeroPoint = inputZeroPoint; }

  /**
   * @brief Sets output scale data to pooling layer
   * @param data the point to scale data
   */
  void setOutputScale(const std::vector<float> outputScale) { _outputScale = outputScale; }
  /**
   * @brief Sets output zeroPoint data to pooling layer
   * @param data the point to zeroPoint data
   */
  void setOutputZeroPoint(const std::vector<uint32_t> outputZeroPoint) { _outputZeroPoint = outputZeroPoint; }
  /**
   * @brief set FusedActivation Type of pooling layer
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
      LOG_AT(NN_MODEL, ERROR) << "Unsupported FusedActivation type : " << actType << std::endl;
      break;
    default:
      LOG_AT(NN_MODEL, ERROR) << "Unsupported FusedActivation type : " << actType << std::endl;
      break;
    }
  }

  /**
   * @brief     Get the pooling type(max, ave, or ...) of pooling layer
   * @return    The pooling type
   */
  const std::string &getPoolType() { return _poolType; }

  /**
   * @brief     Get the kernel size of pooling layer
   * @return    The kernel size
   */
  const std::vector<int> &getKernel() { return _kernel; }

  /**
   * @brief     Get the stride size of pooling layer
   * @return    The stride size
   */
  const std::vector<int> &getStride() { return _stride; }

  /**
   * @brief     Get the pad size of pooling layer
   * @return    The pad size
   */
  const std::vector<int> &getPad() { return _pad; }

  /**
   * @brief     Get the pad type(valid or same) of pooling layer
   * @return    The pad type
   */
  const std::string &getPadType() { return _padType; }
  /**
   * @brief Gets input scale data to pooling layer
   * @return scale data
   */
  std::vector<float> getInputScale() {   return _inputScale; }
  /**
   * @brief Gets input zeroPoint data to pooling layer
   * @return zeroPoint data
   */
  std::vector<uint32_t> getInputZeroPoint() { return _inputZeroPoint; }

  /**
   * @brief Gets output scale data to pooling layer
   * @return scale data
   */
  std::vector<float> getOutputScale() {   return _outputScale; }
  /**
   * @brief Gets output zeroPoint data to pooling layer
   * @return zeroPoint data
   */
  std::vector<uint32_t> getOutputZeroPoint() { return _outputZeroPoint; }
  /**
   * @brief get FusedActivation Type of pooling layer
   */
  NPUC::FusedActivation getFusedActivation() { return _fusedAct; }

  /**
   * @brief     Print the pooling attribute
   */
  void printAttr()
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    PoolType is  " << _poolType << std::endl;
    LOG_AT(NN_MODEL, VERBOSE) << "    PadType is   " << _padType << std::endl;
    if (_stride.size() > 0)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "    Stride are" << std::endl;
      for (float stride : _stride)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "                 " << stride << std::endl;
      }
    }
    if (_kernel.size() > 0)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "    Kernel are" << std::endl;
      for (float kernel : _kernel)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "                 " << kernel << std::endl;
      }
    }
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
  };

private:
  /** @brief the pad size of pooling*/
  std::vector<int> _pad;
  /** @brief the stride of pooling*/
  std::vector<int> _stride;
  /** @brief the kernel of pooling*/
  std::vector<int> _kernel;
  /** @brief the pad type(valid or same) of pooling*/
  std::string _padType;
  /** @brief the pool type(max, ave, or ...) of pooling*/
  std::string _poolType;
  /** @brief the input quantization scale data of pooling layer*/
  std::vector<float> _inputScale;
  /** @brief the input quantization zeroPoint data of pooling layer*/
  std::vector<uint32_t> _inputZeroPoint;
  /** @brief the output quantization scale data of pooling layer*/
  std::vector<float> _outputScale;
  /** @brief the output quantization zeroPoint data of pooling layer*/
  std::vector<uint32_t> _outputZeroPoint;
  /** @brief FusedActivation of pooling layer*/
  NPUC::FusedActivation _fusedAct;
};

/**
 * @brief   A class for NN pooling layer
 */
class NNPoolLayer : public NNLayer
{
public:
  /**
   * @brief     Construction of NNPoolLayer
   * @param     name The name of the layer
   * @param     type The type of the layer
   * @param     inputs The all the name of inputs to the layer
   * @param     outputs The all the name of outputs from the layer
   * @param     attr The attribute of the layer
   */
  NNPoolLayer(std::string name, std::string type, std::vector<std::string> inputs,
              std::vector<std::string> outputs, std::shared_ptr<NPUC::PoolAttr> attr)
      : NNLayer(name, type, inputs, outputs, attr)
  {
  }

  /**
   * @brief     Compute the pooling layer output shape
   * @param     inputShape  The input shape of pooling layer
   */
  void computeOutputShape(const std::vector<std::vector<int>> &inputShape)
  {
    std::vector<int> inShape = inputShape[0];
    std::vector<int> outShape;
    std::shared_ptr<NPUC::PoolAttr> attr = std::static_pointer_cast<NPUC::PoolAttr>(_attr);
    std::string padType = attr->getPadType();
    std::vector<int> stride = attr->getStride();
    std::vector<int> kernel = attr->getKernel();
    bool dataformat_NCHW = (CompileOptions::GetInstance()->getDataFormat() == DataFormat::NCHW);

    if (padType == "VALID")
    {
      for (size_t i = 0; i < inShape.size(); i++)
      {
        int out = (inShape[i] - kernel[i] + stride[i]) / stride[i];
        LOG_AT(NN_MODEL, VERBOSE) << "InShape[" << i << "]: " << inShape[i] << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "OutShape[" << i << "]: " << out << std::endl;
        outShape.push_back(out);

        if ((inShape[i] - kernel[i] + stride[i]) != stride[i] * out)
        {
          LOG_AT(NN_MODEL, VERBOSE) << "Pooling: " << _name << std::endl;
          LOG_AT(NN_MODEL, VERBOSE) << "Padding in Pooling has negative value! " << std::endl;
        }
      }

      std::vector<int> pad;
      pad.push_back(0);
      pad.push_back(0);
      pad.push_back(0);
      pad.push_back(0);
      attr->setPad(pad);
    }
    else if (padType == "SAME")
    {
      for (size_t i = 0; i < inShape.size(); i++)
      {
        int out = (inShape[i] + stride[i] - 1) / stride[i];
        LOG_AT(NN_MODEL, VERBOSE) << "InShape[" << i << "]: " << inShape[i] << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "OutShape[" << i << "]: " << out << std::endl;
        outShape.push_back(out);
      }

      std::vector<int> pad;
      if (dataformat_NCHW)
      {
        int padNeedHeight = (outShape[2] - 1) * stride[2] + kernel[2] - inShape[2];
        pad.push_back(padNeedHeight / 2);
        pad.push_back(padNeedHeight - pad[0]);
        int padNeedWidth = (outShape[3] - 1) * stride[3] + kernel[3] - inShape[3];
        pad.push_back(padNeedWidth / 2);
        pad.push_back(padNeedWidth - pad[2]);
        LOG_AT(NN_MODEL, VERBOSE) << "Pad.Top: " << padNeedHeight / 2 << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "Pad.Bottom: " << padNeedHeight - pad[0] << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "Pad.Left: " << padNeedWidth / 2 << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "Pad.Right: " << padNeedWidth - pad[2] << std::endl;
      }
      else
      {
        int padNeedHeight = (outShape[1] - 1) * stride[1] + kernel[1] - inShape[1];
        pad.push_back(padNeedHeight / 2);
        pad.push_back(padNeedHeight - pad[0]);
        int padNeedWidth = (outShape[2] - 1) * stride[2] + kernel[2] - inShape[2];
        pad.push_back(padNeedWidth / 2);
        pad.push_back(padNeedWidth - pad[2]);
      }

      if (pad[0] != pad[1] || pad[2] != pad[3])
      {
        LOG_AT(NN_MODEL, VERBOSE) << "Pooling: " << _name << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << pad[0] << " " << pad[1] << " " << pad[2] << " " << pad[3]
                                  << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "Padding in Pooling is Not Symmetrical! " << std::endl;
      }

      attr->setPad(pad);
    }
    else if (padType == "EXPLICIT")
    {
      std::vector<int> attrPad = attr->getPad();
      std::vector<int> pad;
      if (dataformat_NCHW)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "EXPLICIT DataFormat: NCHW " << std::endl;
        int outHeight = (inShape[2] - kernel[2] + attrPad[0] + attrPad[1]) / stride[2] + 1;
        int outWidth = (inShape[3] - kernel[3] + attrPad[2] + attrPad[3]) / stride[3] + 1;
        LOG_AT(NN_MODEL, VERBOSE) << "outHeight: " << outHeight << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "outWidth: " << outWidth << std::endl;
        outShape.push_back(inShape[0]);
        outShape.push_back(inShape[1]);
        outShape.push_back(outHeight);
        outShape.push_back(outWidth);
      }
      else
      {
        LOG_AT(NN_MODEL, VERBOSE) << "EXPLICIT DataFormat: NHWC " << std::endl;
        int outHeight = (inShape[1] - kernel[1] + attrPad[0] + attrPad[1]) / stride[1] + 1;
        int outWidth = (inShape[2] - kernel[2] + attrPad[2] + attrPad[3]) / stride[2] + 1;
        LOG_AT(NN_MODEL, VERBOSE) << "outHeight: " << outHeight << std::endl;
        LOG_AT(NN_MODEL, VERBOSE) << "outWidth: " << outWidth << std::endl;
        outShape.push_back(inShape[0]);
        outShape.push_back(outHeight);
        outShape.push_back(outWidth);
        outShape.push_back(inShape[3]);
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
#endif // __NNPOOLLAYER_H__
