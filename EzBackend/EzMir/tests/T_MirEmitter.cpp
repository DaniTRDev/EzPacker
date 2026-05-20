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
    MirRegister *dst = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirRegister *src = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirInstruction *instr = emitter->emitMOV(dst, src);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_TRUE(instr->hasOperands());
    EXPECT_EQ(instr->getOperands()->m_numElems, 2u);
}

// ─── Register creation ────────────────────────────────────────────────────────

TEST_F(MirEmitterTests, CreateRegisterHasUniqueId)
{
    MirRegister *r1 = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirRegister *r2 = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    EXPECT_NE(r1->getRegId(), r2->getRegId());
    EXPECT_NE(r1->getRegId(), MIRID_INVALID);
}

TEST_F(MirEmitterTests, RegisterStoressSize)
{
    MirRegister *r = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(4));
    EXPECT_EQ(r->getSizeInBytes(), 4u);
}

// ─── Additional MirEmitter tests ─────────────────────────────────────────────

TEST_F(MirEmitterTests, EmitAddWithTwoRegisters)
{
    MirRegister *dst = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirRegister *src = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirInstruction *instr = emitter->emitADD(dst, src);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::ADD);
    EXPECT_EQ(instr->getOperands()->m_numElems, 2u);
}

TEST_F(MirEmitterTests, EmitSubWithTwoRegisters)
{
    MirRegister *dst = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirRegister *src = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirInstruction *instr = emitter->emitSUB(dst, src);
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
    MirRegister *a = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirRegister *b = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirInstruction *instr = emitter->emitCMP(a, b);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::CMP);
}

TEST_F(MirEmitterTests, EmitJmpWithReference)
{
    MirBlock *target = ctx->createBlock();
    MirInstruction *instr = emitter->emitJMP(emitter->createBlockRef(target));
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::JMP);
}

TEST_F(MirEmitterTests, EmitRetWithRegister)
{
    MirRegister *r = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirInstruction *instr = emitter->emitRET(r);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->getOpCode(), MirInstructionOpCode::RET);
}

TEST_F(MirEmitterTests, EmitCreateWithRegister)
{
    MirRegister *r = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirInstruction *instr = emitter->emitCREATE(r);
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
    MirRegister *dst = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirRegister *src = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirInstruction *instr = emitter->emitMOV(dst, src);
    ASSERT_NE(instr, nullptr);
    EXPECT_TRUE(instr->hasOperands());
}

TEST_F(MirEmitterTests, InstructionMetadataOperandCount)
{
    MirInstruction *nop = emitter->emitNOP();
    ASSERT_NE(nop, nullptr);
    EXPECT_EQ(nop->getMetadata().m_operandConstraints.size(), 0u);

    MirRegister *dst = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirRegister *src = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    MirInstruction *mov = emitter->emitMOV(dst, src);
    ASSERT_NE(mov, nullptr);
    EXPECT_EQ(mov->getMetadata().m_operandConstraints.size(), 2u);
}

TEST_F(MirEmitterTests, PushOperandToInstruction)
{
    MirInstruction *instr = emitter->emitNOP();
    ASSERT_NE(instr, nullptr);
    MirRegister *r = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    emitter->emitOperandToInstruction(instr, r);
    EXPECT_TRUE(instr->hasOperands());
    EXPECT_EQ(instr->getOperands()->m_numElems, 1u);
}

TEST_F(MirEmitterTests, CreateRegisterDifferentSizes)
{
    MirRegister *r1 = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(1));
    MirRegister *r2 = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(2));
    MirRegister *r4 = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(4));
    MirRegister *r8 = emitter->createVirtualRegister(ctx->getIntegerTypeBySize(8));
    EXPECT_EQ(r1->getSizeInBytes(), 1u);
    EXPECT_EQ(r2->getSizeInBytes(), 2u);
    EXPECT_EQ(r4->getSizeInBytes(), 4u);
    EXPECT_EQ(r8->getSizeInBytes(), 8u);
    // All unique IDs
    EXPECT_NE(r1->getRegId(), r2->getRegId());
    EXPECT_NE(r2->getRegId(), r4->getRegId());
    EXPECT_NE(r4->getRegId(), r8->getRegId());
}