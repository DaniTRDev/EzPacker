/**
 * @file MirEmitterContext.h
 * @brief Central bookkeeping for MIR allocation, IDs, active bindings, and arena pools.
 */
#ifndef EZPACKER_MIREMITTERCONTEXT_H
#define EZPACKER_MIREMITTERCONTEXT_H

#include "EzMirCommon.h"
#include "MirBlock.h"
#include "Function/MirFunction.h"
#include "Type/MirType.h"

/**
 * A struct that contains information about global data.
 */
struct MirGlobalDataEntry
{
    bool m_isReadOnly;
    bool m_uninitialized;
    ConstantArray<uint8_t> m_data;
    MirType *m_dataType;
    size_t m_entryId;
};

/**
 * Enumeration that contains the insertion mode that the emitter context will use.
 */
enum class InsertMode
{
    Append,      // Back.
    InsertBefore // Before a point.
};

struct InsertState
{
    MirBlock *block{ nullptr };
    InsertMode mode{ InsertMode::Append };
    TypedPoolLinkedList<MirInstruction>::Iterator iterator{};
};

class MirEmitterContext : public ErrorEmitter
{
  public:
    MirEmitterContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                      const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Sets the emitter to append instructions to the end of the specified block.
     * Replaces the old `setInsertPoint` logic.
     * @return `false` when `block` is `nullptr`; otherwise `true`.
     */
    bool setInsertPoint(MirBlock *block);

    /**
     * Sets the emitter to insert newly created instructions BEFORE the specified iterator.
     * The sequence of emitted instructions will be automatically preserved.
     */
    void setInsertPoint(MirBlock *block, TypedPoolLinkedList<MirInstruction>::Iterator insertBeforeIt);
    
    /**
     * Returns the block currently bound for instruction emission.
     */
    MirBlock *getCurrentBlock() const;
    
    MirBlock *createBlock();
    MirBlock *getBlockFromRef(const MirReference &ref) const;

    MirId createId();

    MirInstruction *createInstruction(MirInstructionOpCode opcode);

    MirFunction *
    createFunction(MirType *returnType, TypedPoolLinkedList<MirOperand *> *parameters, const std::string_view &name);

    /**
     * Returns a pointer to the function with matching id. Returns nullptr is there wasn't any matches.
     * @param id
     * @return
     */
    MirFunction *getFunctionById(size_t id) const;

    MirGlobalDataEntry *createGlobalData(const void *data, MirType *type, bool isReadOnly = true);
    MirGlobalDataEntry *createGlobalFloatingPoint(double val);
    MirGlobalDataEntry *createGlobalInteger(size_t sizeInBytes, uint64_t val);
    MirGlobalDataEntry *
    createGlobalString(const std::string_view &str, bool includeNullTerminator = true, bool isReadOnly = true);

    MirGlobalDataEntry *getGlobalDataEntryFromId(size_t entryId) const;

    TypedPool *getBlockPool();
    TypedPool *getDataEntryPool();
    TypedPool *getFunctionPool();
    TypedPool *getFunctionParameterPool();
    TypedPool *getInstructionPool();
    TypedPool *getOperandPool();
    TypedArrayPool<uint8_t> *getEntryDataPool();
    TypedPool *getTypePool();

    TypedPoolLinkedList<MirFunction> *getFunctionList() const;
    TypedPoolLinkedList<MirType> *getTypeList() const;

    std::shared_ptr<class MirTypes> getTypes() const;

  private:
    MirId m_currentId;
    MirFunction *m_currentBoundFunction;
    InsertState m_insertState;

    TypedPool m_blockPool;

    TypedPool m_dataEntryPool;
    TypedPool m_functionPool;
    TypedPool m_functionParameterPool;
    TypedPool m_instructionPool;
    TypedPool m_operandPool;
    TypedPool m_stackFramePool;
    TypedPool m_stackObjectPool; // Objects inside frames.
    TypedArrayPool<uint8_t> m_dataPool;

    TypedArrayPool<char> m_namePool;
    TypedPoolLinkedList<MirFunction> *m_functionList;
    TypedPoolLinkedList<MirType> *m_typeList;

    std::map<size_t, MirFunction *> m_blockIdToFunc;
    std::map<size_t, MirFunction *> m_idToFunctionMap;
    std::map<size_t, MirBlock *> m_idToBlockMap;
    std::map<size_t, MirGlobalDataEntry *> m_idToGlobalDataEntry;
    std::shared_ptr<class MirTypes> m_types;
};

#endif // EZPACKER_MIREMITTERCONTEXT_H