#ifndef __NNNETWORK_H__
#define __NNNETWORK_H__

#include <memory>
#include <vector>
#include <string>
#include <stack>
#include "NNLayer.h"
#include "ConvLayer.h"
#include "DeconvLayer.h"

namespace NPUC
{

/**
 * @brief A class for Neural Network network.
 */
class NNNetwork
{
public:
  /**
   * @brief  Default construction of NNNetwork.
   */
  NNNetwork() = default;

  ~NNNetwork();

  /**
   * @brief       Append layer into current network.
   * @param[in]   layer   The layer which will be appended.
   */
#ifdef OFFLINE_COMPILER
  int32_t appendLayers(std::shared_ptr<NPUC::NNLayer> nnLayer)
#else
  int32_t appendLayers(NPUC::NNLayer *nnLayer)
#endif
  {

#ifdef OFFLINE_COMPILER
    std::shared_ptr<NPUC::NNLayer> layer = nnLayer;
#else
    std::shared_ptr<NPUC::NNLayer> layer(nnLayer);
#endif

    for (const auto &l : _layers)
    {
      if (l->getName() == layer->getName())
      {
        LOG_AT(NN_MODEL, ERROR) << "Fail to add a layer because of duplicated layer: " << l->getName()
                                << std::endl;
        return FAIL_ON_NNMODEL;
      }
    }
    _layers.push_back(layer);
    _layerMap[layer->getName()] = layer;

    for (const auto &output : layer->getOutputs())
    {
      _outputMap[output] = layer;
    }
    return RET_OK;
  }
  /**
   * @brief       Return the layers in current network.
   * @return      The layers in the network.
   */
  const std::vector<std::shared_ptr<NPUC::NNLayer>> &layers() { return _layers; }

  /**
   * @brief       Set the weight shape information for the network
   * @param[in]   kernelShapeMap  The weight shape information, including layer name and shape.
   */
  void setWeightShape(const std::map<std::string, std::vector<int>> &kernelShapeMap)
  {
    _kernelShapeMap = kernelShapeMap;
    setKernelInfo();
  }

  /**
   * @brief       Set up the network.
   *              Including topology sort and some layer merging.
   */
  void setUpNetwork();

  /**
   * @brief       Manipulate the network for ondevice compiler
   *              for example, merge consecutive concats
   */
  void manipulateNetwork()
  {
    mergeConcatLayer();
  }

  /**
   * @brief       Remove all of the fake layers in network.
   */
  void removeFake();

  /**
   * @brief       Print the information of the network layer by layer.
   *              Including layer shape, input and output, parameters and so on.
   */
  void printNetwork()
  {
    LOG_AT(NN_MODEL, VERBOSE) << "Layer Cnt: " << _layers.size() << std::endl;
    for (size_t i = 0; i < _layers.size(); i++)
    {
      _layers[i]->printInfo();
    }    
  };  

private:
  /**
   * @brief       Do topology sort for the network.
   *              Try to find each layer's inputs and customers, and reorder the layers.
   * do topology for the whole network.
   * try to find each layer's inputs and customers.
   * reorder the layers according to the order in which they are executed in the network.
   */
  void topologySort();

  /**
   * @brief       Compute the output shape of network layer by layer.
   */
  void computeLayersShape();

  /**
   * @brief       Merge ignored layers, such as dropout and identity.
   */
  void mergeIgnoredLayer();

  /**
   * @brief      Remove a layer which has only one customer.
   * @param[in]  removeLayer The layer which will be removed.
   * remove the current layer
   * the layer must has only one customer.
   * Currently, this function only supports layer which has only one output.
   */
  void removeOneCustomerLayer(std::shared_ptr<NPUC::NNLayer> removeLayer)
  {
    assert(removeLayer->getOutputs().size() == 1);

    std::string oriName = removeLayer->getName();
    std::string newName = removeLayer->getConsumers()[0];
    // find the input idx in customer layer
    std::shared_ptr<NPUC::NNLayer> customerLayer = _outputMap[newName];
    int inputIdx = customerLayer->removeInput(removeLayer->getOutputs()[0]);

    // replace the input name of its customers
    std::vector<std::string> inputs = removeLayer->getInputs();
    int idx = inputIdx;
    for (const auto &name : inputs)
    {
      std::shared_ptr<NPUC::NNLayer> layer = _layerMap[name];
      layer->removeCustomer(oriName);
      layer->appendCustomer(newName);
      customerLayer->insertInput(name, layer->getShapeFromName(name), idx++);
    }
    // remove the layer related info
    removeLayerInfo(removeLayer);
  }

  /**
   * @brief      Remove the layer related information in network.
   * @param[in]  removeLayer The layer which will be removed.
   * remove the layer related info
   * including _layerMap, _outputMap and _layer
   */
  void removeLayerInfo(std::shared_ptr<NPUC::NNLayer> removeLayer)
  {
    // delete layer related
    // delete info in _layerMap
    std::map<std::string, std::shared_ptr<NPUC::NNLayer>>::iterator layerMapIt;
    for (layerMapIt = _layerMap.begin(); layerMapIt != _layerMap.end(); layerMapIt++)
    {
      if (layerMapIt->first == removeLayer->getName())
      {
        _layerMap.erase(layerMapIt);
        break;
      }
    }

    // delete info in _outputMap
    std::map<std::string, std::shared_ptr<NPUC::NNLayer>>::iterator outputMapIt;
    for (outputMapIt = _outputMap.begin(); outputMapIt != _outputMap.end(); outputMapIt++)
    {
      if (outputMapIt->first == removeLayer->getOutputs()[0])
      {
        _outputMap.erase(outputMapIt);
        break;
      }
    }

    // delete layer
    std::vector<std::shared_ptr<NPUC::NNLayer>>::iterator it;
    for (it = _layers.begin(); it != _layers.end(); it++)
    {
      if (*it == removeLayer)
      {
        _layers.erase(it);
        break;
      }
    }
  }

  /**
   * @brief      Merge concat layers if consecutive concat.
   * if a concat layer has only one customer and the customer another concat, the front layer will be
   * merged.
   */
  void mergeConcatLayer()
  {
    for (size_t idx = 0; idx < _layers.size(); idx++)
    {
      if (_layers[idx]->getType() == "Concat")
      {
        std::vector<std::string> customers = _layers[idx]->getConsumers();
        if (customers.size() == 1)
        {

          std::shared_ptr<NPUC::NNLayer> customerLayer = _layerMap[customers[0]];
          if (customerLayer->getType() == "Concat")
          {
            removeOneCustomerLayer(_layers[idx]);
            idx--;
          }
        }
      }
    }
  }

  /**
   * @brief      Remove a layer which has only one input.
   * @param[in]  removeLayer The layer which will be removed.
   * remove the current layer
   * the layer must has only one input.
   * Currently, this function only supports layer which has only one output.
   */
  void removeOneInputLayer(std::shared_ptr<NPUC::NNLayer> removeLayer)
  {
    assert(removeLayer->getOutputs().size() == 1);

    std::string oriName = removeLayer->getName();
    std::string newName = removeLayer->getInputs()[0];

    // find the previous layer
    std::shared_ptr<NPUC::NNLayer> preLayer = _outputMap[newName];
    preLayer->removeCustomer(oriName);

    // replace the input name of its customers
    std::vector<std::string> customers = removeLayer->getConsumers();
    for (const auto &name : customers)
    {
      std::shared_ptr<NPUC::NNLayer> layer = _layerMap[name];
      int idx = layer->removeInput(removeLayer->getOutputs()[0]);
      layer->insertInput(newName, removeLayer->getShapeFromName(newName), idx);
      preLayer->appendCustomer(name);
    }

    // remove the layer related info
    removeLayerInfo(removeLayer);
  }

  /**
   * @brief       Merge reshape layers if input and output shape are same.
   * if the input shape and output shape are the same, the reshape layer will be merged
   */
  void mergeReshapeLayer()
  {
    for (size_t idx = 0; idx < _layers.size(); idx++)
    {
      if (_layers[idx]->getType() == "Reshape")
      {
        std::vector<int> inShape = _layers[idx]->getInputShape(0);
        std::vector<int> outShape = _layers[idx]->getOutputShape(0);
        if (inShape == outShape)
        {
          removeOneInputLayer(_layers[idx]);
          idx--;
        }
      }
    }
  }

  /**
   * @brief      Get the input shape of a layer.
   *             Find layer inputs and give the input shape in Neural Network format(NHWC).
   * @param[in]  layer  The layer which shape is required.
   * @return     The input shape of the layer.
   */
  std::vector<std::vector<int>> getInputShape(std::shared_ptr<NPUC::NNLayer> layer);

  /**
   * @brief      Set the kernel shape information for network
   */
  void setKernelInfo();

  /**
   * @brief      Recursive function for the sort of more than one input layer.
   * @param[in/out] newSort The new order of the layers.
   * @param[in]  mergedLayer The layer which has more than one input.
   * This function recursively sorts the layers according to merged layers.(concat, eletwise, ...)
   * In the network, we call the layer which has more than one input as merged layer.
   * The previous layers should be sort according to the order of the inputs in merged layer.
   */
  void mergedSort(std::vector<std::shared_ptr<NPUC::NNLayer>> &newSort,
                  const std::shared_ptr<NPUC::NNLayer> &mergedLayer);

  /**
   * @brief      Do sorting when the layer has more than one input, such as concat and eletwise.
   * Sort the layers which has more than one input
   * For example: concat, eletwise ...
   *    A
   *   / \
   *   B D
   *   | |
   *   C E
   *   \ /
   *    F
   * the order of layers should be ABCDEF, not ADEBCF
   * currently, this function supports network which has only one output.
   */
  void mulPathsSort();

private:
  /** @brief layers in stf network.*/
  std::vector<std::shared_ptr<NPUC::NNLayer>> _layers;
  /** @brief a map combining the kernel name and kernel shape of all kernels in network.*/
  std::map<std::string, std::vector<int>> _kernelShapeMap;
  /** @brief a map combining layer name and its pointer.*/
  std::map<std::string, std::shared_ptr<NPUC::NNLayer>> _layerMap;
  /** @brief a map combining layer's output name and its layer pointer.*/
  std::map<std::string, std::shared_ptr<NPUC::NNLayer>> _outputMap;
};

} // namespace NPUC
#endif // __NNNETWORK_H__
