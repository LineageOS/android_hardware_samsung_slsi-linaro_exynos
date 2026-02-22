#ifndef __SEVA_NODE_HPP
#define __SEVA_NODE_HPP

#include "buffer.hpp"
#include "types.hpp"

#ifdef ENABLE_SEVA_TESTING
#include "gtest/gtest_prod.h"
#endif

#include <algorithm>
#include <cstdarg>
#include <map>
#include <mutex>
#include <string>

#define SEVA_MAX_INPUT_OUTPUT_COUNT 32

/**
 * @file node.hpp
 * @brief Node class header file
 */

namespace seva {
namespace graph {
class UserParam;
class Connection;
class Graph;

/**
 * @brief represents a node obejct.
 *
 * Can be used to set a computation such as kernel or graph, inputs, outputs,
 * and user params into a node.
 * Also can be used to set the attributes of the buffers.
 */
class Node final : public std::enable_shared_from_this<Node> {
  public:
    using ConnectionSet = std::map<std::size_t, Connection *>;

    /**
     * @brief represents the type of the computation of a node.
     */
    enum class Type {
        /*! \brief Graph type. */
        GRAPH = 0,
        /*! \brief Kernel type. */
        KERNEL = 1,
        /*! \brief EDGE type. */
        EDGE,
    };

    /**
     * @brief represents an node information
     *
     * Can be used to set/get a information
     * such as kernel or graph, inputs, outputs, and user params into a node.
     */
    class Info final {
      public:
        /**
         * @brief Set the type such as Graph or Kernel.
         * @param [in] type the type of the computation.
         */
        void SetType(const Type type) { mType = type; }

        Type GetType() const { return mType; }
        /**
         * @brief Return true, when Kernel is set into a node.
         */
        bool IsKernel() const { return mType == Node::Type::KERNEL ? true : false; }
        /**
         * @brief Return true, when Graph is set into a node.
         */
        bool IsGraph() const { return mType == Node::Type::GRAPH ? true : false; }
        /**
         * @brief Get the id of Graph or Kernel.
         */
        int GetId() const { return mId; }
        /**
         * @brief Set the id of Graph.
         * @param [in] id the unique id of the graph.
         */
        void SetId(int id) { mId = id; }
        /**
         * @brief Set the graph.
         * @param [in] graph the object pointer of the graph.
         */
        void SetGraph(const Graph *graph);
        /**
         * @brief Set the name.
         * @param [in] the name of computation.
         */
        void SetName(const char *name);
        /**
         * @brief Get the object's pointer of Graph or Kernel.
         */
        void *GetObject() const { return mObject; }
        /**
         * @brief Get the name of Graph or Kernel.
         * @return the name of the graph or kernel.
         */
        const char *GetName() const;
        /**
         * @brief Set the number of inports.
         * @param [in] inPorts the number of inports.
         */
        void SetNumOfInPorts(std::size_t inPorts) { mNumOfInPorts = inPorts; }
        /**
         * @brief Get the number of inports.
         * @return the number of inports.
         */
        std::size_t GetNumOfInPorts() const { return mNumOfInPorts; }
        /**
         * @brief Get the number of minimum inports.
         * @return the number of minimum inports.
         */
        std::size_t GetNumOfMinimumInPorts() const;
        /**
         * @brief Set the number of outports.
         * @param [in] outPorts the number of outports.
         */
        void SetNumOfOutPorts(std::size_t outPorts) { mNumOfOutPorts = outPorts; }
        /**
         * @brief Get the number of outports.
         * @return the number of outports.
         */
        std::size_t GetNumOfOutPorts() const { return mNumOfOutPorts; }
        /**
         * @brief Get the number of minimum outports.
         * @return the number of minimum outports.
         */
        std::size_t GetNumOfMinimumOutPorts() const;
        /**
         * @brief Set the number of user parameters.
         * @param [in] userParams the number of user parameters.
         */
        void SetSizeOfUserParam(std::size_t size) { mSizeOfUserParam = size; }
        /**
         * @brief Get the number of user parameters.
         * @return the number of user parameters.
         */
        std::size_t GetSizeOfUserParam() const { return mSizeOfUserParam; }

        /**
         * @brief Constructor.
         */
        Info();
        /**
         * @brief Destructor.
         */
        ~Info();

        bool IsMandatoryInputPort(std::size_t port);
        bool IsMandatoryOutputPort(std::size_t port);
        std::vector<int> GetInputFlags() { return mInputFlags; }
        void SetInputFlags(const std::vector<int> &flags) { mInputFlags = flags; }
        std::vector<int> GetOutputFlags() { return mOutputFlags; }
        void SetOutputFlags(const std::vector<int> &flags) { mOutputFlags = flags; }

      private:
        Type mType;
        int mId;
        std::string mName;
        void *mObject;
        std::size_t mNumOfInPorts;
        std::size_t mNumOfOutPorts;
        std::size_t mSizeOfUserParam;
        std::vector<int> mInputFlags;
        std::vector<int> mOutputFlags;
    };

  public:
    /**
     * @brief Get the unique id of the node.
     * @return the unique id of the node.
     */
    unsigned int GetId() { return mId; }
    /**
     * @brief Get the information of the node.
     * @return the information of the node.
     */
    const Info &GetInfo() const { return mInfo; }
    /**
     * @brief Set the affinity of the node.
     * @param [in] target the target you want to do a computation on.
     *
     * It is a hint.
     * So the computation into the node could be executed other processors.
     */
    void SetAffinity(system::Target target) { mAffinity = target; }
    /**
     * @brief Get the affinity of the node.
     * @return the target.
     */
    system::Target GetAffinity() const { return mAffinity; }

    /**
     * @brief Set the input/output buffers and user parameters.
     * @return If the setting succeeds, it returns true. If not, it return false.
     *
     * The order to input the buffers and user parameters should be the same as the signature of the kernel/graph.
     */
    template <typename T, typename... Rest> bool Set(T &&t, Rest &&... rest) {
        std::lock_guard<std::recursive_mutex> lock(mRecursiveMutex);
        return SetArgument(0, t) && SetArgument(1, rest...);
    }

    /**
     * @cond
     * internal
     */
    const BufferList &GetInputBuffers() const { return mInputBuffers; }
    const BufferList &GetOutputBuffers() const { return mOutputBuffers; }
    const UserParamPtr &GetUserParam() const { return mUserParam; }
    const ConnectionSet &GetInConnections() const { return mInConnections; }
    const ConnectionSet &GetOutConnections() const { return mOutConnections; }
    Node(Graph *graph, const char *kernelName);
    ~Node();
    void DeleteThisNode(void);
    const std::vector<int> &GetInputPortFlags() const { return mInputPortFlag; }
    const std::vector<int> &GetOutputPortFlags() const { return mOutputPortFlag; }
    void SetOrder(unsigned int order) { mOrder = order; }
    unsigned int GetOrder() { return mOrder; }
    int WriteTempOutput(int portid, const char *fileName);
    // int WriteInput(int portid, const char* fileName);
    /**
     * @endcond
     */

  private:
    /**
     * @cond
     * internal
     */
    friend class Graph;
    friend class Connection;
    friend class RingBuffer;
#ifdef ENABLE_OFI_FRAMEWORK
#else
    friend class SimulationEngine;
#endif

    /**
     * @endcond
     */

#ifdef ENABLE_SEVA_TESTING
    FRIEND_TEST(Node, ValidateTwoConnectionOnePort);
    FRIEND_TEST(Node, SetBuffers);
    FRIEND_TEST(Node, LinkTwoNodes);
    FRIEND_TEST(Node, AddUserParam);
    FRIEND_TEST(Node, AddUserParam2);
    FRIEND_TEST(Node, AddUserParam3);
    FRIEND_TEST(Node, AddUserParam4);
    FRIEND_TEST(Graph, DeleteMiddleNode);
    FRIEND_TEST(RingBuffer, AgingWithNode);
#endif

    Node() = delete;
    Node(Graph *graph,
         const std::function<
             bool(Graph &graph, const BufferList &inputs, const BufferList &outputs, const UserParamPtr &param)>
             &graphGenerateFunction);
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;

    NodeHandle GetHandle() { return NodeHandle(shared_from_this()); }

    void SetGraph(Graph *graph);
    void AddInConnection(Connection *connection);
    void AddOutConnection(Connection *connection);
    void RemoveInConnection(Connection *connection);
    void RemoveOutConnection(Connection *connection);

    void ResetExternalIO();
    void FindExternalIO();
    bool Validate();

    bool SetArgument(std::size_t port, BufferPtr &t) {
        system::Log::V(TAG, "%s()", __FUNCTION__);

        std::size_t inPorts = mInfo.GetNumOfInPorts();
        std::size_t outPorts = mInfo.GetNumOfOutPorts();

        if (port >= inPorts + outPorts) {
            system::Log::E(TAG,
                           "%s(): node(%s) have exceeded the maximum number of inputs you can receive.",
                           __FUNCTION__,
                           mInfo.GetName());
            return false;
        }

        if (port < inPorts) {
            return AddInputBuffer(port, t);
        } else if (port < inPorts + outPorts) {
            return AddOutputBuffer(port - inPorts, t);
        }

        system::Log::E(TAG, "%s(): un-reachable here", __FUNCTION__);
        return false;
    }

    template <typename T> bool SetArgument(std::size_t port, T &t) {
        std::size_t inPorts = mInfo.GetNumOfInPorts();
        std::size_t outPorts = mInfo.GetNumOfOutPorts();

        if (port < inPorts + outPorts) {
            return SetArgument(port, (BufferPtr &)t);
        }

        return AddUserParam(port, (UserParamPtr &)t);
    }

    template <typename T, typename... Rest> bool SetArgument(std::size_t port, T t, Rest &... rest) {
        return SetArgument(port, t) && SetArgument(port + 1, rest...);
    }

    bool AddInputBuffer(const std::size_t &port, const BufferPtr &input) {
        system::Log::V(TAG, "%s(): port(%d)", __FUNCTION__, port);

        if ((mInfo.IsMandatoryInputPort(port) == true) && input == nullptr) {
            system::Log::E(TAG, "%s(): Port(%d) should be filled", __FUNCTION__, port);
            return false;
        }

        if (port >= mInfo.GetNumOfInPorts()) {
            system::Log::E(TAG, "%s(): Port(%d) is too large for node(%s)", __FUNCTION__, port, GetInfo().GetName());
            return false;
        }

        if (mInputBuffers.find(port) != mInputBuffers.end()) {
            system::Log::E(
                TAG, "%s(): Port(%d) on node(%s) already has an input buffer", __FUNCTION__, port, GetInfo().GetName());
            return false;
        }

        if (input != nullptr) {
            mInputBuffers.emplace(port, input);
            mInputPortFlag.emplace_back(1);
        } else {
            mInputPortFlag.emplace_back(0);
        }

#if 0
        if (mGraph) {
            mGraph->SetState(Graph::State::UNVERIFIED);
        }
#endif

        return true;
    }

    bool AddOutputBuffer(const std::size_t &port, const BufferPtr &output) {
        system::Log::V(TAG, "%s(): port(%d)", __FUNCTION__, port);

        if ((mInfo.IsMandatoryOutputPort(port) == true) && output == nullptr) {
            system::Log::E(TAG, "%s(): Port(%d) should be filled", __FUNCTION__, port);
            return false;
        }

        if (port >= mInfo.GetNumOfOutPorts()) {
            system::Log::E(TAG, "%s(): Port(%d) is too large for node(%s)", __FUNCTION__, port, GetInfo().GetName());
            return false;
        }

        if (mOutputBuffers.find(port) != mOutputBuffers.end()) {
            system::Log::E(TAG,
                           "%s(): Port(%d) on node(%s) already has an output buffer",
                           __FUNCTION__,
                           port,
                           GetInfo().GetName());
            return false;
        }

        if (output != nullptr) {
            mOutputBuffers.emplace(port, output);
            mOutputPortFlag.emplace_back(1);
        } else {
            mOutputPortFlag.emplace_back(0);
        }

#if 0
        if(mGraph) {
            mGraph->SetState(Graph::State::UNVERIFIED);
        }
#endif

        return true;
    }

    void RemoveInputBuffer(const BufferPtr &input);
    void RemoveOutputBuffer(const BufferPtr &output);

    void UpdateInputBuffer(const BufferPtr &input, uint16_t idx);
    void UpdateOutputBuffer(const BufferPtr &output, uint16_t idx);

    bool AddUserParam(const std::size_t &port, const UserParamPtr &param) {
        system::Log::V(TAG, "%s()", __FUNCTION__);
        if (mInfo.GetSizeOfUserParam() == 0) {
            system::Log::E(TAG, "%s(): node(%s) does not have an user parameter", __FUNCTION__, mInfo.GetName());
            return false;
        }

        std::size_t inPorts = mInfo.GetNumOfInPorts();
        std::size_t outPorts = mInfo.GetNumOfOutPorts();
        if (port > inPorts + outPorts) {
            system::Log::E(TAG,
                           "%s(): node(%s) have exceeded the maximum number of inputs you can receive.",
                           __FUNCTION__,
                           mInfo.GetName());
            system::Log::E(TAG, "port: %d inPorts: %d outPorts: %d\n", port, inPorts, outPorts);
            return false;
        }

        if (mInfo.GetSizeOfUserParam() != param->GetSize()) {
            system::Log::E(TAG,
                           "%s(): node(%s) invalid param size expected(%d) and your param size(%d).",
                           __FUNCTION__,
                           mInfo.GetName(),
                           mInfo.GetSizeOfUserParam(),
                           param->GetSize());
            return false;
        }

        mUserParam = param;
        return true;
    }

    bool RemoveUserParam(const UserParamPtr &param);

    bool SetSignature(std::size_t numInputs, std::size_t numOutputs, std::size_t size);

  private:
    unsigned int mId;
    unsigned int mOrder;
    static unsigned int sCurrentId;

    Graph *mGraph;
    Info mInfo;

    mutable std::recursive_mutex mRecursiveMutex;
    BufferList mInputBuffers;
    BufferList mOutputBuffers;
    UserParamPtr mUserParam;
    ConnectionSet mInConnections;
    ConnectionSet mOutConnections;
    system::Target mAffinity;
    std::function<bool(Graph &graph, const BufferList &inputs, const BufferList &outputs, const UserParamPtr &param)>
        mGraphGeneratedFunction;
    std::vector<int> mInputPortFlag;
    std::vector<int> mOutputPortFlag;
    static const char *TAG;
};
}  // namespace graph
}  // namespace seva
#endif
