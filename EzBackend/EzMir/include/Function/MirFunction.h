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
 * maintained by `MirBuilderContext`.
 */
#ifndef EZPACKER_MIRFUNCTION_H
#define EZPACKER_MIRFUNCTION_H

#include "EzMirCommon.h"
#include "Block/MirBlock.h"
#include "Type/MirType.h"
#include "MirFunctionStackFrame.h"
#include "CallingConvDesc.h"

/**
 * Important: Parameters MUST BE VIRTUAL/PHYSICAL REGISTERS.
 */
class MirFunction
{
  public:
    /**
     * Creates a function wrapper over arena-managed MIR data structures.
     *
     * @param callingConv
     * @param entryPoint    First block executed when the function starts.
     * @param stackFrame    A stack frame object that describes this function's stack frame.
     * @param returnType    MIR type describing the function's return value.
     * @param type
     * @param id            Unique MIR ID for the function itself.
     * @param sourceRef     Source reference that originated this function.
     * @param blocks        Ordered block slice belonging to the function.
     * @param parameters    Slice of parameter operands in declaration order.
     * @param name
     */
    MirFunction(CallingConvDesc *callingConv,
                MirBlock *entryPoint,
                MirFunctionStackFrame *stackFrame,
                MirType *returnType,
                MirType *type,
                MirId id,
                SourceReference *sourceRef,
                std::pmr::list<MirBlock *> blocks,
                std::pmr::list<MirRegister *> parameters,
                std::pmr::string name);

    /**
     * Returns the calling convention of this function.
     */
    CallingConvDesc *getCallingConv() const;

    /**
     * Returns the MirBlock owned by this function that matches the given ID, if no case is found nullptr is returned.
     * @param id
     * @return
     */
    MirBlock *getBlock(size_t id) const;

    /**
     * Returns the function entry block.
     */
    MirBlock *getEntryPoint() const;

    /**
     * Returns the stack frame linked to this object.
     * @return
     */
    MirFunctionStackFrame *getStackFrame() const;

    /**
     * Returns the return type of the function.
     * @return
     */
    MirType *getReturnType() const;

    /**
     * Returns the type of this function.
     */
    MirType *getType() const;

    /**
     * Returns the unique MIR ID assigned to this function.
     */
    MirId getId() const;

    /**
     * Returns the source reference that created this function.
     * @return
     */
    SourceReference *getSourceRef() const;

    /**
     * Returns the mutable list of blocks that belong to this function.
     *
     * The list always contains the entry point as its first block right after
     * `MirBuilderContext::createFunction()` succeeds.
     */
    std::pmr::list<MirBlock *> &getBlocks();

    /**
     * Returns a pointer to the mutable list of blocks that belong to this function.
     *
     * The list always contains the entry point as its first block right after
     * `MirBuilderContext::createFunction()` succeeds.
     */
    std::pmr::list<MirBlock *> *getBlocksPtr();

    /**
     * Returns the MUTABLE parameter list for this function.
     *
     * Each element is a `MirFuncParam*` describing one incoming parameter. The
     * exact calling-convention meaning is defined by later lowering stages.
     */
    std::pmr::list<MirRegister *> &getParameters();

    /**
     * Returns the name of the function.
     * @return
     */
    const std::pmr::string &getName();

  private:
    CallingConvDesc *m_callingConv;
    MirBlock *m_entryPoint;
    MirFunctionStackFrame *m_stackFrame;
    MirType *m_returnType;
    MirType *m_type;
    MirId m_id;
    SourceReference *m_sourceRef;

    std::pmr::list<MirBlock *> m_blocks; // Arena-managed blocks belonging to this function.
    std::pmr::list<MirRegister *> m_parameters;
    std::pmr::map<MirId, MirBlock *> m_blockIdToBlock;
    std::pmr::string m_name;
};

#endif // EZPACKER_MIRFUNCTION_H
