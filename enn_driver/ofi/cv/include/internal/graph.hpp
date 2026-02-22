#ifndef __SEVA_GRAPH_HPP
#define __SEVA_GRAPH_HPP

#include <future>
#include <mutex>
#include <stack>
#include <utility>

#include "connection.hpp"
#include "executable.hpp"
#include "node.hpp"
#include "ofi.hpp"
#include "types.hpp"

/**
 * @file graph.hpp
 * @brief Graph class header file
 */

namespace seva {
/// Main namespace for Graph APIs.
namespace graph {
/**
 * @brief represents a graph obejct.
 *
 * Can be used to construct, manipulate, and compile a graph.
 */
class Graph final {
  public:
    /**
     * @brief represents the state of a graph.
     */
    enum class State {
        /*! \brief Unverified state. */
        UNVERIFIED,
        /*! \brief Verified state. */
        VERIFIED,
        /*! \brief Running state. */
        RUNNING,
        /*! \brief Complete state. */
        COMPLETE,
        /*! \brief Abandoned state. */
        ABANDONED,
    };

  public:
    /**
     * @brief Constructor.
     */
    Graph();
    /**
     * @brief Constructor.
     * @param [in] name the name of the graph.
     *
     * Initialize a graph with given name.
     */
    Graph(const char *name);
    /**
     * @brief Destructor.
     */
    ~Graph();

    /**
     * @brief Get the unique id.
     * @return the unique id of the graph.
     */
    unsigned int GetId() const;

    /**
     * @brief Create a node which has sub-graph.
     * @param [in] graphGenerateFunction subgraph generation function.
     * @return the shared pointer of the created node.
     */
    NodeHandle
    CreateNode(const std::function<
               bool(Graph &graph, const BufferList &inputs, const BufferList &outputs, const UserParamPtr &param)>
                   graphGenerateFunction);
    /**
     * @brief Create a node with given kernelName.
     * @param [in] kernelName  kernel name.
     * @return the shared pointer of the created node.
     */
    NodeHandle CreateNode(const char *kernelName);
    /**

     * @brief Delete a node.
     * @param [in] node NodeHandle of the node.
     */
    void DeleteNode(NodeHandle node);
    /**
     * @brief Compile a graph.
     * @return If it is success, it returns true. If not, it returns false.
     */
    bool Compile();
    /**
     * @brief Compile a graph.
     * @param [in] cgoPath cgo path for generating cgo
     * @return If it is success, it returns true. If not, it returns false.
     */
    bool Compile(const char *cgoPath);
    /**
     * @brief Compile a graph.
     * @param [in] cgoBuffer pointer to pointer of buffer allocated for compiled cgo.
     * @param [in] size pointer to integer allocated buffer size be stored.
     * @return If it is success, it returns true. If not, it returns false.
     */
    bool Compile(char **cgoBuffer, uint32_t *size);
    void ExportToDot(const char *fileName);
    /**
     * @cond
     * internal
     */
    /**
     * @brief Get the list of nodes.
     * @return the reference of the list of nodes.
     */
    const NodeList &GetNodes() const { return mNodes; }
    /**
     * @brief Get the number of inputs.
     * @return the number of inputs.
     */
    std::size_t GetNumOfInputs() const { return mNumOfInputs; }
    /**
     * @brief Get the number of outputs.
     * @return the number of outputs.
     */
    std::size_t GetNumOfOutputs() const { return mNumOfOutputs; }
    /**
     * @brief Get the number of outputs.
     * @return the number of outputs.
     */
    std::size_t GetSizeofUserParam() const { return mSizeUserParam; }
    /**
     * @endcond
     */
    /**
     * @brief Set the name of graph.
     * @param [in] name the name of the graph.
     */
    void SetName(const char *name) { mName = name; }
    /**
     * @brief Get the name of graph.
     * @return the name of the graph.
     */
    const char *GetName() const { return mName.c_str(); }
    /**
     * @brief Register a callback function.
     * @param [in] node the shared pointer of the node that you want to trigger the callback function.
     * @param [in] fp the function pointer of the callback.
     * @param [in] t the arguments of the callback.
     * @return If registering is success, it returns true. It not, it returns false.
     *
     * The callback function would be called after executing the node.
     */
    template <typename FuncPtr, typename T> bool RegisterCallback(NodeHandle node, FuncPtr fp, T t) {
        if (mExec != nullptr) {
            return (mExec->RegisterCallback(
                        mId, node->GetId(), reinterpret_cast<void *&>(fp), reinterpret_cast<void *>(&t)) == 0)
                       ? true
                       : false;
        }

        return false;
    }

    /**
     * @brief Enable intermediate buffer dump
     */
    void EnableDump() { mDumpEnable = true; }

    /**
     * @brief Disable intermediate buffer dump
     */
    void DisableDump() { mDumpEnable = false; }
    /**
     * @brief set temp buffer information between nodes.
     * @param [in] fd fd which is for an ion fd of the temp buffer.
     * @param [in] buffer the address of the temp buffer.
     * @param [in] size the size of the temp buffer.
     * @param [in] offset the offset of the temp buffer.
     */
    void SetTempBuffer(int fd, void *buffer, unsigned int size, unsigned int offset);
    /**
     * @brief set temp buffer information between nodes.
     * @param [in] buffer the address of the temp buffer.
     * @param [in] offset the offset of the temp buffer.
     */
    void SetTempBuffer(void *buffer, unsigned int size);
    /**
     * @brief set use or not intermediate temp buffer
     */
    void SetTempBufferByUser(bool flag);
    /**
     * @cond
     * internal
     */
    const std::vector<unsigned int> &GetTopologicalOrder() const { return mTopologicalOrder; }
    void ExportToDot(std::ofstream &dot);
    void AddNode(NodePtr node) { mNodes.emplace_back(std::move(node)); }
    bool TopologyVerify();
    bool ConnectNodes();
    bool CalculateOutputSize(void);
    bool IsDumpEnabled() const;
    int DumpTempBuffer(const char *);
    bool IsTempBufferAllocatedByUser() const;
    int GetTempBufferFd() const;
    void *GetTempBufferPtr() const;
    unsigned int GetTempBufferSize() const;
    unsigned int GetTempBufferOffset() const;
    system::MemoryInfo GetShapeInfoBuffer() { return mBufferInfo; }
    void SetShapeInfoBuffer(system::MemoryInfo memory) { mBufferInfo = memory; }
    int GetShapeBufferSize() const { return mShapeBufferSize; }
    void CalculateShapeBufferSize();
    /**
     * @endcond
     */

  private:
    /**
     * @cond
     * internal
     */
    friend class Node;
    friend class seva::system::Device;
    /**
     * @endcond
     */

#ifdef ENABLE_SEVA_TESTING
    FRIEND_TEST(Graph, DeleteMiddleNode);
    FRIEND_TEST(Graph, RunGraphSuccess);
    FRIEND_TEST(Graph, RunGraphFail);
    FRIEND_TEST(Graph, RunAsyncGraph);
    FRIEND_TEST(Connection, AddConnection);
    FRIEND_TEST(Graph, MultithreadAccess);
    FRIEND_TEST(Graph, RegisterCallback);
#endif

    Graph(system::Executable *exec);
    Graph(system::Executable *exec, const char *name);
    bool CheckChangesOfIO();
    void ResetChangesOfIO();
    void SetState(State state);
    State GetState() const;
    bool TopologySort();
    bool IsVerified() const;
    template <typename T> int GetDistance(const std::vector<T> &elements, const T &element) {
        int distance = -1;

        auto it = std::find(elements.begin(), elements.end(), element);
        if (it != elements.end()) {
            distance = (int)std::distance(elements.begin(), it);
        }

        return distance;
    }

    int GetNodeDistance(const Node *node);
    void ResetVerifiedInformation();
    void Connect(const PortInfo &source, const PortInfo &destination);
    void Disconnect(Connection *connection);
    const ConnectionList &GetConnections() const { return mConnections; }

    void DrawGraph(std::ofstream &dot, Graph *graph, int depth);
    void DrawNodes(std::ofstream &dot, Graph *graph, int depth);
    void DrawConnections(std::ofstream &dot, Graph *graph, int depth);

    void SetPermissionToAccessVirtualObject(bool access);
    void CalculateExternalIO();
    bool ValidateNodes();
    bool FlatteningSubGraph();

    struct ElemDeleter final {
        void operator()(Node *node) const;
        void operator()(Connection *connection) const;
    };

    void SetDumpDone() { mDumpDone = true; }
    void ClearDumpDone() { mDumpDone = false; }
    bool GetDumpDone() { return mDumpDone; }
    void SetRemoveTempBuffer() { mRemoveTempBuffer = true; }
    bool GetRemoveTempBuffer() { return mRemoveTempBuffer; }
    void SetShapeBufferOffset(unsigned int offset) { mShapeBufferOffset = offset; }
    unsigned int GetShapeBufferOffset() { return mShapeBufferOffset; }
    void SetModelId(int modelId) { mModelId = modelId; }
    int GetModelId() { return mModelId; }
    void SetCgoPath(const char *cgoPath) { mCgoPath = std::string(cgoPath); }
    const std::string &GetCgoPath() { return mCgoPath; }
    const char* GetCgoBuffer() { return mCgoBuffer; }
    int GetCgoSize() { return (int)mCgoSize; }

  private:
    unsigned int mId;
    static unsigned int sCurrentId;

    State mState;
    NodeList mNodes;
    system::Executable *mExec;
    ConnectionList mConnections;
    std::string mName;
    std::size_t mNumOfInputs;
    std::size_t mNumOfOutputs;
    std::size_t mSizeUserParam;
    static const char *TAG;
    std::vector<unsigned int> mTopologicalOrder;
    std::map<int, std::future<bool>> mExecutes;
    mutable std::recursive_mutex mRecursiveMutex;
    int mModelId;
    bool mDumpEnable;
    bool mDumpDone;
    bool mRemoveTempBuffer;
    bool mIsTempBufferAllocatedByUser;
    int mTempBufferFd;
    void *mTempBufferPtr;
    unsigned int mTempBufferSize;
    unsigned int mTempBufferOffset;
    system::MemoryInfo mBufferInfo;
    int mShapeBufferSize;
    unsigned int mShapeBufferOffset;
    std::string mCgoPath;
    char *mCgoBuffer;
    uint32_t mCgoSize;
};
}  // namespace graph
}  // namespace seva
#endif
