#include "InstructionSemanticCheckerVisitorTestFixture.h"

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestValidInstruction_AddRegReg)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    HighLevelMirInstruction instr(HighLevelMirOpCode::ADD, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createReg(1, 32));
    instr.addOperand(HighLevelMirInstructionOperand::createReg(2, 32));
    
    block->m_instructions.push_back(instr);
    
    HighLevelMirInstruction instrSub(HighLevelMirOpCode::SUB, {sourceRef});
    instrSub.addOperand(HighLevelMirInstructionOperand::createReg(1, 32));
    instrSub.addOperand(HighLevelMirInstructionOperand::createReg(2, 32));
    
    block->m_instructions.clear();
    block->m_instructions.push_back(instrSub);
    
    EXPECT_TRUE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestValidInstruction_AddRegImm)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    // ADD uses Op2_MustBeRegOrImm which sets Op2_MustBeImm.
    // So ADD %r1, 123 should be valid.
    
    HighLevelMirInstruction instr(HighLevelMirOpCode::ADD, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createReg(1, 32));
    
    HighLevelMirInstruction instrShl(HighLevelMirOpCode::SHL, {sourceRef});
    instrShl.addOperand(HighLevelMirInstructionOperand::createReg(1, 32));
    instrShl.addOperand(HighLevelMirInstructionOperand::createIntegerImm(1));
    
    block->m_instructions.push_back(instrShl);
    
    EXPECT_TRUE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestInvalidInstruction_Op1NotReg)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    // NEG: Op1_Read | Op1_Write | Op1_MustBeReg | WritesCPUFlags
    // Op1 must be Reg.
    
    HighLevelMirInstruction instr(HighLevelMirOpCode::NEG, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createIntegerImm(123));
    
    block->m_instructions.push_back(instr);
    
    EXPECT_FALSE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestInvalidInstruction_SizeMismatch)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    // SUB has SizeMatch.
    
    HighLevelMirInstruction instr(HighLevelMirOpCode::SUB, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createReg(1, 32));
    instr.addOperand(HighLevelMirInstructionOperand::createReg(2, 64));
    
    block->m_instructions.push_back(instr);
    
    EXPECT_FALSE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestValidInstruction_MovRegMem)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    HighLevelMirInstruction instr(HighLevelMirOpCode::LOAD, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createReg(1, 64));
    instr.addOperand(HighLevelMirInstructionOperand::createMem(64, 2, 0, 0, 0));
    
    block->m_instructions.push_back(instr);
    
    EXPECT_TRUE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestInvalidInstruction_LoadRegReg)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    // LOAD expects Op2 to be Mem.
    
    HighLevelMirInstruction instr(HighLevelMirOpCode::LOAD, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createReg(1, 64));
    instr.addOperand(HighLevelMirInstructionOperand::createReg(2, 64));
    
    block->m_instructions.push_back(instr);
    
    EXPECT_FALSE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestValidInstruction_StoreMemReg)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    // STORE: IsStore
    // IsStore = Op1_Read | Op1_MustBeMem | Op2_Read | WritesMemory ...
    // Op1 must be Mem.
    
    HighLevelMirInstruction instr(HighLevelMirOpCode::STORE, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createMem(64, 1, 0, 0, 0));
    instr.addOperand(HighLevelMirInstructionOperand::createReg(2, 64));
    
    block->m_instructions.push_back(instr);
    
    EXPECT_TRUE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestInvalidInstruction_StoreRegReg)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    // STORE expects Op1 to be Mem.
    
    HighLevelMirInstruction instr(HighLevelMirOpCode::STORE, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createReg(1, 64));
    instr.addOperand(HighLevelMirInstructionOperand::createReg(2, 64));
    
    block->m_instructions.push_back(instr);
    
    EXPECT_FALSE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestTruncationMismatch)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    // TRUNC: Op1_Write | Op1_MustBeReg | Op2_Read | DestSmaller
    // Dest (Op1) must be smaller than Src (Op2).
    
    // Case 1: Dest > Src (Invalid)
    HighLevelMirInstruction instr(HighLevelMirOpCode::TRUNC, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createReg(1, 64));
    instr.addOperand(HighLevelMirInstructionOperand::createReg(2, 32));
    
    block->m_instructions.push_back(instr);
    EXPECT_FALSE(runVisitor(block));
    
    // Case 2: Dest == Src (Invalid)
    block->m_instructions.clear();
    HighLevelMirInstruction instr2(HighLevelMirOpCode::TRUNC, {sourceRef});
    instr2.addOperand(HighLevelMirInstructionOperand::createReg(1, 32));
    instr2.addOperand(HighLevelMirInstructionOperand::createReg(2, 32));
    block->m_instructions.push_back(instr2);
    EXPECT_FALSE(runVisitor(block));
    
    // Case 3: Dest < Src (Valid)
    block->m_instructions.clear();
    HighLevelMirInstruction instr3(HighLevelMirOpCode::TRUNC, {sourceRef});
    instr3.addOperand(HighLevelMirInstructionOperand::createReg(1, 32));
    instr3.addOperand(HighLevelMirInstructionOperand::createReg(2, 64));
    block->m_instructions.push_back(instr3);
    EXPECT_TRUE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestExtensionMismatch)
{
    auto block = HighLevelMirBlock::create(0, nullptr);
    auto sourceRef = createDummySourceRef();
    
    // ZEXT: Op1_Write | Op1_MustBeReg | Op2_Read | DestLarger
    // Dest (Op1) must be larger than Src (Op2).
    
    // Case 1: Dest < Src (Invalid)
    HighLevelMirInstruction instr(HighLevelMirOpCode::ZEXT, {sourceRef});
    instr.addOperand(HighLevelMirInstructionOperand::createReg(1, 32));
    instr.addOperand(HighLevelMirInstructionOperand::createReg(2, 64));
    
    block->m_instructions.push_back(instr);
    EXPECT_FALSE(runVisitor(block));
    
    // Case 2: Dest > Src (Valid)
    block->m_instructions.clear();
    HighLevelMirInstruction instr2(HighLevelMirOpCode::ZEXT, {sourceRef});
    instr2.addOperand(HighLevelMirInstructionOperand::createReg(1, 64));
    instr2.addOperand(HighLevelMirInstructionOperand::createReg(2, 32));
    block->m_instructions.push_back(instr2);
    EXPECT_TRUE(runVisitor(block));
}

TEST_F(InstructionSemanticCheckerVisitorTestFixture, TestScopeTraversal)
{
    // Test that it visits linked blocks.
    auto block1 = HighLevelMirBlock::create(1, nullptr);
    auto block2 = HighLevelMirBlock::create(2, block1);
    block1->m_next = block2;
    
    auto sourceRef = createDummySourceRef();
    
    // Block 1: Valid (NOP)
    HighLevelMirInstruction instr1(HighLevelMirOpCode::NOP, {sourceRef});
    block1->m_instructions.push_back(instr1);
    
    // Block 2: Invalid (SUB Size Mismatch)
    HighLevelMirInstruction instr2(HighLevelMirOpCode::SUB, {sourceRef});
    instr2.addOperand(HighLevelMirInstructionOperand::createReg(1, 64));
    instr2.addOperand(HighLevelMirInstructionOperand::createReg(2, 32));
    block2->m_instructions.push_back(instr2);
    
    // Running on block1 should eventually visit block2 and fail.
    EXPECT_FALSE(runVisitor(block1));
}
