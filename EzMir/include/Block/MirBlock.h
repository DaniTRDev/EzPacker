#ifndef EZMIR_MIR_BLOCK_H
#define EZMIR_MIR_BLOCK_H

#include "EzMirCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"
#include <utility>

/**
 * Represents a basic block in the Machine Intermediate Representation (MIR) control-flow graph.
 *
 * Contains an intrusive doubly-linked list of instructions executing sequentially with a single entry
 * and single exit. Implements nodes of an intrusive doubly-linked list within its parent MirFunction.
 */
class MirBlock
{
  public:
    friend class MirBlockBuilder;
    friend class MirFunctionBuilder;
    friend class IntrusiveLinkedList<MirBlock>;
    friend class MirInstructionBuilder;
    friend class MirInstruction;

    /**
     * Constructs a basic block with a unique MIR ID, optional source reference, owning function, arena
     * allocator and name.
     */
    MirBlock(MirId id,
             class SourceReference *sourceRef,
             class MirFunction *owner,
             std::pmr::memory_resource *alloc,
             const std::pmr::string &name = "");

    /**
     * Returns the mutable intrusive instruction list for this basic block.
     */
    IntrusiveLinkedList<class MirInstruction> &getInstructions() { return m_instructions; }

    /**
     * Returns the immutable intrusive instruction list for this basic block.
     */
    const IntrusiveLinkedList<class MirInstruction> &getInstructions() const;

    /**
     * Returns the last instruction in the basic block, or nullptr if empty.
     */
    class MirInstruction *back() const { return m_instructions.back(); }

    /**
     * Returns the first instruction in the basic block, or nullptr if empty.
     */
    class MirInstruction *front() const { return m_instructions.front(); }

    /**
     * Returns a const iterator pointing to the first instruction in the block.
     */
    IntrusiveLinkedList<MirInstruction>::const_iterator begin() const;

    /**
     * Returns a const iterator pointing past the last instruction in the block.
     */
    IntrusiveLinkedList<MirInstruction>::const_iterator end() const;

    /**
     * Returns the preceding basic block in the function's intrusive layout sequence.
     */
    MirBlock *getPrev() const;

    /**
     * Returns the subsequent basic block in the function's intrusive layout sequence.
     */
    MirBlock *getNext() const;

    /**
     * Returns the parent function owning this basic block, or nullptr if orphaned.
     */
    class MirFunction *getOwner() const;

    /**
     * Retrieves the instruction at the specified 0-based offset, or nullptr if index is out of range.
     */
    class MirInstruction *at(size_t index);

    /**
     * Returns the unique MIR ID assigned to this basic block.
     */
    MirId getId() const;

    /**
     * Returns the source location reference for diagnostics.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Returns the total count of instructions contained in this basic block.
     */
    size_t getInstrCount() const;

    /**
     * Returns the label name of the basic block.
     */
    const std::pmr::string &getName() const;

    /**
     * Sets the label name of the basic block.
     */
    void setName(const std::pmr::string &name);

    /**
     * Returns the block's predecessors in ascending MirId order. This is the exact order in which
     * NonSsaToSsaPass fills PHI incoming operands, so later stages can map operand slots to paths
     * without rescanning instruction operands.
     */
    const std::pmr::vector<MirBlock *> &getPredecessors() const { return m_predecessors; }

    /**
     * Replaces the cached predecessor list. Owned and populated by CodeFlowAnalysisPass.
     */
    void setPredecessors(std::pmr::vector<MirBlock *> predecessors) { m_predecessors = std::move(predecessors); }

  private:
    /**
     * Sets the subsequent basic block in the intrusive list.
     */
    void setNext(MirBlock *next);

    /**
     * Sets the owning function of this basic block.
     */
    void setOwner(class MirFunction *func);

    /**
     * Sets the preceding basic block in the intrusive list.
     */
    void setPrev(MirBlock *prev);

  private:
    /**
     * Intrusive linked list of instructions contained within this basic block.
     */
    IntrusiveLinkedList<MirInstruction> m_instructions;

    /**
     * Intrusive pointer to previous block in layout order.
     */
    MirBlock *m_prev{ nullptr };

    /**
     * Intrusive pointer to next block in layout order.
     */
    MirBlock *m_next{ nullptr };

    /**
     * Parent function owning this block.
     */
    class MirFunction *m_owner;

    /**
     * Unique MIR identifier for this basic block.
     */
    size_t m_id;

    /**
     * Source code reference.
     */
    class SourceReference *m_sourceRef;

    /**
     * Diagnostic/assembly label name of the block.
     */
    std::pmr::string m_name;

    /**
     * Predecessor blocks in ascending MirId order (analysis metadata; see getPredecessors).
     */
    std::pmr::vector<MirBlock *> m_predecessors;
};

#endif // EZMIR_MIR_BLOCK_H
