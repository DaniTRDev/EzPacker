#ifndef EZMIR_MIR_BLOCK_H
#define EZMIR_MIR_BLOCK_H

#include "EzMirCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"

class MirBlock
{
  public:
    /**
     * Creates a block wrapper around an existing instruction slice. A block may or may not have a function owner.
     */
    MirBlock(MirId id,
             class SourceReference *sourceRef,
             class MirFunction *owner = nullptr,
             const std::pmr::string &name = "");

    /**
     * Returns the mutable instruction slice for this block.
     * The returned slice is the same container that `MirBuilderContext` appends to when this block is currently bound.
     */
    IntrusiveLinkedList<class MirInstruction> &getInstructions();

    /**
     * Returns the immutable instruction slice for this block.
     * The returned slice is the same container that `MirBuilderContext` appends to when this block is currently bound.
     */
    const IntrusiveLinkedList<class MirInstruction> &getInstructions() const;

    /**
     * Returns a pointer to the MUTABLE list of instructions.
     */
    IntrusiveLinkedList<class MirInstruction> *getInstructionsPtr();

    /**
     * Returns an interator to the beginning of the instruction list.
     */
    IntrusiveLinkedList<MirInstruction>::iterator begin();

    /**
     * Returns an iterator to the end of the instruction list.
     */
    IntrusiveLinkedList<MirInstruction>::iterator end();

    /**
     * Returns the previous block to this.
     */
    MirBlock *getPrev() const;

    /**
     * Returns the next block to this.
     */
    MirBlock *getNext() const;

    /**
     * Returns the owner of this block.
     */
    class MirFunction *getOwner() const;

    /**
     * Returns the instruction at given index. If index is out of bounds or invalid, nullptr is returned.
     * @param index
     * @return
     */
    class MirInstruction *at(size_t index);

    /**
     * Returns the unique MIR ID assigned to this block.
     */
    MirId getId() const;

    /**
     * Returns the source reference that originated this block.
     * @return
     */
    class SourceReference *getSourceRef() const;

    /**
     * Returns the total number of instructions in this block.
     */
    size_t getInstrCount() const;

    /**
     * Sets the next block to this.
     */
    void setNext(MirBlock *next);

    /**
     * Sets the owning function of this block.
     */
    void setOwner(class MirFunction *func);

    /**
     * Sets the previous block to this.
     */
    void setPrev(MirBlock *prev);

    /**
     * Returns the name of the block, if any.
     */
    const std::pmr::string &getName() const;

  private:
    IntrusiveLinkedList<MirInstruction> m_instructions;
    MirBlock *m_prev{ nullptr };
    MirBlock *m_next{ nullptr };
    class MirFunction *m_owner;
    size_t m_id;
    class SourceReference *m_sourceRef;
    std::pmr::string m_name;
};

#endif // EZMIR_MIR_BLOCK_H
