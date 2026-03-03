#include "MirTestFixture.h"

// =============================================================================
//  Emitter – basic emit (no operands)
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitNOP)
{
    MirBlock *block = createAndBindBlock();
    ASSERT_NE(block, nullptr);

    MirInstruction *instr = m_emitter->emit(MirInstructionOpCode::NOP);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::NOP);
    expectInstructionCount(block, 1);
}

TEST_F(MirTestFixture, Emitter_EmitHALT)
{
    MirBlock *block = createAndBindBlock();
    ASSERT_NE(block, nullptr);

    MirInstruction *instr = m_emitter->emit(MirInstructionOpCode::HALT);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::HALT);
}

// =============================================================================
//  Emitter – typed emit helpers (2-operand)
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitADD_RegReg)
{
    MirBlock *block = createAndBindBlock();

    MirOperand op1 = MirRegister{ 1, 4 };
    MirOperand op2 = MirRegister{ 2, 4 };

    MirInstruction *instr = m_emitter->emitADD(op1, op2);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::ADD);

    expectOperandCount(instr, 2);
    expectOperandType(instr, 0, MirOperandType::Register);
    expectOperandType(instr, 1, MirOperandType::Register);
}

TEST_F(MirTestFixture, Emitter_EmitSUB_RegImm)
{
    MirBlock *block = createAndBindBlock();

    MirOperand op1 = MirRegister{ 1, 4 };
    MirOperand op2 = MirInteger{ 42 };

    MirInstruction *instr = m_emitter->emitSUB(op1, op2);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::SUB);

    expectOperandCount(instr, 2);
    expectOperandType(instr, 0, MirOperandType::Register);
    expectOperandType(instr, 1, MirOperandType::Integer);
}

TEST_F(MirTestFixture, Emitter_EmitMOV_RegReg)
{
    MirBlock *block = createAndBindBlock();

    MirOperand dst = MirRegister{ 1, 8 };
    MirOperand src = MirRegister{ 2, 8 };

    MirInstruction *instr = m_emitter->emitMOV(dst, src);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::MOV);
    expectOperandCount(instr, 2);
}

TEST_F(MirTestFixture, Emitter_EmitMOV_RegImm)
{
    MirBlock *block = createAndBindBlock();

    MirOperand dst = MirRegister{ 1, 4 };
    MirOperand src = MirInteger{ 100 };

    MirInstruction *instr = m_emitter->emitMOV(dst, src);
    ASSERT_NE(instr, nullptr);
    expectOperandType(instr, 0, MirOperandType::Register);
    expectOperandType(instr, 1, MirOperandType::Integer);
}

TEST_F(MirTestFixture, Emitter_EmitMUL_RegReg)
{
    createAndBindBlock();

    MirInstruction *instr = m_emitter->emitMUL(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::MUL);
    expectOperandCount(instr, 2);
}

TEST_F(MirTestFixture, Emitter_EmitDIV_RegReg)
{
    createAndBindBlock();

    MirInstruction *instr = m_emitter->emitDIV(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::DIV);
}

// =============================================================================
//  Emitter – bitwise instructions
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitAND)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitAND(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirInteger{ 0xFF } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::AND);
    expectOperandCount(instr, 2);
}

TEST_F(MirTestFixture, Emitter_EmitOR)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitOR(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirInteger{ 0x10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::OR);
}

TEST_F(MirTestFixture, Emitter_EmitXOR)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitXOR(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::XOR);
}

TEST_F(MirTestFixture, Emitter_EmitSHL)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitSHL(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirInteger{ 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::SHL);
}

TEST_F(MirTestFixture, Emitter_EmitSHR)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitSHR(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirInteger{ 2 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::SHR);
}

TEST_F(MirTestFixture, Emitter_EmitSAR)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitSAR(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirInteger{ 3 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::SAR);
}

// =============================================================================
//  Emitter – unary instructions
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitNEG)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitNEG(MirOperand{ MirRegister{ 1, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::NEG);
    expectOperandCount(instr, 1);
}

TEST_F(MirTestFixture, Emitter_EmitNOT)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitNOT(MirOperand{ MirRegister{ 1, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::NOT);
    expectOperandCount(instr, 1);
}

// =============================================================================
//  Emitter – comparison & test
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitCMP)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitCMP(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::CMP);
    expectOperandCount(instr, 2);
}

TEST_F(MirTestFixture, Emitter_EmitTEST)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitTEST(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::TEST);
}

// =============================================================================
//  Emitter – control flow (jumps)
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitJMP)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJMP(MirOperand{ MirReference{ 42 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JMP);
    expectOperandCount(instr, 1);
    expectOperandType(instr, 0, MirOperandType::Reference);
}

TEST_F(MirTestFixture, Emitter_EmitJE)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJE(MirOperand{ MirReference{ 10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JE);
    expectOperandType(instr, 0, MirOperandType::Reference);
}

TEST_F(MirTestFixture, Emitter_EmitJNE)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJNE(MirOperand{ MirReference{ 10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JNE);
}

TEST_F(MirTestFixture, Emitter_EmitJG)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJG(MirOperand{ MirReference{ 10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JG);
}

TEST_F(MirTestFixture, Emitter_EmitJGE)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJGE(MirOperand{ MirReference{ 10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JGE);
}

TEST_F(MirTestFixture, Emitter_EmitJL)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJL(MirOperand{ MirReference{ 10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JL);
}

TEST_F(MirTestFixture, Emitter_EmitJLE)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJLE(MirOperand{ MirReference{ 10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JLE);
}

TEST_F(MirTestFixture, Emitter_EmitJA)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJA(MirOperand{ MirReference{ 10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JA);
}

TEST_F(MirTestFixture, Emitter_EmitJB)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJB(MirOperand{ MirReference{ 10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JB);
}

// =============================================================================
//  Emitter – CALL / RET
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitCALL)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitCALL(MirOperand{ MirReference{ 99 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::CALL);
    expectOperandType(instr, 0, MirOperandType::Reference);
}

TEST_F(MirTestFixture, Emitter_EmitRET)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitRET(MirOperand{ MirRegister{ 1, 8 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::RET);
    expectOperandType(instr, 0, MirOperandType::Register);
}

// =============================================================================
//  Emitter – memory instructions
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitLOAD)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitLOAD(MirOperand{ MirRegister{ 1, 8 } }, MirOperand{ MirMemory{ 2, 0, 0, 0 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::LOAD);
    expectOperandType(instr, 0, MirOperandType::Register);
    expectOperandType(instr, 1, MirOperandType::Memory);
}

TEST_F(MirTestFixture, Emitter_EmitSTORE)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitSTORE(MirOperand{ MirMemory{ 1, 0, 0, 0x10 } }, MirOperand{ MirRegister{ 2, 8 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::STORE);
    expectOperandType(instr, 0, MirOperandType::Memory);
    expectOperandType(instr, 1, MirOperandType::Register);
}

TEST_F(MirTestFixture, Emitter_EmitLEA)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitLEA(MirOperand{ MirRegister{ 1, 8 } }, MirOperand{ MirMemory{ 2, 3, 4, 0x10 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::LEA);
    expectOperandCount(instr, 2);
}

// =============================================================================
//  Emitter – type cast instructions
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitTRUNC)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitTRUNC(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 8 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::TRUNC);
}

TEST_F(MirTestFixture, Emitter_EmitZEXT)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitZEXT(MirOperand{ MirRegister{ 1, 8 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::ZEXT);
}

TEST_F(MirTestFixture, Emitter_EmitSEXT)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitSEXT(MirOperand{ MirRegister{ 1, 8 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::SEXT);
}

TEST_F(MirTestFixture, Emitter_EmitBITCAST)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitBITCAST(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::BITCAST);
}

// =============================================================================
//  Emitter – CREATE
// =============================================================================

TEST_F(MirTestFixture, Emitter_EmitCREATE)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitCREATE(MirOperand{ MirRegister{ 1, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::CREATE);
    expectOperandCount(instr, 1);
}

// =============================================================================
//  Emitter – createRegister
// =============================================================================

TEST_F(MirTestFixture, Emitter_CreateRegister)
{
    MirRegister reg = m_emitter->createRegister(8);
    EXPECT_NE(reg.m_id, 0);
    EXPECT_EQ(reg.m_size, 8);
}

TEST_F(MirTestFixture, Emitter_CreateRegister_DistinctIds)
{
    MirRegister r1 = m_emitter->createRegister(4);
    MirRegister r2 = m_emitter->createRegister(4);
    MirRegister r3 = m_emitter->createRegister(8);
    EXPECT_NE(r1.m_id, r2.m_id);
    EXPECT_NE(r2.m_id, r3.m_id);
}

TEST_F(MirTestFixture, Emitter_CreateRegister_VariousSizes)
{
    MirRegister r1 = m_emitter->createRegister(1);
    MirRegister r2 = m_emitter->createRegister(2);
    MirRegister r4 = m_emitter->createRegister(4);
    MirRegister r8 = m_emitter->createRegister(8);
    EXPECT_EQ(r1.m_size, 1);
    EXPECT_EQ(r2.m_size, 2);
    EXPECT_EQ(r4.m_size, 4);
    EXPECT_EQ(r8.m_size, 8);
}

// =============================================================================
//  Emitter – pushOperandToInstruction
// =============================================================================

TEST_F(MirTestFixture, Emitter_PushOperandToInstruction)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emit(MirInstructionOpCode::NOP);
    ASSERT_NE(instr, nullptr);
    EXPECT_FALSE(instr->hasOperands());

    m_emitter->pushOperandToInstruction(instr, MirOperand{ MirRegister{ 5, 4 } });
    EXPECT_TRUE(instr->hasOperands());
    expectOperandCount(instr, 1);
    expectOperandType(instr, 0, MirOperandType::Register);
}

TEST_F(MirTestFixture, Emitter_PushMultipleOperands)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emit(MirInstructionOpCode::NOP);
    ASSERT_NE(instr, nullptr);

    m_emitter->pushOperandToInstruction(instr, MirOperand{ MirRegister{ 1, 4 } });
    m_emitter->pushOperandToInstruction(instr, MirOperand{ MirInteger{ 42 } });
    m_emitter->pushOperandToInstruction(instr, MirOperand{ MirDouble{ 3.14 } });

    expectOperandCount(instr, 3);
    expectOperandType(instr, 0, MirOperandType::Register);
    expectOperandType(instr, 1, MirOperandType::Integer);
    expectOperandType(instr, 2, MirOperandType::Double);
}

// =============================================================================
//  Emitter – getContext
// =============================================================================

TEST_F(MirTestFixture, Emitter_GetContext)
{
    EXPECT_EQ(m_emitter->getContext(), m_context.get());
}

// =============================================================================
//  Emitter – sequence of emissions in same block
// =============================================================================

TEST_F(MirTestFixture, Emitter_SequenceInBlock)
{
    MirBlock *block = createAndBindBlock();

    m_emitter->emitNOP();
    m_emitter->emitADD(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirInteger{ 10 } });
    m_emitter->emitCMP(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    m_emitter->emitJE(MirOperand{ MirReference{ 99 } });

    expectInstructionCount(block, 4);
    expectOpcode(block, 0, MirInstructionOpCode::NOP);
    expectOpcode(block, 1, MirInstructionOpCode::ADD);
    expectOpcode(block, 2, MirInstructionOpCode::CMP);
    expectOpcode(block, 3, MirInstructionOpCode::JE);
}

// =============================================================================
//  Emitter – emissions across multiple blocks
// =============================================================================

TEST_F(MirTestFixture, Emitter_MultipleBlocks)
{
    MirBlock *b1 = createAndBindBlock();
    m_emitter->emitNOP();
    m_emitter->emitJMP(MirOperand{ MirReference{ 10 } });

    MirBlock *b2 = createAndBindBlock();
    m_emitter->emitADD(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    m_emitter->emitHALT();

    expectInstructionCount(b1, 2);
    expectInstructionCount(b2, 2);

    expectOpcode(b1, 0, MirInstructionOpCode::NOP);
    expectOpcode(b1, 1, MirInstructionOpCode::JMP);
    expectOpcode(b2, 0, MirInstructionOpCode::ADD);
    expectOpcode(b2, 1, MirInstructionOpCode::HALT);
}

// =============================================================================
//  Operand value verification
// =============================================================================

TEST_F(MirTestFixture, Emitter_OperandValues_Register)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitMOV(MirOperand{ MirRegister{ 7, 8 } }, MirOperand{ MirRegister{ 13, 4 } });
    ASSERT_NE(instr, nullptr);

    MirOperand *op0 = getOperand(instr, 0);
    ASSERT_NE(op0, nullptr);
    ASSERT_NE(op0->getRegister(), nullptr);
    EXPECT_EQ(op0->getRegister()->m_id, 7);
    EXPECT_EQ(op0->getRegister()->m_size, 8);

    MirOperand *op1 = getOperand(instr, 1);
    ASSERT_NE(op1, nullptr);
    ASSERT_NE(op1->getRegister(), nullptr);
    EXPECT_EQ(op1->getRegister()->m_id, 13);
    EXPECT_EQ(op1->getRegister()->m_size, 4);
}

TEST_F(MirTestFixture, Emitter_OperandValues_Integer)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitADD(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirInteger{ -42 } });
    ASSERT_NE(instr, nullptr);

    MirOperand *op1 = getOperand(instr, 1);
    ASSERT_NE(op1, nullptr);
    ASSERT_NE(op1->getInteger(), nullptr);
    EXPECT_EQ(op1->getInteger()->m_value, -42);
}

TEST_F(MirTestFixture, Emitter_OperandValues_Double)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emit(MirInstructionOpCode::NOP);
    m_emitter->pushOperandToInstruction(instr, MirOperand{ MirDouble{ 3.14159 } });

    MirOperand *op0 = getOperand(instr, 0);
    ASSERT_NE(op0, nullptr);
    ASSERT_NE(op0->getDouble(), nullptr);
    EXPECT_DOUBLE_EQ(op0->getDouble()->m_value, 3.14159);
}

TEST_F(MirTestFixture, Emitter_OperandValues_Memory)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitLOAD(MirOperand{ MirRegister{ 1, 8 } }, MirOperand{ MirMemory{ 5, 6, 2, 0x100 } });
    ASSERT_NE(instr, nullptr);

    MirOperand *op1 = getOperand(instr, 1);
    ASSERT_NE(op1, nullptr);
    ASSERT_NE(op1->getMemory(), nullptr);
    EXPECT_EQ(op1->getMemory()->m_baseRegId, 5);
    EXPECT_EQ(op1->getMemory()->m_indexRegId, 6);
    EXPECT_EQ(op1->getMemory()->m_scale, 2);
    EXPECT_EQ(op1->getMemory()->m_offset, 0x100);
}

TEST_F(MirTestFixture, Emitter_OperandValues_Reference)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJMP(MirOperand{ MirReference{ 42 } });
    ASSERT_NE(instr, nullptr);

    MirOperand *op0 = getOperand(instr, 0);
    ASSERT_NE(op0, nullptr);
    ASSERT_NE(op0->getReference(), nullptr);
    EXPECT_EQ(op0->getReference()->m_refId, 42);
}

TEST_F(MirTestFixture, Emitter_OperandValues_BigInteger)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emit(MirInstructionOpCode::NOP);
    m_emitter->pushOperandToInstruction(instr, MirOperand{ MirBigInteger{ 999 } });

    MirOperand *op0 = getOperand(instr, 0);
    ASSERT_NE(op0, nullptr);
    ASSERT_NE(op0->getBigInteger(), nullptr);
    EXPECT_EQ(op0->getBigInteger()->m_constantId, 999);
}

// =============================================================================
//  Instruction metadata
// =============================================================================

TEST_F(MirTestFixture, Emitter_InstructionMetadata_NOP)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitNOP();
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getMetadata().m_operandCount, 0);
    EXPECT_EQ(instr->getMetadata().m_flags, MirInstructionFlags::None);
}

TEST_F(MirTestFixture, Emitter_InstructionMetadata_ADD)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitADD(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getMetadata().m_operandCount, 2);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::Op1_Read);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::Op1_Write);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::Op2_Read);
}

TEST_F(MirTestFixture, Emitter_InstructionMetadata_JMP_IsTerminator)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJMP(MirOperand{ MirReference{ 1 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::IsTerminator);
}

TEST_F(MirTestFixture, Emitter_InstructionMetadata_JE_IsBranch)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitJE(MirOperand{ MirReference{ 1 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::IsTerminator);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::IsBranch);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::ReadsCPUFlags);
}

TEST_F(MirTestFixture, Emitter_InstructionMetadata_CALL_IsCall)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitCALL(MirOperand{ MirReference{ 1 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::IsCall);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::HasSideEffect);
}

TEST_F(MirTestFixture, Emitter_InstructionMetadata_RET_IsReturn)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitRET(MirOperand{ MirRegister{ 1, 8 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::IsReturn);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::IsTerminator);
}

TEST_F(MirTestFixture, Emitter_InstructionMetadata_CMP_WritesCPUFlags)
{
    createAndBindBlock();
    MirInstruction *instr = m_emitter->emitCMP(MirOperand{ MirRegister{ 1, 4 } }, MirOperand{ MirRegister{ 2, 4 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::WritesCPUFlags);
}

TEST_F(MirTestFixture, Emitter_InstructionMetadata_LOAD_ReadsMemory)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitLOAD(MirOperand{ MirRegister{ 1, 8 } }, MirOperand{ MirMemory{ 2, 0, 0, 0 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::ReadsMemory);
}

TEST_F(MirTestFixture, Emitter_InstructionMetadata_STORE_WritesMemory)
{
    createAndBindBlock();
    MirInstruction *instr =
            m_emitter->emitSTORE(MirOperand{ MirMemory{ 1, 0, 0, 0 } }, MirOperand{ MirRegister{ 2, 8 } });
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->getFlags() & MirInstructionFlags::WritesMemory);
}
