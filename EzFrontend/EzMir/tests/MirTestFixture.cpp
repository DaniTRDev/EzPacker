#include "MirTestFixture.h"

void MirTestFixture::SetUp()
{
    m_errorCollector = std::make_shared<ErrorCollector>();
    m_sourceManager = std::make_shared<SourceManager>("");
    m_context = std::make_shared<MirEmitterContext>(m_errorCollector, m_sourceManager);
    m_emitter = std::make_shared<MirEmitter>(m_context.get());
    m_globalDataEmitter = std::make_shared<MirGlobalDataEmitter>(m_context.get());
    m_errorCollector->beginScope();

    Test::SetUp();
}

void MirTestFixture::TearDown()
{
    m_errorCollector->endScope(ErrorAction::Commit);
    m_globalDataEmitter.reset();
    m_emitter.reset();
    m_context.reset();
    m_sourceManager.reset();
    m_errorCollector.reset();

    Test::TearDown();
}

// ──────────────────────────────────────────────────────────────
//  Block helpers
// ──────────────────────────────────────────────────────────────

MirBlock *MirTestFixture::createBlock() { return m_context->createBlock(); }

MirBlock *MirTestFixture::createAndBindBlock()
{
    MirBlock *block = m_context->createBlock();
    if (block)
        m_context->bindToBlock(block);
    return block;
}

// ──────────────────────────────────────────────────────────────
//  Instruction helpers
// ──────────────────────────────────────────────────────────────

MirInstruction *MirTestFixture::getInstruction(MirBlock *block, size_t index)
{
    if (!block)
        return nullptr;
    auto instrs = block->getInstructions();
    if (!instrs || index >= instrs->m_numElems)
        return nullptr;
    return instrs->get<MirInstruction>(index);
}

size_t MirTestFixture::getInstructionCount(MirBlock *block)
{
    if (!block)
        return 0;
    auto instrs = block->getInstructions();
    return instrs ? instrs->m_numElems : 0;
}

// ──────────────────────────────────────────────────────────────
//  Operand helpers
// ──────────────────────────────────────────────────────────────

MirOperand *MirTestFixture::getOperand(MirInstruction *instr, size_t index)
{
    if (!instr)
        return nullptr;
    auto ops = instr->getOperands();
    if (!ops || index >= ops->m_numElems)
        return nullptr;
    return ops->get<MirOperand>(index);
}

size_t MirTestFixture::getOperandCount(MirInstruction *instr)
{
    if (!instr)
        return 0;
    auto ops = instr->getOperands();
    return ops ? ops->m_numElems : 0;
}

// ──────────────────────────────────────────────────────────────
//  Assertion helpers
// ──────────────────────────────────────────────────────────────

void MirTestFixture::expectInstructionCount(MirBlock *block, size_t expected)
{
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(getInstructionCount(block), expected);
}

MirInstruction *MirTestFixture::expectOpcode(MirBlock *block, size_t instrIdx, MirInstructionOpCode expected)
{
    MirInstruction *instr = getInstruction(block, instrIdx);
    EXPECT_NE(instr, nullptr);
    if (instr)
        EXPECT_EQ(instr->getOpCode(), expected);
    return instr;
}

void MirTestFixture::expectOperandCount(MirInstruction *instr, size_t expected)
{
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(getOperandCount(instr), expected);
}

MirOperand *MirTestFixture::expectOperandType(MirInstruction *instr, size_t opIdx, MirOperandType expected)
{
    MirOperand *op = getOperand(instr, opIdx);
    EXPECT_NE(op, nullptr);
    if (op)
        EXPECT_EQ(op->getType(), expected);
    return op;
}
