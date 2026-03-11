#include "Emitter/MirEmitterContext.h"

MirEmitterContext::MirEmitterContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                                     const std::shared_ptr<SourceManager> &sourceManager) :
    m_currentId(1), m_currentBoundBlock(nullptr), m_currentBoundFunction(nullptr),
    ErrorEmitter(errorCollector, sourceManager)
{
    m_functionList = m_functionPool.createSlice<MirFunction>();
    m_typeList = m_typePool.createSlice<MirType>();
}

bool MirEmitterContext::bindToBlock(MirBlock *block)
{
    if (!block)
        return false;

    m_currentBoundBlock = block;
    return true;
}

bool MirEmitterContext::doesTypeExist(size_t typeId) const { return m_idToTypeMap.contains(typeId); }

bool MirEmitterContext::doesTypeExist(const std::string_view &typeName) const { return m_typeNames.contains(typeName); }

MirBlock *MirEmitterContext::createBlock()
{
    MirBlock *block = m_blockPool.create<MirBlock>(createId(), m_instructionPool.createSlice<MirInstruction>());

    if (m_currentBoundFunction)
    {
        // If we are in a function, append the new block to its blocks too.
        m_blockPool.appendToSlice(m_currentBoundFunction->getBlocks(), block);
    }

    return block;
}

MirBlock *MirEmitterContext::getCurrentBoundBlock() const { return m_currentBoundBlock; }

MirId MirEmitterContext::createId() { return m_currentId++; }

MirFunction *MirEmitterContext::createFunction(size_t returnTypeId)
{
    if (returnTypeId == 0)
    {
        emitError(ErrorSeverity::Fatal,
                  "Could not create function because return type ID is null",
                  "MirEmitterContext::createFunction");
        return nullptr;
    }

    TypedPoolSlice<MirBlock> *functionBlockList = m_blockPool.createSlice<MirBlock>();
    TypedPoolSlice<MirOperand> *functionParameterList = m_functionParameterPool.createSlice<MirOperand>();

    /*
     * We can't use createBlock here because we need to create the block WITHOUT appending it to the current function,
     * if any.
     */
    MirBlock *entryPoint = m_blockPool.create<MirBlock>(createId(), m_instructionPool.createSlice<MirInstruction>());
    m_blockPool.appendToSlice(functionBlockList, entryPoint);

    MirFunction *func = m_functionPool.createAndAppendToSlice<MirFunction>(m_functionList,
                                                                           entryPoint,
                                                                           createId(),
                                                                           returnTypeId,
                                                                           functionBlockList,
                                                                           functionParameterList);

    m_currentBoundFunction = func;
    return func;
}

MirInstruction *MirEmitterContext::createInstruction(MirInstructionOpCode opcode)
{
    MirInstruction *instr = m_instructionPool.create<MirInstruction>(opcode, m_operandPool.createSlice<MirOperand>());

    if (m_currentBoundBlock)
    {
        // Automatically bind this instruction to the latest binded block.
        m_instructionPool.appendToSlice(m_currentBoundBlock->getInstructions(), instr);
    }

    return instr;
}

MirType *MirEmitterContext::createType(MirTypeKind kind, TypedPoolSlice<MirType> *types, const std::string_view &name)
{
    if (name.empty())
    {
        emitError(ErrorSeverity::Fatal, "Could not create type because name is empty", "MirEmitterContext::createType");
        return nullptr;
    }

    MirType *type = m_typePool.create<MirType>(kind, createId(), types, name);

    m_typePool.appendToSlice(m_typeList, type);
    m_idToTypeMap.insert({ type->getId(), type });

    return type;
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

TypedPoolSlice<MirFunction> *MirEmitterContext::getFunctionList() const { return m_functionList; }
