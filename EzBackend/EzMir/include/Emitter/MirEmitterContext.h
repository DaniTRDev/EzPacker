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

struct MirGlobalDataEntry
{
    bool m_isReadOnly;
    bool m_uninitialized;
    MirType *m_dataType;
    size_t m_entryId;
    ConstantArray<uint8_t> m_data;
};

// Clean internal state for tracking exactly where instructions go.
enum class InsertMode
{
    Append,
    InsertBefore
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
     * Replaces the old `bindToBlock` logic.
     * @return `false` when `block` is `nullptr`; otherwise `true`.
     */
    bool setInsertPoint(MirBlock *block);

    /**
     * Sets the emitter to insert newly created instructions BEFORE the specified iterator.
     * The sequence of emitted instructions will be automatically preserved.
     */
    void setInsertPoint(MirBlock *block, TypedPoolLinkedList<MirInstruction>::Iterator insertBeforeIt);

    /**
     * Legacy wrapper for backward compatibility.
     */
    bool bindToBlock(MirBlock *block) { return setInsertPoint(block); }

    /**
     * Returns the block currently bound for instruction emission.
     */
    MirBlock *getCurrentBoundBlock() const;

    bool doesTypeExist(size_t typeId) const;
    bool doesTypeExist(const std::string_view &typeName) const;

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

    MirType *createType(MirTypeKind kind,
                        size_t totalSizeInBytes,
                        TypedPoolLinkedList<MirType> *subTypes,
                        const std::string_view &name);
    /**
     * Finds and returns an integer MirType of the specified size.
     * Returns nullptr if no such type has been created yet.
     */
    MirType *getIntegerTypeBySize(size_t sizeInBytes);
    MirType *getMirTypeById(size_t id);

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
    TypedPool m_typePool;
    TypedPool m_stackFramePool;
    TypedPool m_stackObjectPool; // Objects inside frames.
    TypedArrayPool<uint8_t> m_dataPool;

    TypedArrayPool<char> m_namePool;
    TypedPoolLinkedList<MirFunction> *m_functionList;
    TypedPoolLinkedList<MirType> *m_typeList;

    std::map<size_t, MirFunction *> m_blockIdToFunc;
    std::map<size_t, MirFunction *> m_idToFunctionMap;
    std::map<size_t, MirType *> m_idToTypeMap;
    std::map<size_t, MirBlock *> m_idToBlockMap;
    std::map<size_t, MirGlobalDataEntry *> m_idToGlobalDataEntry;
    std::set<std::string_view> m_typeNames;
    std::shared_ptr<class MirTypes> m_types;
};

#endif // EZPACKER_MIREMITTERCONTEXT_H