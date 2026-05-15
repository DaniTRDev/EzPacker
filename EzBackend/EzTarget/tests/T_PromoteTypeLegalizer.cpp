/**
 * @file T_PromoteTypeLegalizer.cpp
 * @brief Unit tests for PromoteTypeLegalizer.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetLegalizer/StandardLegalizers/PromoteTypeLegalizer.h>
#include <filesystem>

class DummyABIDesc : public ABIDesc
{
  public:
    DummyABIDesc()
    {
        setRegSizeInBits(64); // 8 bytes
    }
    
    ArgLocation getArgLoc(size_t id) const override
    {
        return ArgLocation();
    }
};

class PromoteTypeLegalizerTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> ctx;
    MirEmitter *emitter;
    DummyABIDesc abi;
    TargetDesc *targetDesc;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        ctx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(ctx.get());
        
        targetDesc = new TargetDesc(&abi, "DummyTarget");

        ec->beginScope();
        MirBlock *block = ctx->createBlock();
        ctx->bindToBlock(block);
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete emitter;
        delete targetDesc;
    }
};

TEST_F(PromoteTypeLegalizerTests, IgnoreLegalSizedRegister)
{
    MirRegister r = emitter->createVirtualRegister(8); // 8 bytes
    MirInstruction *instr = emitter->emitMOV(MirOperand(r), MirOperand(r));
    
    auto it = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    auto opIt = instr->getOperands()->begin();
    
    bool modified = StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, opIt, 0);
    EXPECT_FALSE(modified);
}

TEST_F(PromoteTypeLegalizerTests, PromoteOutputRegister)
{
    MirRegister r = emitter->createVirtualRegister(4); // 4 bytes, smaller than 8 bytes
    MirInstruction *instr = emitter->emitMOV(MirOperand(r), MirOperand(r));
    
    auto it = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    auto opIt = instr->getOperands()->begin(); // Destination operand (Write)
    
    bool modified = StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, opIt, 0);
    EXPECT_TRUE(modified);
    EXPECT_EQ((*opIt)->getRegister()->m_sizeInBytes, 8u);
    
    // We expect a TRUNC instruction emitted AFTER this one.
    // So the list should be MOV, TRUNC.
    auto nextIt = it;
    ++nextIt;
    EXPECT_NE(nextIt, ctx->getCurrentBoundBlock()->getInstructions()->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::TRUNC);
}

TEST_F(PromoteTypeLegalizerTests, PromoteInputRegister)
{
    MirRegister r = emitter->createVirtualRegister(4); // 4 bytes
    MirInstruction *instr = emitter->emitMOV(MirOperand(r), MirOperand(r));
    
    auto it = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    auto opIt = instr->getOperands()->begin();
    ++opIt; // Source operand (Input)
    
    bool modified = StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, opIt, 1);
    EXPECT_TRUE(modified);
    EXPECT_EQ((*opIt)->getRegister()->m_sizeInBytes, 8u);
    
    // We expect an EXT (SEXT or ZEXT) instruction emitted BEFORE this one.
    // So the list should be ZEXT/SEXT, MOV.
    auto firstIt = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    EXPECT_NE((*firstIt)->getOpCode(), MirInstructionOpCode::MOV);
    EXPECT_TRUE((*firstIt)->getOpCode() == MirInstructionOpCode::ZEXT || (*firstIt)->getOpCode() == MirInstructionOpCode::SEXT);
}

TEST_F(PromoteTypeLegalizerTests, PromoteImmediate)
{
    MirRegister r = emitter->createVirtualRegister(8);
    MirInstruction *instr = emitter->emitMOV(MirOperand(r), MirOperand(MirInteger{42, 4})); // 4-byte int
    
    auto it = ctx->getCurrentBoundBlock()->getInstructions()->begin();
    auto opIt = instr->getOperands()->begin();
    ++opIt; // Source operand
    
    bool modified = StandardLegalizers::promoteTypeLegalizer(emitter, targetDesc, it, opIt, 1);
    EXPECT_TRUE(modified);
    EXPECT_EQ((*opIt)->getInteger()->m_sizeInBytes, 8u);
    EXPECT_EQ((*opIt)->getInteger()->m_value, 42);
}
