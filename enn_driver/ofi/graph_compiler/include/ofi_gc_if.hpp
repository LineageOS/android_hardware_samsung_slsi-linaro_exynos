
#pragma once

#include <map>
#include <memory>
#include <string>

#include "ofi_gc_api.hpp"

namespace ofi {
namespace gc {

/**
* @brief Compiler parents class
*/
class GRAPH_COMPILER_API_CLASS(Compiler) {
  protected:
    class Impl;
    std::shared_ptr<Impl> _impl;

  public:
    /**
    * @brief Constructor.
    */
    Compiler();

    /**
    * @brief Destructor.
    */
    virtual ~Compiler();

    /**
    * @brief Sets configuration for plugin, acceptable keys can be found in ofi_gc_api.h file.
    * @param config Map of pairs: (config parameter name, config parameter value)
    * @param deviceName An optional name of device. If devie name i not specified, the config is set for all the registered devices.
    * @return ture if succeeded.
    *
    * The key declared as DECLARE_CONFIG_KEY() can be used.;
    */
    bool addConfig(const std::map<std::string, std::string> & config,
                   const std::string & deviceName = std::string());

    /**
    * @brief Select the Soc.
    * @param string for target.
    * @return ture if succeeded.
    */
    bool setSoC(const std::string& target);

    /**
    * @brief Set for output buffer. using output parameters.
    * @param buffer for cgo.
    * @param buffer for size.
    * @return ture if succeeded.
    */
    bool setOutput(char** cgoBuffer, uint32_t* size);

    /**
    * @brief Set mode for compiler, acceptable mask value can be found in the file.
    * @param masking mode ofr compiler.
    * @return ture if succeeded.
    */
    bool setMode(int mode);

    /**
    * @brief Get error status
    * @return enum statusCode:int.
    */
    int getErrorStatus() const;

    /**
    * @brief Get error message
    * @return string for error message.
    */
    std::string getErrorMessage() const;

    /**
    * @brief Process compile.
    * @return ture if succeeded.
    *
    * pure virtual function interface for child compiler
    */
    virtual bool process() = 0;
};


/**
* @brief Compiler class for CV
*/
class GRAPH_COMPILER_API_CLASS(SevaCompiler) : public Compiler {
  public:
    /**
    * @brief Constructor.
    */
    SevaCompiler();

    /**
    * @brief Destructor.
    */
    ~SevaCompiler();

    /**
    * @brief Set IR to graph.
    * @param into seva::graph
    * @return ture if succeeded.
    */
    bool setIr(const void* graph);

    /**
    * @brief Process compile.
    * @param set whether it is ofi or not.
    * @return ture if succeeded.
    */
    bool process() override;
};

/**
* @brief Compiler class for NN
*/
class GRAPH_COMPILER_API_CLASS(NnCompiler) : public Compiler {
    class NnImpl;
    std::shared_ptr<NnImpl> _nnImpl;
  public:
    /**
    * @brief Constructor.
    */
    NnCompiler();

    /**
    * @brief Destructor.
    */
    ~NnCompiler();

    /**
    * @brief Set IR to xml
    * @param path to xml
    * @return ture if succeeded.
    */
    bool setIr(const std::string& xml);

    /**
    * @brief Sets the weights and bias path
    * @param path to weight.
    * @param path to bias.
    * @return ture if succeeded.
    */
    bool setWeightsBias(const std::string& weightPath, const std::string& biasPath);

    /**
    * @brief Sets the weights and bias pointer and size
    * @param Blob for weight
    * @param Blob for bias
    * * @return ture if succeeded.
    */
    bool setWeightsBias(const void* weight, uint32_t wSize,
                        const void* bias, uint32_t bSize);

    /**
    * @brief Process compile.
    * @return ture if succeeded.
    */
    bool process() override;
};

/**
* @brief Compiler class for GraphGen
*/
class GRAPH_COMPILER_API_CLASS(GraphGenCompiler) : public Compiler {
    class GgImpl;
    std::shared_ptr<GgImpl> _ggImpl;
  public:
    /**
    * @brief Constructor.
    */
    GraphGenCompiler();

    /**
    * @brief Destructor.
    */
    ~GraphGenCompiler();

    /**
    * @brief Set IR to buffer.
    * @param buffer and buffer size.
    * @return ture if succeeded.
    */
    bool setIr(const char* garphIr);

    /**
    * @brief Set SharedMemoryInfo
    * @param buffer and buffer size.
    * @return ture if succeeded.
    */
    bool setSharedMemoryInfo(const char* memInfo, uint32_t infoSize);

    /**
    * @brief Process compile.
    * @return ture if succeeded.
    */
    bool process() override;
};
}  // namespace gc
}  // namespace ofi

