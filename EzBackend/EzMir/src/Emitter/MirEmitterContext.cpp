#include "Emitter/MirEmitterContext.h"
#include <cstring>
#include <algorithm>

MirEmitterContext::MirEmitterContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                                     const std::shared_ptr<SourceManager> &sourceManager) :
    m_currentId(1), m_currentBoundFunction(nullptr), ErrorEmitter(errorCollector, sourceManager)
{
    m_functionList = m_functionPool.createLinkedList<MirFunction>();
    m_typeList = m_typePool.createLinkedList<MirType>();
}

bool MirEmitterContext::setInsertPoint(MirBlock *block)
{
    m_insertState.block = block;
    m_insertState.mode = InsertMode::Append;
    m_insertState.iterator = {}; // Clear iterator
    return block != nullptr;
}

void MirEmitterContext::setInsertPoint(MirBlock *block, TypedPoolLinkedList<MirInstruction>::Iterator insertBeforeIt)
{
    m_insertState.block = block;
    m_insertState.mode = InsertMode::InsertBefore;
    m_insertState.iterator = insertBeforeIt;
}

MirBlock *MirEmitterContext::getCurrentBoundBlock() const { return m_insertState.block; }

bool MirEmitterContext::doesTypeExist(size_t typeId) const { return m_idToTypeMap.contains(typeId); }

bool MirEmitterContext::doesTypeExist(const std::string_view &typeName) const { return m_typeNames.contains(typeName); }

MirBlock *MirEmitterContext::createBlock()
{
    MirBlock *block = m_blockPool.create<MirBlock>(createId(), m_instructionPool.createLinkedList<MirInstruction>());

    if (m_currentBoundFunction)
    {
        // If we are in a function, append the new block to its blocks too.
        m_blockPool.appendToListBack(m_currentBoundFunction->getBlocks(), block);
    }

    m_idToBlockMap.insert({ block->getId(), block });
    return block;
}

MirBlock *MirEmitterContext::getBlockFromRef(const MirReference &ref) const
{
    if (ref.getRefType() != MirReferenceType::Block)
        return nullptr;

    auto it = m_idToBlockMap.find(ref.get<MirReference>()->getRefId());
    if (it != m_idToBlockMap.end())
        return it->second;

    return nullptr;
}

MirId MirEmitterContext::createId() { return m_currentId++; }

MirInstruction *MirEmitterContext::createInstruction(MirInstructionOpCode opcode)
{
    MirInstruction *instr =
            m_instructionPool.create<MirInstruction>(opcode, m_operandPool.createLinkedList<MirOperand>());

    // If no block is bound, just return the floating/orphan instruction
    if (!m_insertState.block)
        return instr;

    // Insert it into the current block cleanly
    auto *targetList = m_insertState.block->getInstructions();

    if (m_insertState.mode == InsertMode::Append)
    {
        m_instructionPool.appendToListBack(targetList, instr);
    }
    else
    {
        // Because we always insert BEFORE the iterator, the iterator remains pointing
        // to the original target. Subsequent emissions will naturally form a correct sequence!
        m_instructionPool.appendToListBefore(targetList, m_insertState.iterator, instr);
    }

    return instr;
}

MirFunction *MirEmitterContext::createFunction(MirType *returnType,
                                               TypedPoolLinkedList<MirOperand *> *parameters,
                                               const std::string_view &name)
{
    if (!returnType)
    {
        emitError(ErrorSeverity::Fatal,
                  "Could not create function because return type is null",
                  "MirEmitterContext::createFunction");
        return nullptr;
    }

    TypedPoolLinkedList<MirBlock> *functionBlockList = m_blockPool.createLinkedList<MirBlock>();

    if (!parameters)
        parameters = m_operandPool.createLinkedList<MirOperand *>();

    /*
     * We can't use createBlock here because we need to create the block WITHOUT appending it to the current function,
     * if any.
     */
    MirBlock *entryPoint =
            m_blockPool.create<MirBlock>(createId(), m_instructionPool.createLinkedList<MirInstruction>());
    m_blockPool.appendToListBack(functionBlockList, entryPoint);

    ConstantArray<char> copiedName = m_namePool.createConstantArray(name.length() + 1); // Include null terminator
    std::strcpy(copiedName.m_elems, name.data());
    copiedName.m_elems[name.length()] = '\0'; // Ensure null terminator is included.

    MirFunctionStackFrame *stackFrame = m_stackFramePool.create<MirFunctionStackFrame>(nullptr, &m_stackObjectPool);
    MirFunction *func = m_functionPool.createAndAppendToListBack<MirFunction>(m_functionList,
                                                                              entryPoint,
                                                                              returnType,
                                                                              stackFrame,
                                                                              createId(),
                                                                              functionBlockList,
                                                                              parameters,
                                                                              copiedName.m_elems);

    stackFrame->setOwner(func);

    m_currentBoundFunction = func;
    setInsertPoint(entryPoint);

    return func;
}

MirGlobalDataEntry *MirEmitterContext::createGlobalData(const void *data, size_t size, bool isReadOnly)
{
    MirGlobalDataEntry *entry = getDataEntryPool()->create<MirGlobalDataEntry>();
    entry->m_isReadOnly = isReadOnly;
    entry->m_uninitialized = false;
    entry->m_entryId = createId();
    entry->m_dataSize = size;
    entry->m_data = getEntryDataPool()->createConstantArray(size);

    if (!data)
    {
        memset(entry->m_data.m_elems, '\0', size);
        entry->m_uninitialized = true;
    }
    else
    {
        std::copy_n((const char *)data, size, (char *)entry->m_data.m_elems);
    }

    m_idToGlobalDataEntry.insert({ entry->m_entryId, entry });
    return entry;
}

MirGlobalDataEntry *MirEmitterContext::createGlobalFloatingPoint(double val)
{
    return createGlobalData(&val, sizeof(val), true);
}

MirGlobalDataEntry *MirEmitterContext::createGlobalInteger(uint64_t val)
{
    return createGlobalData(&val, sizeof(val), true);
}

MirGlobalDataEntry *
MirEmitterContext::createGlobalString(const std::string_view &str, bool includeNullTerminator, bool isReadOnly)
{
    size_t totalSize = str.size() + (includeNullTerminator ? 1 : 0);
    MirGlobalDataEntry *entry = getDataEntryPool()->create<MirGlobalDataEntry>();

    entry->m_isReadOnly = isReadOnly;
    entry->m_uninitialized = false;
    entry->m_entryId = createId();
    entry->m_dataSize = totalSize;
    entry->m_data = getEntryDataPool()->createConstantArray(totalSize);
    std::copy_n(str.data(), str.size(), entry->m_data.m_elems);

    if (includeNullTerminator)
    {
        entry->m_data.m_elems[str.size()] = '\0';
    }

    m_idToGlobalDataEntry.insert({ entry->m_entryId, entry });
    return entry;
}

MirGlobalDataEntry *MirEmitterContext::getGlobalDataEntryFromId(size_t entryId)
{
    auto it = m_idToGlobalDataEntry.find(entryId);
    if (it != m_idToGlobalDataEntry.end())
        return it->second;

    return nullptr;
}

MirType *MirEmitterContext::createType(MirTypeKind kind,
                                       size_t totalSizeInBytes,
                                       TypedPoolLinkedList<MirType> *types,
                                       const std::string_view &name)
{
    if (name.empty())
    {
        emitError(ErrorSeverity::Fatal, "Could not create type because name is empty", "MirEmitterContext::createType");
        return nullptr;
    }

    MirType *type = m_typePool.create<MirType>(kind, createId(), totalSizeInBytes, types, name);

    m_typePool.appendToListBack(m_typeList, type);
    m_typeNames.insert(name);
    m_idToTypeMap.insert({ type->getId(), type });

    return type;
}

MirType *MirEmitterContext::getIntegerTypeBySize(size_t sizeInBytes)
{
    // Iterate through all types registered in the context
    for (auto it = m_typeList->begin(); it != m_typeList->end(); ++it)
    {
        MirType *type = *it;
        if (type->getKind() == MirTypeKind::Integer && type->getTotalSizeInBytes() == sizeInBytes)
        {
            return type;
        }
    }

    emitError(ErrorSeverity::Fatal,
              "Could not find an integer type of the requested size",
              "MirEmitterContext::getIntegerTypeBySize");
    return nullptr;
}

MirType *MirEmitterContext::getMirTypeById(size_t id)
{
    if (id == 0)
    {
        emitError(ErrorSeverity::Fatal, "Cannot get MIR type with null ID", "MirEmitterContext::getMirTypeById");
        return nullptr;
    }

    auto it = m_idToTypeMap.find(id);
    if (it == m_idToTypeMap.end())
    {
        emitError(ErrorSeverity::Fatal,
                  "No MIR type with the given ID exists in the context",
                  "MirEmitterContext::getMirTypeById");
        return nullptr;
    }

    return it->second;
}

TypedPool *MirEmitterContext::getBlockPool() { return &m_blockPool; }

TypedPool *MirEmitterContext::getFunctionPool() { return &m_functionPool; }

TypedPool *MirEmitterContext::getFunctionParameterPool() { return &m_functionParameterPool; }

TypedPool *MirEmitterContext::getInstructionPool() { return &m_instructionPool; }

TypedPool *MirEmitterContext::getOperandPool() { return &m_operandPool; }

TypedPool *MirEmitterContext::getDataEntryPool() { return &m_dataEntryPool; }

TypedPool *MirEmitterContext::getTypePool() { return &m_typePool; }

TypedArrayPool<uint8_t> *MirEmitterContext::getEntryDataPool() { return &m_dataPool; }

TypedPoolLinkedList<MirFunction> *MirEmitterContext::getFunctionList() const { return m_functionList; }

TypedPoolLinkedList<MirType> *MirEmitterContext::getTypeList() const { return m_typeList; }
