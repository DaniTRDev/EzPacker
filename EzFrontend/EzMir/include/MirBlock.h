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
