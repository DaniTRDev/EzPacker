/**
 * @file MirFunction.h
 * @brief Function-level container in the MIR: entry point, blocks, parameters, return type.
 *
 * A MirFunction groups an ordered list of basic blocks into a single
 * callable unit.  It has a designated entry-point block, a return-type ID,
 * a unique function ID, and a parameter operand list.  Functions are
 * created through MirEmitterContext::createFunction().
 */
#ifndef EZPACKER_MIRFUNCTION_H
#define EZPACKER_MIRFUNCTION_H

#include "EzMirCommon.h"
#include "MirBlock.h"

class MirFunction
{
  public:
    /**
     * Creates a function with the given entry point, ID, return type, block list, and parameters.
     * @param entryPoint   The first block to execute when the function is called.
     * @param id           Unique identifier for this function within the compilation.
     * @param returnTypeId The MIR type ID representing the function's return type.
     * @param blocks       Linked list of all basic blocks owned by this function.
     * @param parameters   Linked list of operands representing the function's parameters.
     */
    MirFunction(MirBlock *entryPoint,
                size_t id,
                size_t returnTypeId,
                TypedPoolSlice<MirBlock> *blocks,
                TypedPoolSlice<MirOperand> *parameters);

    /**
     * Returns the entry point block of this function.
     * @return  MirBlock *
     */
    MirBlock *getEntryPoint();

    /**
     * Returns the ID that represents this function in the current compilation.
     * @return size_t
     */
    size_t getId();

    /**
     * Returns the return type of this function.
     * @return size_t
     */
    size_t getReturnTypeId();

    /**
     * Returns the blocks owned by this function.
     * @return TypedPoolSlice<MirBlock> *
     */
    TypedPoolSlice<MirBlock> *getBlocks();

    /**
     * Returns the linked list of parameters used by this function.
     * @return TypedPoolSlice *
     */
    TypedPoolSlice<MirOperand> *getParameters();

  private:
    MirBlock *m_entryPoint;
    size_t m_id;
    size_t m_returnTypeId;
    TypedPoolSlice<MirBlock> *m_blocks; // blocks owned by this function.
    TypedPoolSlice<MirOperand> *m_parameters;
};

#endif // EZPACKER_MIRFUNCTION_H
