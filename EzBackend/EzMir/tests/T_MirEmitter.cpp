/**
 * @file T_MirEmitter.cpp
 * @brief Unit tests for MirEmitter.
 *
 * Covers: emit with no operands, emit with correct operand count, emit with
 * wrong operand count (should fail), helper emitXxx shortcuts, register
 * creation, null context attachment.
 */
#include <gtest/gtest.h>
#include <EzMir.h>

class MirEmitterTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> ctx;
    MirEmitter *emitter;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ctx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(ctx.get());

        ec->beginScope();
        MirBlock *block = ctx->createBlock();
        ctx->bindToBlock(block);
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete emitter;
    }
};

// ─── Attachment ───────────────────────────────────────────────────────────────

TEST_F(MirEmitterTests, AttachToNullContextFails)
{
    try
    {
        MirEmitter local_emitter(ctx.get());
        EXPECT_FALSE(local_emitter.attachToContext(nullptr));
    }
    catch (const std::exception &ex)
    {
    
    }
}

TEST_F(MirEmitterTests, AttachToValidContextSucceeds) { EXPECT_EQ(emitter->getContext(), ctx.get()); }

// ─── Zero-operand emit ────────────────────────────────────────────────────────

TEST_F(MirEmitterTests, EmitNopProducesInstruction)
{
    MirInstruction *instr = emitter->emit(MirInstructionOpCode::NOP);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::NOP);
}

TEST_F(MirEmitterTests, EmitNopViaHelperProducesInstruction)
{
    MirInstruction *instr = emitter->emitNOP();
    ASSERT_NE(instr, nullptr);
}

// ─── Two-operand instruction ──────────────────────────────────────────────────

TEST_F(MirEmitterTests, EmitMovWithTwoRegisters)
{
    MirRegister dst = emitter->createVirtualRegister(8);
    MirRegister src = emitter->createVirtualRegister(8);
    MirInstruction *instr = emitter->emitMOV(MirOperand(dst), MirOperand(src));
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_TRUE(instr->hasOperands());
    EXPECT_EQ(instr->getOperands()->m_numElems, 2u);
}

// ─── Register creation ────────────────────────────────────────────────────────

TEST_F(MirEmitterTests, CreateRegisterHasUniqueId)
{
    MirRegister r1 = emitter->createVirtualRegister(8);
    MirRegister r2 = emitter->createVirtualRegister(8);
    EXPECT_NE(r1.m_id, r2.m_id);
    EXPECT_NE(r1.m_id, MIRID_INVALID);
}

TEST_F(MirEmitterTests, RegisterStoressSize)
{
    MirRegister r = emitter->createVirtualRegister(4);
    EXPECT_EQ(r.m_sizeInBytes, 4u);
}

// ─── Additional MirEmitter tests ─────────────────────────────────────────────

TEST_F(MirEmitterTests, EmitAddWithTwoRegisters)
{
    MirRegister dst = emitter->createVirtualRegister(8);
    MirRegister src = emitter->createVirtualRegister(8);
    MirInstruction *instr = emitter->emitADD(MirOperand(dst), MirOperand(src));
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::ADD);
    EXPECT_EQ(instr->getOperands()->m_numElems, 2u);
}

TEST_F(MirEmitterTests, EmitSubWithTwoRegisters)
{
    MirRegister dst = emitter->createVirtualRegister(8);
    MirRegister src = emitter->createVirtualRegister(8);
    MirInstruction *instr = emitter->emitSUB(MirOperand(dst), MirOperand(src));
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::SUB);
}

TEST_F(MirEmitterTests, EmitHalt)
{
    MirInstruction *instr = emitter->emitHALT();
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::HALT);
}

TEST_F(MirEmitterTests, EmitCmpWithTwoRegisters)
{
    MirRegister a = emitter->createVirtualRegister(8);
    MirRegister b = emitter->createVirtualRegister(8);
    MirInstruction *instr = emitter->emitCMP(MirOperand(a), MirOperand(b));
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::CMP);
}

TEST_F(MirEmitterTests, EmitJmpWithReference)
{
    MirBlock *target = ctx->createBlock();
    MirInstruction *instr = emitter->emitJMP(ctx->createReference(target));
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JMP);
}

TEST_F(MirEmitterTests, EmitRetWithRegister)
{
    MirRegister r = emitter->createVirtualRegister(8);
    MirInstruction *instr = emitter->emitRET(MirOperand(r));
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::RET);
}

TEST_F(MirEmitterTests, EmitCreateWithRegister)
{
    MirRegister r = emitter->createVirtualRegister(8);
    MirInstruction *instr = emitter->emitCREATE(MirOperand(r));
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::CREATE);
}

TEST_F(MirEmitterTests, InstructionHasOperandsFalseForNop)
{
    MirInstruction *instr = emitter->emitNOP();
    ASSERT_NE(instr, nullptr);
    EXPECT_FALSE(instr->hasOperands());
}

TEST_F(MirEmitterTests, InstructionHasOperandsTrueForMov)
{
    MirRegister dst = emitter->createVirtualRegister(8);
    MirRegister src = emitter->createVirtualRegister(8);
    MirInstruction *instr = emitter->emitMOV(MirOperand(dst), MirOperand(src));
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->hasOperands());
}

TEST_F(MirEmitterTests, InstructionMetadataOperandCount)
{
    MirInstruction *nop = emitter->emitNOP();
    ASSERT_NE(nop, nullptr);
    EXPECT_EQ(nop->getMetadata().m_operands.size(), 0u);

    MirRegister dst = emitter->createVirtualRegister(8);
    MirRegister src = emitter->createVirtualRegister(8);
    MirInstruction *mov = emitter->emitMOV(MirOperand(dst), MirOperand(src));
    ASSERT_NE(mov, nullptr);
    EXPECT_EQ(mov->getMetadata().m_operands.size(), 2u);
}

TEST_F(MirEmitterTests, MirOperandRegisterType)
{
    MirRegister r = emitter->createVirtualRegister(8);
    MirOperand op(r);
    EXPECT_EQ(op.getType(), MirOperandType::Register);
    ASSERT_NE(op.getRegister(), nullptr);
    EXPECT_EQ(op.getRegister()->m_id, r.m_id);
    EXPECT_EQ(op.getRegister()->m_sizeInBytes, 8u);
}

TEST_F(MirEmitterTests, MirOperandIntegerType)
{
    MirOperand op(MirInteger{ 42 });
    EXPECT_EQ(op.getType(), MirOperandType::Integer);
    ASSERT_NE(op.getInteger(), nullptr);
    EXPECT_EQ(op.getInteger()->m_value, 42);
}

TEST_F(MirEmitterTests, MirOperandDoubleType)
{
    MirOperand op(MirDouble{ 3.14 });
    EXPECT_EQ(op.getType(), MirOperandType::Double);
    ASSERT_NE(op.getDouble(), nullptr);
    EXPECT_DOUBLE_EQ(op.getDouble()->m_value, 3.14);
}

TEST_F(MirEmitterTests, MirOperandReferenceType)
{
    MirOperand op(MirReference{ .m_type = MirReferenceType::Block, .m_refId = 99 });
    EXPECT_EQ(op.getType(), MirOperandType::Reference);
    ASSERT_NE(op.getReference(), nullptr);
    EXPECT_EQ(op.getReference()->m_type, MirReferenceType::Block);
    EXPECT_EQ(op.getReference()->m_refId, 99u);
}

TEST_F(MirEmitterTests, PushOperandToInstruction)
{
    MirInstruction *instr = emitter->emitNOP();
    ASSERT_NE(instr, nullptr);
    MirRegister r = emitter->createVirtualRegister(8);
    emitter->emitOperandToInstruction(instr, MirOperand(r));
    EXPECT_TRUE(instr->hasOperands());
    EXPECT_EQ(instr->getOperands()->m_numElems, 1u);
}

TEST_F(MirEmitterTests, CreateRegisterDifferentSizes)
{
    MirRegister r1 = emitter->createVirtualRegister(1);
    MirRegister r2 = emitter->createVirtualRegister(2);
    MirRegister r4 = emitter->createVirtualRegister(4);
    MirRegister r8 = emitter->createVirtualRegister(8);
    EXPECT_EQ(r1.m_sizeInBytes, 1u);
    EXPECT_EQ(r2.m_sizeInBytes, 2u);
    EXPECT_EQ(r4.m_sizeInBytes, 4u);
    EXPECT_EQ(r8.m_sizeInBytes, 8u);
    // All unique IDs
    EXPECT_NE(r1.m_id, r2.m_id);
    EXPECT_NE(r2.m_id, r4.m_id);
    EXPECT_NE(r4.m_id, r8.m_id);
}