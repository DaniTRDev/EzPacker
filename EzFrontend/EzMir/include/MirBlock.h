/**
 * @file MirBlock.h
 * @brief A basic block in the MIR — a straight-line sequence of instructions.
 *
 * Every MirBlock has a unique ID and owns a linked list of MirInstruction
 * objects.  Basic blocks are the fundamental unit of the control-flow graph:
 * each block has a single entry point and ends with a terminator instruction
 * (JMP, conditional jump, RET, or HALT).  Blocks are created and managed by
 * MirEmitterContext.
 */
#ifndef EZPACKER_MIRBLOCK_H
#define EZPACKER_MIRBLOCK_H

#include "EzMirCommon.h"
#include "Instruction/MirInstruction.h"

class MirBlock
{
  public:
    /**
     * Creates the block with the given ID.
     * @param id
     * @param instructions Initial instructions of the block or empty container (NOT NULL).
     */
    MirBlock(size_t id, TypedPoolSlice<MirInstruction> *instructions);

    /**
     * Returns the ID of this block.
     * @return size_t
     */
    size_t getId() const;

    /**
     * Returns a read-only instruction list.
     * @return TypedPoolSlice<MirInstruction> *
     */
    TypedPoolSlice<MirInstruction> *getInstructions() const;

  private:
    size_t m_id;
    TypedPoolSlice<MirInstruction> *m_instructions; // Linked list of instructions.
};

#endif // EZPACKER_MIRBLOCK_H
