
#pragma once

namespace ofi {
namespace gc {

#define GRAPH_COMPILER_API(type)        __attribute__((visibility("default"))) type
#define GRAPH_COMPILER_API_CLASS(type)  __attribute__((visibility("default"))) type

/**
* @brief shortcut for defining configuration keys
*/
#define _CONFIG_KEY(name) KEY_##name
#define DECLARE_CONFIG_KEY(name) static constexpr auto _CONFIG_KEY(name) = #name

/**
* @brief shortcut for defining configuration values
*/
#define DECLARE_CONFIG_VALUE(name) static constexpr auto name = #name

/**
* @brief generic boolean values
*/
DECLARE_CONFIG_VALUE(YES);
DECLARE_CONFIG_VALUE(NO);

/**
* @brief Limit #threads that are used by Inference Engine for inference on the CPU.
*/
DECLARE_CONFIG_KEY(CPU_THREADS_NUM);

/**
* @brief The name for setting CPU affinity per thread option.
* It is passed to IInferencePlugin::SetConfig(), this option should be used with values:
* PluginConfigParams::YES or PluginConfigParams::NO
* Ignored, if the OpenVINO compiled with OpenMP threading and any affinity-related OpenMP's
* environment variable is set
*/
DECLARE_CONFIG_KEY(CPU_BIND_THREAD);

/**
* @brief Optimize CPU execution to maximize throughput.
* It is passed to IInferencePlugin::SetConfig(), this option should be used with values:
* - KEY_CPU_THROUGHPUT_NUMA creates as many streams as needed to accomodate NUMA and avoid associated penalties
* - KEY_CPU_THROUGHPUT_AUTO creates bare minimum of streams to improve the performance,
*   this is the most portable option if you have no insights into how many cores you target machine will have
*   (and what is the optimal number of streams)
* - finally, specifying the positive integer value creates the requested number of streams
*/
DECLARE_CONFIG_VALUE(CPU_THROUGHPUT_NUMA);
DECLARE_CONFIG_VALUE(CPU_THROUGHPUT_AUTO);
DECLARE_CONFIG_KEY(CPU_THROUGHPUT_STREAMS);

/**
* @brief Optimize GPU plugin execution to maximize throughput.
* It is passed to IInferencePlugin::SetConfig(), this option should be used with values:
* - KEY_GPU_THROUGHPUT_AUTO creates bare minimum of streams that might improve performance in some cases,
*   this option allows to enable throttle hint for opencl queue thus reduce CPU load without significant performance drop
* - a positive integer value creates the requested number of streams
*/
DECLARE_CONFIG_VALUE(GPU_THROUGHPUT_AUTO);
DECLARE_CONFIG_KEY(GPU_THROUGHPUT_STREAMS);


/**
* @brief The name for setting performance counters option.
* It is passed to IInferencePlugin::SetConfig(), this option should be used with values:
* PluginConfigParams::YES or PluginConfigParams::NO
*/
DECLARE_CONFIG_KEY(PERF_COUNT);

/**
* @brief The key defines dynamic limit of batch processing.
* Specified value is applied to all following Infer() calls. Inference Engine processes
* min(batch_limit, original_batch_size) first pictures from input blob. For example, if input
* blob has sizes 32x3x224x224 after applying plugin.SetConfig({KEY_DYN_BATCH_LIMIT, 10})
* Inference Engine primitives processes only beginner subblobs with size 10x3x224x224.
* This value can be changed before any Infer() call to specify a new batch limit.
*
* The paired parameter value should be convertible to integer number. Acceptable values:
* -1 - Do not limit batch processing
* >0 - Direct value of limit. Batch size to process is min(new batch_limit, original_batch)
*/
DECLARE_CONFIG_KEY(DYN_BATCH_LIMIT);

DECLARE_CONFIG_KEY(DYN_BATCH_ENABLED);

/**
* @brief The key controls threading inside Inference Engine.
* It is passed to IInferencePlugin::SetConfig(), this option should be used with values:
* PluginConfigParams::YES or PluginConfigParams::NO
*/
DECLARE_CONFIG_KEY(SINGLE_THREAD);

/**
* @brief This key directs the plugin to load a configuration file.
* The value should be a file name with the plugin specific configuration
*/
DECLARE_CONFIG_KEY(CONFIG_FILE);

/**
* @brief This key enables dumping of the kernels used by the plugin for custom layers.
* This option should be used with values: PluginConfigParams::YES or PluginConfigParams::NO (default)
*/
DECLARE_CONFIG_KEY(DUMP_KERNELS);

/**
* @brief This key controls performance tuning done or used by the plugin.
* This option should be used with values: PluginConfigParams::TUNING_CREATE,
* PluginConfigParams::TUNING_USE_EXISTING or PluginConfigParams::TUNING_DISABLED (default)
*/
DECLARE_CONFIG_KEY(TUNING_MODE);


DECLARE_CONFIG_VALUE(TUNING_CREATE);
DECLARE_CONFIG_VALUE(TUNING_USE_EXISTING);
DECLARE_CONFIG_VALUE(TUNING_DISABLED);

/**
* @brief This key defines the tuning data filename to be created/used
*/
DECLARE_CONFIG_KEY(TUNING_FILE);

/**
* @brief the key for setting desirable log level.
* This option should be used with values: PluginConfigParams::LOG_NONE (default),
* PluginConfigParams::LOG_WARNING, PluginConfigParams::LOG_INFO, PluginConfigParams::LOG_DEBUG
*/
DECLARE_CONFIG_KEY(LOG_LEVEL);

DECLARE_CONFIG_VALUE(LOG_NONE);
DECLARE_CONFIG_VALUE(LOG_WARNING);
DECLARE_CONFIG_VALUE(LOG_INFO);
DECLARE_CONFIG_VALUE(LOG_DEBUG);

/**
* @brief the key for setting of required device to execute on
* values: device id starts from "0" - first device, "1" - second device, etc
*/
DECLARE_CONFIG_KEY(DEVICE_ID);

/**
* @brief the key for enabling exclusive mode for async requests of different executable networks and the same plugin.
* Sometimes it's necessary to avoid oversubscription requests that are sharing the same device in parallel.
* E.g. There 2 task executors for CPU device: one - in the Hetero plugin, another - in pure CPU plugin.
* Parallel execution both of them might lead to oversubscription and not optimal CPU usage. More efficient
* to run the corresponding tasks one by one via single executor.
* By default, the option is set to YES for hetero cases, and to NO for conventional (single-plugin) cases
* Notice that setting YES disables the CPU streams feature (see another config key in this file)
*/
DECLARE_CONFIG_KEY(EXCLUSIVE_ASYNC_REQUESTS);

/**
 * @brief This key enables dumping of the internal primitive graph.
 * Should be passed into LoadNetwork method to enable dumping of internal graph of primitives and
 * corresponding configuration information. Value is a name of output dot file without extension.
 * Files <dot_file_name>_init.dot and <dot_file_name>_perf.dot will be produced.
 */
DECLARE_CONFIG_KEY(DUMP_EXEC_GRAPH_AS_DOT);

/**
* @brief This key lists device names available to the plugin.
* The value should be a bracketed string format. e.g. {CPU,GPU,DSP}.
*/
DECLARE_CONFIG_KEY(DEVICE_LIST);

/**
* @brief This key means test vector path for NN
*/
DECLARE_CONFIG_KEY(TESTVECTOR_PATH);

/**
* @brief This key means test vector finename style.
*/
DECLARE_CONFIG_KEY(TV_FILENAME_STYLE);

/**
* @brief This key means weight bin path for NN
*/
DECLARE_CONFIG_KEY(WEIGHT_PATH);

/**
* @brief This key means bias bin path for NN
*/
DECLARE_CONFIG_KEY(BIAS_PATH);

/**
* @brief This key means graph output configuration for NN
*/
DECLARE_CONFIG_KEY(GRAPH_OUT_CONF);

/**
* @brief This key means spec of dsp target
*/
DECLARE_CONFIG_KEY(DSP_TARGET);

/**
* @brief This key means enabling dummy kernel mode
*/
DECLARE_CONFIG_KEY(DUMMY_KERNEL);

/**
* @brief This key means temp buffer allocated by user or not
*/
DECLARE_CONFIG_KEY(TEMP_BUFFER_ALLOCATED_BY_USER);

/**
* @brief This key means enabling multi buffer shape mode
*/
DECLARE_CONFIG_KEY(MULTI_SHAPE_INFOS);

/**
* @brief This key means path that output files stored
*/
DECLARE_CONFIG_KEY(OUTPUT_PATH);

/**
* @brief This key means enabling weight channel swap for first layer
*/
DECLARE_CONFIG_KEY(WEIGHT_CHANNEL_SWAP);

/**
* @brief This key means that flattened dimension is allowed for DSP performance.
*/
DECLARE_CONFIG_KEY(ALLOW_FLATTEN);

/**
* @brief This key means file name of compiled graph output.
*/
DECLARE_CONFIG_KEY(CGO_FILENAME);

/**
* @brief This key means name of model for model specific optimization.
*/
DECLARE_CONFIG_KEY(OPTIMIZE_MODEL_NAME);

/**
* @brief This key means bit width of model for mapping kernel.
*/
DECLARE_CONFIG_KEY(BIT_WIDTH);

/**
* @brief This key means tilling with optimized mode even more time needed.
*/
DECLARE_CONFIG_KEY(OPT_TILING);

/**
* @brief This key means enabling kernel chaining.
*/
DECLARE_CONFIG_KEY(KERNEL_CHAINING);

/**
* @brief This key means type of model (unified or not)
*/
DECLARE_CONFIG_KEY(MODEL_TYPE);

/**
* @brief This key means type of sdk (unified or not)
*/
DECLARE_CONFIG_KEY(SDK_TYPE);

/**
* @enum StatusCode
* @brief This enum contains error return value
*/
enum StatusCode : int{
    OK = 0,
    GENERAL_ERROR = -1,
    NOT_IMPLEMENTED = -2,
    NETWORK_NOTE_LOADED = -3,
    PARAMETER_MISMATCH = -4,
    NOT_FOUND = -5,
    OUT_OF_BOUNDS = -6,
    /*
     * @brief exception not of std::exception derived type was thrown
     */
    UNEXPECTED = -7,
    REQUEST_BUSY = -8,
    RESULT_NOT_READY = -9,
    NOT_ALLOCATED = -10,
    INFER_NOT_STARTED = -11,
    NETWORK_NOT_READ = -12,

    INVALID_PARAMETER = -20,
    UNSET_VALUE_EXISTS = -21
};

/**
* @brief This value for compiler mode masking.
*/
GRAPH_COMPILER_API(extern const int COMPILER_MODE_FULL);
GRAPH_COMPILER_API(extern const int COMPILER_MODE_MEM);
GRAPH_COMPILER_API(extern const int COMPILER_MODE_FILE);
GRAPH_COMPILER_API(extern const int COMPILER_MODE_FAST);
GRAPH_COMPILER_API(extern const int COMPILER_MODE_CACHE);
GRAPH_COMPILER_API(extern const int COMPILER_MODE_VERIFICATION);
GRAPH_COMPILER_API(extern const int COMPILER_MODE_SKIP_OPTIMIZE_PASS);
GRAPH_COMPILER_API(extern const int COMPILER_MODE_REPLACE_DUMMY_KERNEL);

}  // namespace gc
}  // namespace ofi

