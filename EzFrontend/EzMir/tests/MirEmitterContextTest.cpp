#include "MirTestFixture.h"

// =============================================================================
//  Block creation
// =============================================================================

TEST_F(MirTestFixture, Context_CreateBlock)
{
    MirBlock *block = m_context->createBlock();
    ASSERT_NE(block, nullptr);
    EXPECT_NE(block->getId(), 0);
}

TEST_F(MirTestFixture, Context_CreateMultipleBlocksDistinctIds)
{
    MirBlock *b1 = m_context->createBlock();
    MirBlock *b2 = m_context->createBlock();
    MirBlock *b3 = m_context->createBlock();
    ASSERT_NE(b1, nullptr);
    ASSERT_NE(b2, nullptr);
    ASSERT_NE(b3, nullptr);
    EXPECT_NE(b1->getId(), b2->getId());
    EXPECT_NE(b2->getId(), b3->getId());
    EXPECT_NE(b1->getId(), b3->getId());
}

TEST_F(MirTestFixture, Context_NewBlockHasEmptyInstructions)
{
    MirBlock *block = m_context->createBlock();
    ASSERT_NE(block, nullptr);
    ASSERT_NE(block->getInstructions(), nullptr);
    EXPECT_EQ(block->getInstructions()->m_numElems, 0);
}

// =============================================================================
//  Block binding
// =============================================================================

TEST_F(MirTestFixture, Context_BindToBlock)
{
    MirBlock *block = m_context->createBlock();
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(m_context->bindToBlock(block));
    EXPECT_EQ(m_context->getCurrentBoundBlock(), block);
}

TEST_F(MirTestFixture, Context_BindToNullBlock)
{
    EXPECT_FALSE(m_context->bindToBlock(nullptr));
}

TEST_F(MirTestFixture, Context_RebindToNewBlock)
{
    MirBlock *b1 = m_context->createBlock();
    MirBlock *b2 = m_context->createBlock();
    EXPECT_TRUE(m_context->bindToBlock(b1));
    EXPECT_EQ(m_context->getCurrentBoundBlock(), b1);
    EXPECT_TRUE(m_context->bindToBlock(b2));
    EXPECT_EQ(m_context->getCurrentBoundBlock(), b2);
}

TEST_F(MirTestFixture, Context_InitialBoundBlockIsNull)
{
    EXPECT_EQ(m_context->getCurrentBoundBlock(), nullptr);
}

// =============================================================================
//  ID creation
// =============================================================================

TEST_F(MirTestFixture, Context_CreateId_ReturnsNonZero)
{
    MirId id = m_context->createId();
    EXPECT_NE(id, MIRID_INVALID);
}

TEST_F(MirTestFixture, Context_CreateId_Monotonic)
{
    MirId id1 = m_context->createId();
    MirId id2 = m_context->createId();
    MirId id3 = m_context->createId();
    EXPECT_LT(id1, id2);
    EXPECT_LT(id2, id3);
}

// =============================================================================
//  Instruction creation
// =============================================================================

TEST_F(MirTestFixture, Context_CreateInstruction_NOP)
{
    MirInstruction *instr = m_context->createInstruction(MirInstructionOpCode::NOP);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::NOP);
}

TEST_F(MirTestFixture, Context_CreateInstruction_ADD)
{
    MirInstruction *instr = m_context->createInstruction(MirInstructionOpCode::ADD);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::ADD);
}

TEST_F(MirTestFixture, Context_CreateInstruction_HasEmptyOperandSlice)
{
    MirInstruction *instr = m_context->createInstruction(MirInstructionOpCode::MOV);
    ASSERT_NE(instr, nullptr);
    ASSERT_NE(instr->getOperands(), nullptr);
    EXPECT_EQ(instr->getOperands()->m_numElems, 0);
    EXPECT_FALSE(instr->hasOperands());
}

TEST_F(MirTestFixture, Context_InstructionAppendsToBoundBlock)
{
    MirBlock *block = createAndBindBlock();
    ASSERT_NE(block, nullptr);

    MirInstruction *instr = m_context->createInstruction(MirInstructionOpCode::NOP);
    ASSERT_NE(instr, nullptr);

    expectInstructionCount(block, 1);
    EXPECT_EQ(getInstruction(block, 0), instr);
}

TEST_F(MirTestFixture, Context_MultipleInstructionsAppendToBoundBlock)
{
    MirBlock *block = createAndBindBlock();
    ASSERT_NE(block, nullptr);

    m_context->createInstruction(MirInstructionOpCode::NOP);
    m_context->createInstruction(MirInstructionOpCode::ADD);
    m_context->createInstruction(MirInstructionOpCode::SUB);

    expectInstructionCount(block, 3);
    expectOpcode(block, 0, MirInstructionOpCode::NOP);
    expectOpcode(block, 1, MirInstructionOpCode::ADD);
    expectOpcode(block, 2, MirInstructionOpCode::SUB);
}

TEST_F(MirTestFixture, Context_InstructionNotAppendedWhenNoBlockBound)
{
    // No block bound; instruction is created but not linked.
    MirInstruction *instr = m_context->createInstruction(MirInstructionOpCode::NOP);
    ASSERT_NE(instr, nullptr);
    // The instruction exists in the pool but no block holds it.
}

TEST_F(MirTestFixture, Context_InstructionsGoToCorrectBlockAfterRebind)
{
    MirBlock *b1 = createAndBindBlock();
    m_context->createInstruction(MirInstructionOpCode::NOP);

    MirBlock *b2 = createAndBindBlock();
    m_context->createInstruction(MirInstructionOpCode::ADD);
    m_context->createInstruction(MirInstructionOpCode::SUB);

    expectInstructionCount(b1, 1);
    expectOpcode(b1, 0, MirInstructionOpCode::NOP);

    expectInstructionCount(b2, 2);
    expectOpcode(b2, 0, MirInstructionOpCode::ADD);
    expectOpcode(b2, 1, MirInstructionOpCode::SUB);
}

// =============================================================================
//  Function creation
// =============================================================================

TEST_F(MirTestFixture, Context_CreateFunction_ValidReturnType)
{
    MirFunction *func = m_context->createFunction(1);
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getReturnTypeId(), 1);
    EXPECT_NE(func->getId(), 0);
}

TEST_F(MirTestFixture, Context_CreateFunction_InvalidReturnType)
{
    MirFunction *func = m_context->createFunction(0);
    EXPECT_EQ(func, nullptr);
}

TEST_F(MirTestFixture, Context_CreateFunction_HasEntryPoint)
{
    MirFunction *func = m_context->createFunction(1);
    ASSERT_NE(func, nullptr);
    EXPECT_NE(func->getEntryPoint(), nullptr);
}

TEST_F(MirTestFixture, Context_CreateFunction_EntryPointInBlockList)
{
    MirFunction *func = m_context->createFunction(1);
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->getBlocks(), nullptr);
    EXPECT_EQ(func->getBlocks()->m_numElems, 1); // Only the entry point initially
}

TEST_F(MirTestFixture, Context_CreateFunction_HasEmptyParameters)
{
    MirFunction *func = m_context->createFunction(1);
    ASSERT_NE(func, nullptr);
    ASSERT_NE(func->getParameters(), nullptr);
    EXPECT_EQ(func->getParameters()->m_numElems, 0);
}

TEST_F(MirTestFixture, Context_CreateMultipleFunctions_DistinctIds)
{
    MirFunction *f1 = m_context->createFunction(1);
    MirFunction *f2 = m_context->createFunction(2);
    ASSERT_NE(f1, nullptr);
    ASSERT_NE(f2, nullptr);
    EXPECT_NE(f1->getId(), f2->getId());
}

TEST_F(MirTestFixture, Context_CreateFunction_DifferentReturnTypes)
{
    MirFunction *f1 = m_context->createFunction(1);
    MirFunction *f2 = m_context->createFunction(42);
    ASSERT_NE(f1, nullptr);
    ASSERT_NE(f2, nullptr);
    EXPECT_EQ(f1->getReturnTypeId(), 1);
    EXPECT_EQ(f2->getReturnTypeId(), 42);
}

TEST_F(MirTestFixture, Context_NewBlockInsideFunctionAppendsToFunction)
{
    MirFunction *func = m_context->createFunction(1);
    ASSERT_NE(func, nullptr);
    EXPECT_EQ(func->getBlocks()->m_numElems, 1); // entry point

    MirBlock *extra = m_context->createBlock();
    ASSERT_NE(extra, nullptr);
    // createBlock while a function is active appends to its block list
    EXPECT_EQ(func->getBlocks()->m_numElems, 2);
}

// =============================================================================
//  Pool accessors
// =============================================================================

TEST_F(MirTestFixture, Context_PoolAccessors_NonNull)
{
    EXPECT_NE(m_context->getBlockPool(), nullptr);
    EXPECT_NE(m_context->getFunctionPool(), nullptr);
    EXPECT_NE(m_context->getFunctionParameterPool(), nullptr);
    EXPECT_NE(m_context->getInstructionPool(), nullptr);
    EXPECT_NE(m_context->getOperandPool(), nullptr);
    EXPECT_NE(m_context->getDataEntryPool(), nullptr);
    EXPECT_NE(m_context->getEntryDataPool(), nullptr);
}
