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
#include "Type/MirType.h"
#include "MirFunctionStackFrame.h"

/**
 * Important: Parameters MUST BE VIRTUAL/PHYSICAL REGISTERS.
 */
class MirFunction
{
  public:
    /**
     * Creates a function wrapper over arena-managed MIR data structures.
     *
     * @param entryPoint    First block executed when the function starts.
     * @param id            Unique MIR ID for the function itself.
     * @param returnType    MIR type describing the function's return value.
     * @param blocks        Ordered block slice belonging to the function.
     * @param parameters    Slice of parameter operands in declaration order.
     * @param name
     */
    MirFunction(MirBlock *entryPoint,
                MirType *returnType,
                size_t id,
                std::pmr::list<MirBlock> *blocks,
                std::pmr::vector<MirOperand *> *parameters,
                std::pmr::string name);

    /**
     * Returns the name of the function.
     * @return
     */
    const char *getName();

    /**
     * Returns the function entry block.
     */
    MirBlock *getEntryPoint();

    /**
     * Returns the stack frame linked to this object.
     * @return
     */
    MirFunctionStackFrame *getStackFrame();

    /**
     * Returns the return type of the function.
     * @return
     */
    MirType *getReturnType();

    /**
     * Returns the unique MIR ID assigned to this function.
     */
    size_t getId();

    /**
     * Returns the mutable list of blocks that belong to this function.
     *
     * The list always contains the entry point as its first block right after
     * `MirEmitterContext::createFunction()` succeeds.
     */
    TypedPoolLinkedList<MirBlock> *getBlocks();

    /**
     * Returns the parameter list for this function.
     *
     * Each element is a `MirOperand` describing one incoming parameter. The
     * exact calling-convention meaning is defined by later lowering stages.
     */
    TypedPoolLinkedList<MirOperand *> *getParameters();

    /**
     * Appends a parameter to the function.
     * @param param
     * @param paramType
     * @param name
     */
    void appendParameter(MirRegister *param, const char *name);

  private:
    MirBlock *m_entryPoint;
    MirFunctionStackFrame *m_stackFrame;
    MirType *m_returnType;
    size_t m_id;
    TypedPoolLinkedList<MirBlock> *m_blocks; // Arena-managed blocks belonging to this function.
    TypedPoolLinkedList<MirOperand *> *m_parameters;
    const char *m_name;
};

#endif // EZPACKER_MIRFUNCTION_H
