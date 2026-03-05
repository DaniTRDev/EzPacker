/**
 * @file MirEmitterContext.h
 * @brief Central bookkeeping for all MIR artefacts: blocks, functions,
 *        instructions, operands, types, IDs, and the "current block" cursor.
 *
 * MirEmitterContext owns the arena pools that back every MIR object and
 * provides factory methods (createBlock, createInstruction, createFunction,
 * createType, createId).  It also maintains the "currently bound block" so
 * that newly created instructions are automatically appended to it.
 *
 * The special constant MIRID_INVALID (0) is defined here and used
 * throughout the compiler to signal an unlinked or missing MIR ID.
 */
#ifndef EZPACKER_MIREMITTERCONTEXT_H
#define EZPACKER_MIREMITTERCONTEXT_H

#include "EzMirCommon.h"
#include "MirBlock.h"
#include "Function/MirFunction.h"
#include "Type/MirType.h"

using MirId = size_t;
constexpr MirId MIRID_INVALID = 0; // Easy error checking.

class MirEmitterContext : public ErrorEmitter
{
  public:
    /**
     * Creates the object with default values.
     * @param errorCollector
     * @param sourceManager
     */
    MirEmitterContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                      const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Binds the context to the given block.
     * @param block
     * @return bool
     */
    bool bindToBlock(MirBlock *block);

    /**
     * Creates a block and returns the allocated pointer to the new block.
     * @return MirBlock *
     */
    MirBlock *createBlock();

    /**
     * Returns the block this context is bound to at the moment of the call.
     * @return MirBlock *
     */
    MirBlock *getCurrentBoundBlock() const;

    /**
     * Creates an id and returns it.
     * @return MirId
     */
    MirId createId();

    /**
     * Creates an empty instruction with the given opcode.
     * @param opcode
     * @return opcode
     */
    MirInstruction *createInstruction(MirInstructionOpCode opcode);

    /**
     * Creates a function, appends it to the context and sets it as the active function. It also updates current
     * function and creates the entry point block for the function and sets it as the current one.
     * @param returnTypeId
     * @return MirFunction *
     */
    MirFunction *createFunction(size_t returnTypeId);

    /**
     * Creates a type with the given kind, subtypes and name. The type is added to the context and returned.
     * @param kind
     * @param types
     * @param name
     * @return MirType *
     */
    MirType *createType(MirTypeKind kind, TypedPoolSlice<MirType> *types, const std::string_view &name);

    /**
     * Returns the MIR type with the given ID, or nullptr if no type with that ID exists in the context.
     * @param id
     * @return MirType *
     */
    MirType *getMirTypeById(size_t id);
    
    /**
     * Returns the pool of blocks.
     * @return TypedPool.
     */
    TypedPool *getBlockPool();

    /**
     * Returns the pool of global data entries.
     * @return TypedPool *
     */
    TypedPool *getDataEntryPool();

    /**
     * Returns the pool of functions.
     * @return TypedPool.
     */
    TypedPool *getFunctionPool();

    /**
     * Returns the pool of function parameters.
     * @return  TypedPool *
     */
    TypedPool *getFunctionParameterPool();

    /**
     * Returns the pool of instructions.
     * @return TypedPool
     */
    TypedPool *getInstructionPool();

    /**
     * Returns the pool of instruction operands.
     * @return TypedPool *
     */
    TypedPool *getOperandPool();

    /**
     *  Returns the pool of the data stored in each entry.
     * @return TypedArrayPool<uint8_t> *
     */
    TypedArrayPool<uint8_t> *getEntryDataPool();

    /**
     * Returns the type pool.
     * @return TypedPool *
     */
    TypedPool *getTypePool();

  private:
    MirId m_currentId; // 0 == invalid.

    MirBlock *m_currentBoundBlock;
    MirFunction *m_currentBoundFunction;

    TypedPool m_blockPool;
    TypedPool m_dataEntryPool; // Pool to contain the entry itself, the entry data is independent.
    TypedPool m_functionPool;
    TypedPool m_functionParameterPool;
    TypedPool m_instructionPool;
    TypedPool m_operandPool;
    TypedPool m_typePool;
    TypedArrayPool<uint8_t> m_dataPool;          // Pool to contain the data of an entry.
    TypedPoolSlice<MirFunction> *m_functionList; // Linked list of functions managed by this context.
    TypedPoolSlice<MirType> *m_typeList; // Pool to contain the types used in the module, this is not managed by this
                                         // context but it is needed for type checking and function creation.
    std::map<size_t, MirType *> m_idToTypeMap; // Map to link a MIR type ID to the corresponding MIR type, this is used
                                               // to make type checking faster.
};

#endif // EZPACKER_MIREMITTERCONTEXT_H
