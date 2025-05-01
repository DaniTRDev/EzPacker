#ifndef EZPACKER_CODEFLOWGRAPH_H
#define EZPACKER_CODEFLOWGRAPH_H

#include "EzLifterCommon.h"

enum class CodeFlowBlockType : uint8_t
{
    Invalid = 0,
    Branch, // Conditional Jumps.
    Call,   // Call
    Jump,   // Unconditional Jumps.
    Return, // Block is a return block.
    Syscall // Block is a syscall. Each syscall depends on the OS.
};

inline std::map<CodeFlowBlockType, const char *> CodeFlowBlockType2Str = {
    {CodeFlowBlockType::Invalid, "Invalid"}, {CodeFlowBlockType::Branch, "Branch"},
    {CodeFlowBlockType::Call, "Call"},       {CodeFlowBlockType::Jump, "Jump"},
    {CodeFlowBlockType::Return, "Return"},   {CodeFlowBlockType::Syscall, "Syscall"}};

/**
 * Simple structure that contains information about a block (vertice) of our Graph. FULL-CONNECTED GRAPH!
 */
struct CodeFlowBlock
{
    CodeFlowBlockType m_type;
    uint64_t m_address;
    std::unordered_set<uint64_t> m_children;
};

/**
 * This class represents an object that will expose friendly methods to interact with the Code Flow Graph. It'll be of
 * help when trying to know where does a block jumps to.
 */
class CodeFlowGraph
{
  public:
    /**
     * Constructs the object with default values.
     */
    CodeFlowGraph();
    
    /**
     * Destroys the object.
     */
    ~CodeFlowGraph();
    
    /**
     * Returns true if the block exists.
     * @param address
     * @return bool
     */
    bool doesBlockExist(uint64_t address) const;

    /**
     * Returns true if given block is a child of parent. False other ways.
     * @param parent
     * @param block
     * @return bool.
     */
    bool isBlockChildOf(CodeFlowBlock *parent, uint64_t block) const;

    /**
     * Pushes a children (to) into "from". Returns true if succeeded, returns false if any of the blocks does not exist.
     * @param from
     * @param to
     * @return bool
     */
    bool pushChildrenIfExist(uint64_t from, uint64_t to);

    /**
     * Pushes a new block to the list if it didn't exist. Returns true if block was pushed, false if block is already
     * present. If block is the first of the graph, its address will be saved in m_firstBlock.
     * @param type
     * @param address
     * @return bool
     */
    bool pushIfNotExists(CodeFlowBlockType type, uint64_t address);

    /**
     * Returns the block at given address. If block does not exist, nullptr is returned.
     * @param address
     * @return CodeFlowBlock *
     */
    CodeFlowBlock *getBlock(uint64_t address) const;

    /**
     * Prints current stored graph in the form:
     * blockAddr, type -> [childAddr1, childAddr2, childAddr3]
     * childAddr1, type -> [...]
     * childAddr2, type -> [...]
     * childAddr3, type -> [...]
     */
    void printGraph() const;

    /**
     * Returns every parent of the given block.
     * @param address
     * @return std::vector<CodeFlowBlock*>
     */
    std::vector<CodeFlowBlock *> getBlockParents(uint64_t address) const;

  private:
    // Address, Block. Ordered by address so search operations take O(log n).
    uint64_t m_firstBlock; // Address of the "parent of parents" block.
    std::unordered_map<uint64_t, std::shared_ptr<CodeFlowBlock>> m_blocks;
};

#endif // EZPACKER_CODEFLOWGRAPH_H
