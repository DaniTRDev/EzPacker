#include "Emitter/MirEmitterContext.h"
#include "Type/MirTypeTable.h"

MirEmitterContext::MirEmitterContext(const std::shared_ptr<DiagnosticCollector> &diagCollector) :
    m_currentId(1), m_globalResource(), m_functionResource(&m_globalResource), m_functions(&m_globalResource),
    m_globalData(&m_globalResource), m_diagCollector(diagCollector)
{
}

const InsertState &MirEmitterContext::getInsertPoint() const { return m_insertState; }

MirBlock *MirEmitterContext::getCurrentBlock() const { return m_insertState.block; }

MirBlock *MirEmitterContext::createBlock()
{
    std::pmr::polymorphic_allocator<MirBlock> blockAlloc(getFuncAllocator());
    MirBlock *newBlock = blockAlloc.allocate(1); // Allocate raw aligned memory.

    blockAlloc.construct(newBlock,
                         createId(),
                         std::pmr::vector<MirInstruction>(getFuncAllocator())); // Call constructor.

    return newBlock;
}

MirId MirEmitterContext::createId() { return m_currentId++; }

MirInstruction *MirEmitterContext::createInstruction(MirInstructionOpCode opcode)
{
    std::pmr::polymorphic_allocator<MirInstruction> instrAlloc(getFuncAllocator());
    MirInstruction *newInstr = instrAlloc.allocate(1); // Allocate raw aligned memory.

    instrAlloc.construct(newInstr, opcode, std::pmr::vector<MirOperand>(getFuncAllocator())); // Call constructor.
    return newInstr;
}

MirInstruction *MirEmitterContext::createInstruction(MirInstructionOpCode opcode,
                                                     const std::initializer_list<MirOperand *> &operands)
{
    MirInstruction *instr = createInstruction(opcode);
    auto operandList = instr->getOperands();

    for (auto &op : operands)
    {
        operandList->push_back(op);
    }

    return instr;
}

MirFunction *MirEmitterContext::createFunc(const std::string &name,
                                           MirType *returnType,
                                           const std::initializer_list<MirRegister *> &params)
{
    std::pmr::memory_resource *arena = getFuncAllocator();
    std::pmr::list<MirBlock *> blockList(arena);
    std::pmr::list<MirRegister *> operandList(arena);

    blockList.push_back(createBlock());
    operandList.insert(operandList.begin(), params.begin(), params.end());

    std::pmr::string copiedName(name.c_str(), getGlobalAllocator());

    return m_functions.back();
}

void MirEmitterContext::setInsertPoint(MirBlock *block)
{
    m_insertState.block = block;
    m_insertState.mode = InsertMode::Append;
    m_insertState.iterator = {}; // Clear iterator
}

void MirEmitterContext::setInsertPoint(MirBlock *block, std::pmr::list<MirInstruction>::iterator insertBeforeIt)
{
    m_insertState.block = block;
    m_insertState.mode = InsertMode::InsertBefore;
    m_insertState.iterator = insertBeforeIt;
}

std::pmr::monotonic_buffer_resource *MirEmitterContext::getGlobalAllocator() { return &m_globalResource; }

std::pmr::monotonic_buffer_resource *MirEmitterContext::getFuncAllocator() { return &m_functionResource; }
