/**
 * @file MirFunction.h
 * @brief Function-level MIR container: entry point, block list, parameters, and return type.
 *
 * A `MirFunction` groups a set of `MirBlock`s into one callable MIR unit.
 * The object stores:
 *   - an entry-point block,
 *   - a unique MIR function ID,
 *   - the MIR type ID of the return value,
 *   - the arena-managed block list for the function,
 *   - and the operand list describing its parameters.
 *
 * The function does not own these slices directly; they are allocated and
 * maintained by `MirEmitterContext`. When a function is the currently active
 * function inside the context, newly created blocks are appended to its block
 * list automatically.
 */
#ifndef EZPACKER_MIRFUNCTION_H
#define EZPACKER_MIRFUNCTION_H

#include "EzMirCommon.h"
#include "MirBlock.h"

class MirFunction
{
  public:
    /**
     * Creates a function wrapper over arena-managed MIR data structures.
     *
     * @param entryPoint   First block executed when the function starts.
     * @param id           Unique MIR ID for the function itself.
     * @param returnTypeId MIR type ID describing the function's return value.
     * @param blocks       Ordered block slice belonging to the function.
     * @param parameters   Slice of parameter operands in declaration order.
     */
    MirFunction(MirBlock *entryPoint,
                size_t id,
                size_t returnTypeId,
                TypedPoolSlice<MirBlock> *blocks,
                TypedPoolSlice<MirOperand> *parameters);

    /**
     * Returns the function entry block.
     */
    MirBlock *getEntryPoint();

    /**
     * Returns the unique MIR ID assigned to this function.
     */
    size_t getId();

    /**
     * Returns the MIR type ID of the function's return value.
     */
    size_t getReturnTypeId();

    /**
     * Returns the mutable list of blocks that belong to this function.
     *
     * The list always contains the entry point as its first block right after
     * `MirEmitterContext::createFunction()` succeeds.
     */
    TypedPoolSlice<MirBlock> *getBlocks();

    /**
     * Returns the parameter list for this function.
     *
     * Each element is a `MirOperand` describing one incoming parameter. The
     * exact calling-convention meaning is defined by later lowering stages.
     */
    TypedPoolSlice<MirOperand> *getParameters();

  private:
    MirBlock *m_entryPoint;
    size_t m_id;
    size_t m_returnTypeId;
    TypedPoolSlice<MirBlock> *m_blocks; // Arena-managed blocks belonging to this function.
    TypedPoolSlice<MirOperand> *m_parameters;
};

#endif // EZPACKER_MIRFUNCTION_H
