#ifndef EZMIR_MIR_BLOCK_H
#define EZMIR_MIR_BLOCK_H

#include "EzMirCommon.h"

class MirBlock
{
  public:
    /**
     * Creates a block wrapper around an existing instruction slice. A block may or may not have a function owner.
     */
    MirBlock(MirId id,
             class SourceReference *sourceRef,
             std::pmr::list<class MirInstruction *> instructions,
             class MirFunction *owner = nullptr,
             const std::pmr::string &name = "");

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
     * Sets the owning function of this block.
     */
    void setOwner(class MirFunction *func);

    /**
     * Returns the mutable instruction slice for this block.
     * The returned slice is the same container that `MirBuilderContext` appends to when this block is currently bound.
     */
    std::pmr::list<class MirInstruction *> &getInstructions();

    /**
     * Returns the immutable instruction slice for this block.
     * The returned slice is the same container that `MirBuilderContext` appends to when this block is currently bound.
     */
    const std::pmr::list<class MirInstruction *> &getInstructions() const;

    /**
     * Returns a pointer to the MUTABLE list of instructions.
     */
    std::pmr::list<class MirInstruction *> *getInstructionsPtr();

    /**
     * Returns the name of the block, if any.
     */
    const std::pmr::string &getName() const;

  private:
    class MirFunction *m_owner;
    size_t m_id;
    class SourceReference *m_sourceRef;
    std::pmr::list<MirInstruction *> m_instructions; // Arena-managed linked list of instructions.
    std::pmr::string m_name;
};

#endif // EZMIR_MIR_BLOCK_H
