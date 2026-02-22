#ifndef __NNDECONVLAYER_H__
#define __NNDECONVLAYER_H__

#include "NNLayer.h"
#include "ConvLayer.h"

namespace NPUC
{
/**
 * @brief A class of Neural Network deconvolution layer
 */
  class NNDeconvLayer : public NNLayer
  {
  public:
/**
 * @brief NNDeconvLayer constructor
 * @param name the name of the layer
 * @param type the type of the layer
 * @param inputs the all the name of inputs to the layer
 * @param outputs the all the name of outputs from the layer
 * @param attr the attribute of the layer
 */
  NNDeconvLayer(std::string name, std::string type, std::vector<std::string> inputs,
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
    std::vector<int> kernel = attr->getKernel();
    int outChannel = attr->getOutChannel();
    int eFilterHeight = kernel[0];
    int eFilterWidth = kernel[1];

    if (padType == "VALID")
    {
      if (dataformat_NCHW)
      {
        int outHeight = inShape[2] * stride[2] + std::max(eFilterHeight - stride[2], 0);
        int outWidth = inShape[3] * stride[3] + std::max(eFilterWidth - stride[3], 0);
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
      }
      else
      {
        int outHeight = inShape[1] * stride[1] + std::max(eFilterHeight - stride[1], 0);
        int outWidth = inShape[2] * stride[2] + std::max(eFilterWidth - stride[2], 0);
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
      }
    }
    else if (padType == "SAME")
    {
      if (dataformat_NCHW)
      {
        int outHeight = inShape[2] * stride[2];
        int outWidth = inShape[3] * stride[3];
        outShape.push_back(inShape[0]);
        outShape.push_back(outChannel);
        outShape.push_back(outHeight);
        outShape.push_back(outWidth);

        std::vector<int> pad;
        int padNeedHeight = (inShape[2] - 1) * stride[2] + eFilterHeight - outHeight;
        pad.push_back(padNeedHeight / 2);
        pad.push_back(padNeedHeight - pad[0]);
        int padNeedWidth = (inShape[3] - 1) * stride[3] + eFilterWidth - outWidth;
        pad.push_back(padNeedWidth / 2);
        pad.push_back(padNeedWidth - pad[2]);

        if (pad[0] != pad[1] || pad[2] != pad[3])
        {
          LOG_AT(NN_MODEL, VERBOSE) << "Deconvolution: " << _name << std::endl;
          LOG_AT(NN_MODEL, VERBOSE) << pad[0] << " " << pad[1] << " " << pad[2] << " " << pad[3]
                                    << std::endl;
          LOG_AT(NN_MODEL, VERBOSE) << "Padding in Deconvolution is Not Symmetrical! " << std::endl;
        }

        attr->setPad(pad);
      }
      else
      {
        int outHeight = inShape[1] * stride[1];
        int outWidth = inShape[2] * stride[2];
        outShape.push_back(inShape[0]);
        outShape.push_back(outHeight);
        outShape.push_back(outWidth);
        outShape.push_back(outChannel);

        std::vector<int> pad;
        int padNeedHeight = (inShape[1] - 1) * stride[1] + eFilterHeight - outHeight;
        pad.push_back(padNeedHeight / 2);
        pad.push_back(padNeedHeight - pad[0]);
        int padNeedWidth = (inShape[2] - 1) * stride[2] + eFilterWidth - outWidth;
        pad.push_back(padNeedWidth / 2);
        pad.push_back(padNeedWidth - pad[2]);

        if (pad[0] != pad[1] || pad[2] != pad[3])
        {
          LOG_AT(NN_MODEL, VERBOSE) << "Deconvolution: " << _name << std::endl;
          LOG_AT(NN_MODEL, VERBOSE) << pad[0] << " " << pad[1] << " " << pad[2] << " " << pad[3]
                                    << std::endl;
          LOG_AT(NN_MODEL, VERBOSE) << "Padding in Deconvolution is Not Symmetrical! " << std::endl;
        }

        attr->setPad(pad);
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
#endif // __NNDECONVLAYER_H__
