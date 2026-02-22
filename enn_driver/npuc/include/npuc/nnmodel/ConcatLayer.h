#ifndef __NNCONCATLAYER_H__
#define __NNCONCATLAYER_H__

#include "NNLayer.h"

namespace NPUC
{

/**
 * @brief A class stores concat attributes
 */
class ConcatAttr : public LayerAttr
{
public:
  /**
   * @brief ConcatAttr constructor
   */
  ConcatAttr() : _axis(0) {}
  /**
   * @brief Sets the axis to concat layer
   * @param axis the axis of concat layer
   */
  void setAxis(int axis) { _axis = axis; }
  /**
   * @brief Gets the axis of concat layer
   * @return the axis of concat layer
   */
  int getAxis() { return _axis; }
  /**
   * @brief Sets output scale data to concat layer
   * @param data the point to scale data
   */
  void setOutputScale(const std::vector<float> outputScale) { _outputScale = outputScale; }
  /**
   * @brief Gets output scale data to concat layer
   * @return scale data
   */
  std::vector<float> getOutputScale() {   return _outputScale; }
  /**
   * @brief Prints the attribute of concat layer
   */
  void printAttr() { LOG_AT(NN_MODEL, VERBOSE) << "    Axis is      " << _axis << std::endl; }
private:
  /** @brief the axis of concat layer*/
  int _axis;
  /** @brief the output quantization scale data of concat layer*/
  std::vector<float> _outputScale;
};

/**
 * @brief A class of Neural Network pooling layer
 */
class NNConcatLayer : public NNLayer
{
public:
  /**
   * @brief NNConcatLayer constructor
   * @param name the name of the layer
   * @param type the type of the layer
   * @param inputs the all the name of inputs to the layer
   * @param outputs the all the name of outputs from the layer
   * @param attr the attribute of the layer
   */
  NNConcatLayer(std::string name, std::string type, std::vector<std::string> inputs,
                std::vector<std::string> outputs, std::shared_ptr<NPUC::ConcatAttr> attr)
      : NNLayer(name, type, inputs, outputs, attr)
  {
  }

  /**
   * @brief Computes the shape of output with the shape of inputs
   * @param inputShape the shape of inputs
   */
  void computeOutputShape(const std::vector<std::vector<int>> &inputShape)
  {
    std::shared_ptr<NPUC::ConcatAttr> attr = std::static_pointer_cast<NPUC::ConcatAttr>(_attr);
    size_t axis = attr->getAxis();
    int axisSum = 0;

    for (size_t i = 0; i < inputShape.size(); i++)
    {
      axisSum += inputShape[i][axis];
    }

    std::vector<int> shape;
    for (size_t i = 0; i < inputShape[0].size(); i++)
    {
      if (i != axis)
      {
        shape.push_back(inputShape[0][i]);
      }
      else
      {
        shape.push_back(axisSum);
      }
    }

    _shapeMap[_outputs[0]] = shape;

    for (size_t i = 0; i < inputShape.size(); i++)
    {
      _shapeMap[_inputs[i]] = inputShape[i];
    }
  }
};

} // namespace NPUC
#endif // __NNCONCATLAYER_H__
