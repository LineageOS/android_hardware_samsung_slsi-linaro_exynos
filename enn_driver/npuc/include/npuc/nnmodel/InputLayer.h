#ifndef __NNINPUTLAYER_H__
#define __NNINPUTLAYER_H__

#include "NNLayer.h"

namespace NPUC
{

/**
 * @brief   A class for input layer attribute
 *          It has batch, channel, height and width information
 */
class InputAttr : public LayerAttr
{
public:
  /**
   * @brief   Default construction of InputAttr
   */
  InputAttr() : _batch(0), _channel(0), _height(0), _width(0) {}

  /**
   * @brief     Set the batch of input layer
   * @param     batch    Batch of input layer
   */
  void setBatch(int batch) { _batch = batch; }

  /**
   * @brief     Set the channel of input layer
   * @param     channel    Channel of input layer
   */
  void setChannel(int channel) { _channel = channel; }

  /**
   * @brief     Set the height of input layer
   * @param     height    Height of input layer
   */
  void setHeight(int height) { _height = height; }

  /**
   * @brief     Set the width of input layer
   * @param     width    Width of input layer
   */
  void setWidth(int width) { _width = width; }

  /**
   * @brief     Get the batch of input layer
   * @return    Batch of input layer
   */
  int getBatch() { return _batch; }

  /**
   * @brief     Get the channel of input layer
   * @return    Channel of input layer
   */
  int getChannel() { return _channel; }

  /**
   * @brief     Get the height of input layer
   * @return    Height of input layer
   */
  int getHeight() { return _height; }

  /**
   * @brief     Get the width of input layer
   * @return    Width of input layer
   */
  int getWidth() { return _width; }

  /**
   * @brief     Print the input attribute
   */
  void printAttr()
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    Batch is     " << _batch << std::endl;
    LOG_AT(NN_MODEL, VERBOSE) << "    Channel is   " << _channel << std::endl;
    LOG_AT(NN_MODEL, VERBOSE) << "    Height is    " << _height << std::endl;
    LOG_AT(NN_MODEL, VERBOSE) << "    Width is     " << _width << std::endl;
  }

private:
  /** @brief batch of input layer*/
  int _batch;
  /** @brief channel of input layer*/
  int _channel;
  /** @brief height of input layer*/
  int _height;
  /** @brief width of input layer*/
  int _width;
};

/**
 * @brief   A class for NN input layer
 */
class NNInputLayer : public NNLayer
{
public:
  /**
   * @brief     Construction of NNInputLayer
   * @param     name The name of the layer
   * @param     type The type of the layer
   * @param     inputs The all the name of inputs to the layer
   * @param     outputs The all the name of outputs from the layer
   * @param     attr The attribute of the layer
   */
  NNInputLayer(std::string name, std::string type, std::vector<std::string> inputs,
               std::vector<std::string> outputs, std::shared_ptr<NPUC::InputAttr> attr)
      : NNLayer(name, type, inputs, outputs, attr)
  {
    std::vector<int> shape;
    bool dataformat_NCHW = (CompileOptions::GetInstance()->getDataFormat() == DataFormat::NCHW);
    if (dataformat_NCHW)
    {
      shape.push_back(attr->getBatch());
      shape.push_back(attr->getChannel());
      shape.push_back(attr->getHeight());
      shape.push_back(attr->getWidth());
    }
    else
    {
      shape.push_back(attr->getBatch());
      shape.push_back(attr->getHeight());
      shape.push_back(attr->getWidth());
      shape.push_back(attr->getChannel());
    }
    _shapeMap[_outputs[0]] = shape;
  }

  /**
  * @brief      Compute the input layer output shape
  *             Because the output shape is not dependent on input, so this is an empty function.
  * @param      inputShape  The input shape of input layer
  */
  // TODO if it is different from NNLayer, please add something
  // void computeOutputShape(const std::vector<std::vector<int>> &inputShape) { }
};

} // namespace NPUC
#endif // __NNINPUTLAYER_H__
