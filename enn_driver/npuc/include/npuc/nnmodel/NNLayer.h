#ifndef __NNLayer_H__
#define __NNLayer_H__

#include <vector>
#include <string>
#include <map>
#include <cassert>
#include <algorithm>

#include "Logging.h"
#include "CompileOptions.h"

namespace NPUC
{

/**
 * @brief A base class of layer attribute
 */
class LayerAttr
{
public:
  /**
   * @brief A virtual function of printing attribute
   */
  virtual void printAttr(){};
  virtual ~LayerAttr() = default;
};

/**
 * @brief A base class of Neural Network layer
 */
class NNLayer
{
public:
  /**
   * @brief NNLayer constructor
   * @param name the name of the layer
   * @param type the type of the layer
   * @param inputs the all the name of inputs to the layer
   * @param outputs the all the name of outputs from the layer
   * @param attr the attribute of the layer
   */
  NNLayer(std::string name, std::string type, std::vector<std::string> inputs,
          std::vector<std::string> outputs, std::shared_ptr<NPUC::LayerAttr> attr)
      : _name(name), _type(type), _inputs(inputs), _outputs(outputs), _attr(attr)
  {
  }

  virtual ~NNLayer() = default;
  /**
   * @brief Gets the name of the layer
   * @return The name of the layer
   */
  const std::string &getName() { return _name; }
  /**
   * @brief Gets the type of the layer
   * @return The type of the layer
   */
  const std::string &getType() { return _type; }
  /**
   * @brief Gets the all the name of inputs to the layer
   * @return The list of all the inputs name
   */
  const std::vector<std::string> &getInputs() { return _inputs; }
  /**
   * @brief Gets the all the name of outputs from the layer
   * @return The list of outputs name
   */
  const std::vector<std::string> &getOutputs() { return _outputs; }
  /**
   * @brief Gets names of the consumers whose input name is this layer
   * @return The list of consumers name
   */
  const std::vector<std::string> &getConsumers() { return _consumers; }
  /**
   * @brief Gets the reshape given the name of output or input of this layer
   * @param name the name of output or input
   * @return The shape given the name
   */
  const std::vector<int> &getShapeFromName(std::string name) { return _shapeMap[name]; }
  /**
   * @brief Gets the attribute of the layer
   * @return the point to the attribute of the layer
   */
  std::shared_ptr<NPUC::LayerAttr> getAttr() { return _attr; }
  /**
   * @brief Adds the name of the consumer to the list of consumers
   * @param name the name of consumer need to add
   */
  void appendCustomer(const std::string &name) { _consumers.push_back(name); }
  /**
   * @brief Removes the name of the consumer from the list of consumers
   * @param name the name of consumer need to remove
   */
  void removeCustomer(const std::string &name)
  {
    std::vector<std::string>::iterator it;
    for (it = _consumers.begin(); it != _consumers.end(); it++)
    {
      if ((*it) == name)
      {
        _consumers.erase(it);
        break;
      }
    }
  }

  /**
   * @brief Inserts the shape of input given index
   * @param input the name of input to insert
   * @param shape the shape of input to insert
   * @param idx the index of inputs to the layer
   */
  void insertInput(const std::string &input, const std::vector<int> &shape, int idx)
  {
    std::vector<std::string>::iterator it = _inputs.begin() + idx;
    _inputs.insert(it, input);

    _shapeMap[input] = shape;
  }

  /**
   * @brief Inserts the shape of output
   * @param output the name of output to insert
   * @param shape the shape of output to insert
   */
  void insertOutputShape(const std::string &output, const std::vector<int> &shape)
  {
    _shapeMap[output] = shape;
  }

  /**
   * @brief Removes the input given its name
   * @param input the name of input to remove
   * @return 0 if no input given is found; otherwise, the original position of input
   */
  int removeInput(const std::string &input)
  {
    int idx = 0;
    std::vector<std::string>::iterator it;
    for (it = _inputs.begin(); it != _inputs.end(); it++, idx++)
    {
      if (*it == input)
      {
        _inputs.erase(it);
        break;
      }
    }

    if (_shapeMap.find(input) != _shapeMap.end())
    {
      std::vector<int> shape = _shapeMap[input];
      _shapeMap.erase(input);
    }

    return idx;
  }

  /**
   * @brief Gets the shape of input given index (NCHW format)
   * @param idx the index of inputs to the layer
   * @return Empty if no input given index is found; otherwise, the shape of input given index
   */
  std::vector<int> getInputShape(int idx)
  {
    std::string input = _inputs[idx];
    std::vector<int> tfShape = _shapeMap[input];
    std::vector<int> shape;

    assert(tfShape.size() == 2 || tfShape.size() == 4);

    if (tfShape.size() == 2)
    {
      shape.push_back(tfShape[0]);
      shape.push_back(tfShape[1]);
      shape.push_back(1);
      shape.push_back(1);
    }
    else
    {
      bool dataformat_NCHW = (CompileOptions::GetInstance()->getDataFormat() == DataFormat::NCHW);
      if (dataformat_NCHW)
      {
        shape.push_back(tfShape[0]);
        shape.push_back(tfShape[1]);
        shape.push_back(tfShape[2]);
        shape.push_back(tfShape[3]);
      }
      else
      {
        shape.push_back(tfShape[0]);
        shape.push_back(tfShape[3]);
        shape.push_back(tfShape[1]);
        shape.push_back(tfShape[2]);
      }
    }

    return shape;
  }

  /**
   * @brief Gets the shape of output given index (NCHW format)
   * @param idx the index of outputs from the layer
   * @return Empty if no output given index is found; otherwise, the shape of output given index
   */
  std::vector<int> getOutputShape(int idx)
  {
    std::string output = _outputs[idx];
    std::vector<int> tfShape = _shapeMap[output];
    std::vector<int> shape;

    assert(tfShape.size() == 2 || tfShape.size() == 4);

    if (tfShape.size() == 2)
    {
      shape.push_back(tfShape[0]);
      shape.push_back(tfShape[1]);
      shape.push_back(1);
      shape.push_back(1);
    }
    else
    {
      bool dataformat_NCHW = (CompileOptions::GetInstance()->getDataFormat() == DataFormat::NCHW);
      if (dataformat_NCHW)
      {
        shape.push_back(tfShape[0]);
        shape.push_back(tfShape[1]);
        shape.push_back(tfShape[2]);
        shape.push_back(tfShape[3]);
      }
      else
      {
        shape.push_back(tfShape[0]);
        shape.push_back(tfShape[3]);
        shape.push_back(tfShape[1]);
        shape.push_back(tfShape[2]);
      }
    }

    return shape;
  }

  void setInputShape(const std::vector<int> &inp, unsigned int idx)
  {
    const std::string &input = _inputs[idx];
    assert(inp.size() == _shapeMap[input].size());
    std::copy(inp.begin(), inp.end(), _shapeMap[input].begin());
  }

  void setOutputShape(const std::vector<int> &out, unsigned int idx)
  {
    const std::string &output = _outputs[idx];
    assert(out.size() == _shapeMap[output].size());
    std::copy(out.begin(), out.end(), _shapeMap[output].begin());
  }

  /**
   * @brief Print the information of the layer
   */
  void printInfo()
  {
    LOG_AT(NN_MODEL, VERBOSE) << _name << std::endl;
    LOG_AT(NN_MODEL, VERBOSE) << "{" << std::endl;
    LOG_AT(NN_MODEL, VERBOSE) << "    Type is      " << _type << std::endl;
    LOG_AT(NN_MODEL, VERBOSE) << "    Input size is      " << _inputs.size() << std::endl;
    if (_inputs.size() > 0)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "    Input are    " << std::endl;
      for (size_t j = 0; j < _inputs.size(); j++)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "                 " << _inputs[j] << std::endl;
      }
    }
    LOG_AT(NN_MODEL, VERBOSE) << "    Output size is      " << _outputs.size() << std::endl;
    if (_outputs.size() > 0)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "    Output are   " << std::endl;
      for (size_t j = 0; j < _outputs.size(); j++)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "                 " << _outputs[j] << std::endl;
      }
    }
    if (_inputs.size() > 0 && _shapeMap[_inputs[0]].size() > 0)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "    In shape are" << std::endl;
      for (size_t j = 0; j < _shapeMap[_inputs[0]].size(); j++)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "                 " << _shapeMap[_inputs[0]][j] << std::endl;
      }
    }
    if (_outputs.size() > 0 && _shapeMap[_outputs[0]].size() > 0)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "    Out shape are" << std::endl;
      for (size_t j = 0; j < _shapeMap[_outputs[0]].size(); j++)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "                 " << _shapeMap[_outputs[0]][j] << std::endl;
      }
    }
    if (_consumers.size() > 0)
    {
      LOG_AT(NN_MODEL, VERBOSE) << "    Consumers are" << _consumers.size() << std::endl;
      for (size_t j = 0; j < _consumers.size(); j++)
      {
        LOG_AT(NN_MODEL, VERBOSE) << "                 " << _consumers[j] << std::endl;
      }
    }
    _attr->printAttr();
    LOG_AT(NN_MODEL, VERBOSE) << "}" << std::endl;
  }

  /**
   * @brief Computes the shape of output with the shape of input
   * This fn can process the layer which has only one input, one output, and no shape changes.
   * @param inputShape the shape of input
   */
  virtual void computeOutputShape(const std::vector<std::vector<int>> &inputShape)
  {
    _shapeMap[_inputs[0]] = inputShape[0];
    _shapeMap[_outputs[0]] = inputShape[0];
  }

protected:
  /** @brief the name of layer*/
  std::string _name;
  /** @brief the type of layer */
  std::string _type;
  /** @brief the name of inputs to layer*/
  std::vector<std::string> _inputs;
  /** @brief the name of outputs from layer*/
  std::vector<std::string> _outputs;
  /** @brief the name of consumers of the layer*/
  std::vector<std::string> _consumers;
  /** @brief key: the name of input and output, value: the shape*/
  std::map<std::string, std::vector<int>> _shapeMap;
  /** @brief the attribute of layer*/
  std::shared_ptr<NPUC::LayerAttr> _attr = nullptr;
};

} // namespace NPUC
#endif // __NNLayer_H__
