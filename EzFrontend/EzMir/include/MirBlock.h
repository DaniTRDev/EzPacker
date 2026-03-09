/**
 * @file MirBlock.h
 * @brief A basic block in the MIR — a straight-line sequence of instructions.
 *
 * Every `MirBlock` has a unique MIR ID and stores its instructions in a
 * `TypedPoolSlice<MirInstruction>` allocated by `MirEmitterContext`.
 * The block itself does not own the slice memory; it simply points at the
 * arena-managed list that the context appends to whenever it is bound and new
 * instructions are created.
 *
 * `MirBlock` is intentionally small: it models only the ordered instruction
 * list and the block identifier. Control-flow meaning comes from the final
 * instruction(s) stored in the block, typically a terminator such as `JMP`,
 * conditional branch, `RET`, or `HALT`.
 */
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
     * @param id Unique MIR ID for this block. `0` is reserved as invalid by
     *           convention, so callers typically pass an ID produced by
     *           `MirEmitterContext::createId()`.
     * @param instructions Arena-managed instruction slice associated with this
     *                     block. The pointer is expected to remain valid for the
     *                     lifetime of the owning context.
     */
    MirBlock(size_t id, TypedPoolSlice<MirInstruction> *instructions);

    /**
     * Returns the unique MIR ID assigned to this block.
     */
    size_t getId() const;

    /**
     * Returns the mutable instruction slice for this block.
     *
     * The returned slice is the same container that `MirEmitterContext`
     * appends to when this block is currently bound.
     */
    TypedPoolSlice<MirInstruction> *getInstructions() const;

  private:
    size_t m_id;
    TypedPoolSlice<MirInstruction> *m_instructions; // Arena-managed linked list of instructions.
};

#endif // EZPACKER_MIRBLOCK_H
