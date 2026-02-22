#ifndef __NNELTWISELAYER_H__
#define __NNELTWISELAYER_H__

#include "NNLayer.h"

namespace NPUC
{
/**
 * @brief   A class for eltwise layer attribute
 *          It has op type (Add, Maximum or ...) information
 */
class EltwiseAttr : public LayerAttr
{
public:
  /**
   * @brief   Default construction of EltwiseAttr
   */
  EltwiseAttr() {}

  /**
   * @brief     Set the op type of eltwise layer
   * @param     op    Op type of eltwise layer
   */
  void setOp(const std::string &op) { _op = op; }

  /**
   * @brief     Get the op type of eltwise layer
   * @return    Op type of eltwise layer
   */
  const std::string &getOp() { return _op; }

  /**
   * @brief     Print the eltwise attribute
   */
  void printAttr() { LOG_AT(NN_MODEL, VERBOSE) << "    Op is        " << _op << std::endl; }
private:
  /** the op type */
  std::string _op;
};

/**
 * @brief   A class for NN eltwise layer
 */
class NNEltwiseLayer : public NNLayer
{
public:
  /**
   * @brief     Construction of NNEltwiseLayer
   * @param     name The name of the layer
   * @param     type The type of the layer
   * @param     inputs The all the name of inputs to the layer
   * @param     outputs The all the name of outputs from the layer
   * @param     attr The attribute of the layer
   */
  NNEltwiseLayer(std::string name, std::string type, std::vector<std::string> inputs,
                 std::vector<std::string> outputs, std::shared_ptr<NPUC::EltwiseAttr> attr)
      : NNLayer(name, type, inputs, outputs, attr)
  {
  }

  /**
   * @brief     Compute the eltwise layer output shape
   * @param     inputShape  The input shape of eltwise layer
   */
  void computeOutputShape(const std::vector<std::vector<int>> &inputShape)
  {
    _shapeMap[_inputs[0]] = inputShape[0];

    for (size_t i = 1; i < inputShape.size(); i++)
    {
      assert(inputShape[0] == inputShape[i]);
      _shapeMap[_inputs[i]] = inputShape[i];
    }

    _shapeMap[_outputs[0]] = inputShape[0];
  }
};

} // namespace NPUC
#endif // __NNELTWISELAYER_H__
