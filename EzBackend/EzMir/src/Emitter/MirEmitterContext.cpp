#include "Emitter/MirEmitterContext.h"
#include "Type/MirTypeTable.h"

MirEmitterContext::MirEmitterContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                                     const std::shared_ptr<SourceManager> &sourceManager) :
    m_currentId(1), m_currentBoundFunction(nullptr), ErrorEmitter(errorCollector, sourceManager)
{
    m_functionList = m_functionPool.linkedList<MirFunction>();
    m_typeList = m_typePool.linkedList<MirType>();

    m_types = std::make_shared<MirTypeTable>();
    m_types->initialize(this);
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

MirBlock *MirEmitterContext::getCurrentBlock() const { return m_insertState.block; }

MirBlock *MirEmitterContext::createBlock()
{
    MirBlock *block = m_blockPool.create<MirBlock>(createId(), m_instructionPool.linkedList<MirInstruction>());
    LOG_DEBUG(std::format("Creating block (id: {})", block->getId()), "MirEmitterContext");

    if (m_currentBoundFunction)
    {
        size_t blockId = block->getId();
        m_blockIdToFunc.insert({ blockId, m_currentBoundFunction });

        // If we are in a function, append the new block to its blocks too.
        LOG_DEBUG(std::format("Block appended to function (id: {}) (func: {})",
                              blockId,
                              m_currentBoundFunction->getName()),
                  "MirEmitterContext");
        m_currentBoundFunction->getBlocks()->appendBack(block);
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
    MirInstruction *instr = m_instructionPool.create<MirInstruction>(opcode, m_operandPool.linkedList<MirOperand>());

    // If no block is bound, just return the floating/orphan instruction
    if (!m_insertState.block)
        return instr;

    // Insert it into the current block cleanly
    auto *targetList = m_insertState.block->getInstructions();

    if (m_insertState.mode == InsertMode::Append)
    {
        targetList->appendBack(instr);
    }
    else
    {
        // Because we always insert BEFORE the iterator, the iterator remains pointing
        // to the original target. Subsequent emissions will naturally form a correct sequence!
        targetList->appendBefore(m_insertState.iterator, instr);
    }

    LOG_DEBUG(std::format("Instruction inserted into block (id: {}) (instr: {})",
                          m_insertState.block->getId(),
                          instr->toString()),
              "MirEmitterContext");

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

    TypedPoolLinkedList<MirBlock> *functionBlockList = m_blockPool.linkedList<MirBlock>();

    if (!parameters)
        parameters = m_operandPool.linkedList<MirOperand *>();

    /*
     * We can't use createBlock here because we need to create the block WITHOUT appending it to the current function,
     * if any.
     */
    MirBlock *entryPoint = m_blockPool.create<MirBlock>(createId(), m_instructionPool.linkedList<MirInstruction>());
    functionBlockList->appendBack(entryPoint);

    ConstantArray<char> copiedName = m_namePool.createConstantArray(name.length() + 1); // Include null terminator
    std::strcpy(copiedName.m_elems, name.data());
    copiedName.m_elems[name.length()] = '\0'; // Ensure null terminator is included.

    MirFunctionStackFrame *stackFrame = m_stackFramePool.create<MirFunctionStackFrame>(nullptr, &m_stackObjectPool);
    MirFunction *func = m_functionList->createAndAppendBack(entryPoint,
                                                            returnType,
                                                            stackFrame,
                                                            createId(),
                                                            functionBlockList,
                                                            parameters,
                                                            copiedName.m_elems);

    stackFrame->setOwner(func);

    LOG_DEBUG(std::format("Creating function (id: {}) (name: {}) (ReturnType: {}) (argCount: {})",
                          func->getId(),
                          func->getName(),
                          func->getReturnType()->getName(),
                          func->getParameters()->m_numElems),
              "MirEmitterContext");

    m_blockIdToFunc.insert({ entryPoint->getId(), func });
    m_idToFunctionMap.insert({ func->getId(), func });
    m_currentBoundFunction = func;
    setInsertPoint(entryPoint);

    return func;
}

MirFunction *MirEmitterContext::getFunctionById(size_t id) const
{
    auto it = m_idToFunctionMap.find(id);
    if (it == m_idToFunctionMap.end())
    {
        it = m_blockIdToFunc.find(id);
    }

    return it != m_idToFunctionMap.end() ? it->second : nullptr;
}

MirGlobalDataEntry *MirEmitterContext::createGlobalData(const void *data, MirType *type, bool isReadOnly)
{
    size_t size = type->getTotalSizeInBytes();
    MirGlobalDataEntry *entry = getDataEntryPool()->create<MirGlobalDataEntry>();
    entry->m_isReadOnly = isReadOnly;
    entry->m_uninitialized = false;
    entry->m_entryId = createId();
    entry->m_dataType = type;
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

    LOG_DEBUG(std::format("Creating global data (id: {}) (type: {}) (size: {}) (initialized: {}) (readOnly: {})",
                          entry->m_entryId,
                          entry->m_dataType->getName(),
                          entry->m_dataType->getTotalSizeInBytes(),
                          !entry->m_uninitialized,
                          entry->m_isReadOnly),
              "MirEmitterContext");

    m_idToGlobalDataEntry.insert({ entry->m_entryId, entry });
    return entry;
}

MirGlobalDataEntry *MirEmitterContext::createGlobalFloatingPoint(double val)
{
    return createGlobalData(&val, m_types->getFloat64Type(), true);
}

MirGlobalDataEntry *MirEmitterContext::createGlobalInteger(size_t sizeInBytes, uint64_t val)
{
    return createGlobalData(&val, getIntegerTypeBySize(sizeInBytes), true);
}

MirGlobalDataEntry *
MirEmitterContext::createGlobalString(const std::string_view &str, bool includeNullTerminator, bool isReadOnly)
{
    size_t totalSize = str.size() + (includeNullTerminator ? 1 : 0);
    MirGlobalDataEntry *entry = getDataEntryPool()->create<MirGlobalDataEntry>();

    TypedPoolLinkedList<MirType> *pointerSubTypes = getTypePool()->linkedList<MirType>();
    pointerSubTypes->appendBack(m_types->getInt8Type());

    MirType *type = createType(MirTypeKind::Pointer, totalSize, pointerSubTypes, "String");

    entry->m_isReadOnly = isReadOnly;
    entry->m_uninitialized = false;
    entry->m_entryId = createId();
    entry->m_dataType = type;
    entry->m_data = getEntryDataPool()->createConstantArray(totalSize);
    std::copy_n(str.data(), str.size(), entry->m_data.m_elems);

    if (includeNullTerminator)
    {
        entry->m_data.m_elems[str.size()] = '\0';
    }

    LOG_DEBUG(std::format("Creating global string '{}' (id: {}) (size: {}) (readonly: {})",
                          entry->m_entryId,
                          entry->m_dataType->getName(),
                          str,
                          isReadOnly),
              "MirEmitterContext");

    m_idToGlobalDataEntry.insert({ entry->m_entryId, entry });
    return entry;
}

MirGlobalDataEntry *MirEmitterContext::getGlobalDataEntryFromId(size_t entryId) const
{
    auto it = m_idToGlobalDataEntry.find(entryId);
    if (it != m_idToGlobalDataEntry.end())
        return it->second;

    return nullptr;
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

std::shared_ptr<class MirTypeTable> MirEmitterContext::getTypes() const { return m_types; }
