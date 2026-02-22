#ifndef __NNRESHAPELAYER_H__
#define __NNRESHAPELAYER_H__

#include "NNLayer.h"

namespace NPUC
{

/**
 * @brief   A class for reshape layer attribute
 *          It has type and shape information
 */
class ReshapeAttr : public LayerAttr
{
public:
  /**
   * @brief   Default construction of ReshapeAttr
   */
  ReshapeAttr() {}

  /**
   * @brief     Set the type info(reshape or squeeze) of reshape layer
   * @param     type    The reshape type
   */
  void setType(std::string type) { _type = type; }

  /**
   * @brief     Set the shape info of reshape layer
   * @param     shapeInfo    The shape info
   */
  void setShapeInfo(const std::vector<int> shapeInfo) { _shapeInfo = shapeInfo; }

  /**
   * @brief     Set the reshape param of reshape layer
   * @param     reshapeParam    The reshape param
   */
  void setReshapeParam(const std::vector<int> reshapeParam) { _reshapeParam = reshapeParam; }

  /**
  * @brief     Get the type info(reshape or squeeze) of reshape layer
  * @return    The reshape type
  */
  const std::string &getType() { return _type; }

  /**
   * @brief     Get the shape info of reshape layer
   * @return    The shape info
   */
  const std::vector<int> &getShapeInfo() { return _shapeInfo; }

  /**
   * @brief     Get the reshape param of reshape layer
   * @return    The reshape param
   */
  const std::vector<int> &getReshapeParam() { return _reshapeParam; }

  /**
   * @brief     Print the reshape attribute
   */
  void printAttr()
  {
    LOG_AT(NN_MODEL, VERBOSE) << "    RType is     " << _type << std::endl;
    LOG_AT(NN_MODEL, VERBOSE) << "    RInfo are    " << _shapeInfo[0] << std::endl;
    for (size_t j = 1; j < _shapeInfo.size(); j++)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "                 " << _shapeInfo[j] << std::endl;
    }
  }

private:
  /** @brief the type info(reshape or squeeze) of reshape layer*/
  std::string _type;
  /** @brief the shape info(read from Neural Network op) of reshape layer*/
  std::vector<int> _shapeInfo;
  /** @brief the reshape param(the calculated output shape) of reshape layer*/
  std::vector<int> _reshapeParam;
};

/**
 * @brief   A class for NN reshape layer
 */
class NNReshapeLayer : public NNLayer
{
public:
  /**
   * @brief     Construction of NNReshapeLayer
   * @param     name The name of the layer
   * @param     type The type of the layer
   * @param     inputs The all the name of inputs to the layer
   * @param     outputs The all the name of outputs from the layer
   * @param     attr The attribute of the layer
   */
  NNReshapeLayer(std::string name, std::string type, std::vector<std::string> inputs,
                 std::vector<std::string> outputs, std::shared_ptr<NPUC::ReshapeAttr> attr)
      : NNLayer(name, type, inputs, outputs, attr)
  {
  }

  /**
   * @brief     Compute the reshape layer output shape
   * @param     inputShape  The input shape of reshape layer
   */
  void computeOutputShape(const std::vector<std::vector<int>> &inputShape)
  {
    std::shared_ptr<NPUC::ReshapeAttr> attr = std::static_pointer_cast<NPUC::ReshapeAttr>(_attr);
    std::string type = attr->getType();
    std::vector<int> shapeInfo = attr->getShapeInfo();

    if (type == "Squeeze")
    {
      std::vector<int> inShape = inputShape[0];
      std::vector<int> outShape = inShape;
      for (size_t i = 0; i < shapeInfo.size(); i++)
      {
        int dim = shapeInfo[i];
        if (outShape[dim] == 1)
        {
          outShape[dim] = 0;
        }
      }

      std::vector<int>::iterator it;
      for (it = outShape.begin(); it != outShape.end();)
      {
        if (*it == 0)
        {
          it = outShape.erase(it);
        }
        else
        {
          it++;
        }
      }

      _shapeMap[_outputs[0]] = outShape;
      attr->setReshapeParam(outShape);
    }
    else if (type == "Reshape")
    {
      std::vector<int> inShape = inputShape[0];
      std::vector<int> outShape = shapeInfo;
      int product = 1;
      for (auto i : inShape)
      {
        product *= i;
      }

      int subProduct = 1;
      int dim = -1;
      for (size_t i = 0; i < shapeInfo.size(); i++)
      {
        if (shapeInfo[i] == -1)
        {
          dim = i;
        }
        else
        {
          subProduct *= shapeInfo[i];
        }
      }

      if (dim == -1)
      {
        if (subProduct != product)
        {
          LOG_AT(NN_MODEL, VERBOSE) << "Can not Reshape! " << std::endl;
        }
      }
      else
      {
        if (product % subProduct != 0)
        {
          LOG_AT(NN_MODEL, VERBOSE) << "Can not Reshape! " << std::endl;
        }
        outShape[dim] = product / subProduct;
      }

      _shapeMap[_outputs[0]] = outShape;
      attr->setReshapeParam(shapeInfo);
    }
    else
    {
      LOG_AT(NN_MODEL, VERBOSE) << "Unknown Reshape Type!" << std::endl;
    }

    for (size_t i = 0; i < inputShape.size(); i++)
    {
      _shapeMap[_inputs[i]] = inputShape[i];
    }
  }
};

} // namespace NPUC
#endif // __NNRESHAPELAYER_H__
