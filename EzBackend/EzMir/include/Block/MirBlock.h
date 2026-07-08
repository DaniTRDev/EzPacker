#ifndef EZPACKER_MIRBLOCK_H
#define EZPACKER_MIRBLOCK_H

#include "EzMirCommon.h"
#include "Instruction/MirInstruction.h"

class MirBlock
{
  public:
    /**
     * Creates a block wrapper around an existing instruction slice.
     *
     * @param id Unique MIR ID for this block. `0` is reserved as invalid by convention, so callers typically pass an ID
     * produced by `MirBuilderContext::createId()`.
     * @param sourceRef Source reference that originated this block.
     * @param instructions Arena-managed instruction slice associated with this block.
     * @param name
     */
    MirBlock(size_t id,
             SourceReference *sourceRef,
             std::pmr::list<MirInstruction *> instructions,
             const std::pmr::string &name = "");

    /**
     * Returns the instruction at given index. If index is out of bounds or invalid, nullptr is returned.
     * @param index
     * @return
     */
    MirInstruction *at(size_t index);

    /**
     * Returns the unique MIR ID assigned to this block.
     */
    size_t getId() const;

    /**
     * Returns the source reference that originated this block.
     * @return
     */
    SourceReference *getSourceRef() const;

    /**
     * Returns the mutable instruction slice for this block.
     * The returned slice is the same container that `MirBuilderContext` appends to when this block is currently bound.
     */
    std::pmr::list<MirInstruction *> &getInstructions();

    /**
     * Returns the inmutable instruction slice for this block.
     * The returned slice is the same container that `MirBuilderContext` appends to when this block is currently bound.
     */
    const std::pmr::list<MirInstruction *> &getInstructions() const;

    /**
     * Returns a pointer to the MUTABLE list of instructions.
     * @return
     */
    std::pmr::list<MirInstruction *> *getInstructionsPtr();

    /**
     * Returns the name of the block, if any.
     * @return
     */
    const std::pmr::string &getName() const;

  private:
    size_t m_id;
    SourceReference *m_sourceRef;
    std::pmr::list<MirInstruction *> m_instructions; // Arena-managed linked list of instructions.
    std::pmr::string m_name;
};

#endif // EZPACKER_MIRBLOCK_H
