/**
 * @file T_ExpandTypeLegalizer.cpp
 * @brief Unit tests for ExpandTypeLegalizer.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <filesystem>

class DummyABIDescExpand : public ABIDesc
{
  public:
    DummyABIDescExpand()
    {
        setRegSizeInBits(32); // 4 bytes legal size
    }

    ArgLocation getArgLoc(size_t id) const override { return ArgLocation(); }
};

class ExpandTypeLegalizerTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;

    DummyABIDescExpand abi;
    TargetDesc *targetDesc;
    LegalizerActionList *actionList;
    LegalizerHandlerList *handlerList;
    LegalizerContext *legalizerCtx;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());

        targetDesc = new TargetDesc(&abi, "DummyTarget");
        actionList = new LegalizerActionList();
        handlerList = new LegalizerHandlerList();

        legalizerCtx = new LegalizerContext(targetDesc, actionList, handlerList, emitter);

        ec->beginScope();
        MirBlock *block = emitterCtx->createBlock();
        emitterCtx->bindToBlock(block);
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete legalizerCtx;
        delete handlerList;
        delete actionList;
        delete targetDesc;
        delete emitter;
    }
};

TEST_F(ExpandTypeLegalizerTests, ExpandAddInstruction)
{
    MirRegister dest = emitter->createVirtualRegister(8); // 8 bytes (illegal, needs expansion to 2x 4 bytes)
    MirRegister src = emitter->createVirtualRegister(8);

    MirInstruction *instr = emitter->emitADD(MirOperand(dest), MirOperand(src));

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it = list->begin();

    bool result = StandardLegalizers::expandTypeLegalizer(legalizerCtx, list, it);

    EXPECT_TRUE(result);

    auto newList = emitterCtx->getCurrentBoundBlock()->getInstructions();

    // We expect ADD (low) and ADC (high) instructions to be generated, and the original ADD removed.
    auto nextIt = newList->begin();
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADD);

    // Validate low operands
    auto addOperands = (*nextIt)->getOperands();
    EXPECT_EQ(addOperands->m_numElems, 2);
    auto addOpIt = addOperands->begin();
    EXPECT_EQ((*addOpIt)->getRegister()->m_sizeInBytes, 4);

    ++nextIt;
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADC);

    // Validate high operands
    auto adcOperands = (*nextIt)->getOperands();
    EXPECT_EQ(adcOperands->m_numElems, 2);
    auto adcOpIt = adcOperands->begin();
    EXPECT_EQ((*adcOpIt)->getRegister()->m_sizeInBytes, 4);
}

TEST_F(ExpandTypeLegalizerTests, ExpandInstructionWithImmediate)
{
    MirRegister dest = emitter->createVirtualRegister(8); // 8 bytes
    MirInteger imm = { .m_value = 0x123456789ABCDEF0, .m_sizeInBytes = 8 };

    MirInstruction *instr = emitter->emitADD(MirOperand(dest), MirOperand(imm));

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it = list->begin();

    bool result = StandardLegalizers::expandTypeLegalizer(legalizerCtx, list, it);

    EXPECT_TRUE(result);

    auto newList = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto nextIt = newList->begin();

    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADD);

    auto addOperands = (*nextIt)->getOperands();
    auto addOpIt = addOperands->begin();
    ++addOpIt; // Get the immediate
    ASSERT_TRUE((*addOpIt)->isInteger());
    EXPECT_EQ((*addOpIt)->getInteger()->m_sizeInBytes, 4);
    EXPECT_EQ(static_cast<uint32_t>((*addOpIt)->getInteger()->m_value), 0x9ABCDEF0);

    ++nextIt;
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADC);

    auto adcOperands = (*nextIt)->getOperands();
    auto adcOpIt = adcOperands->begin();
    ++adcOpIt; // Get the immediate
    ASSERT_TRUE((*adcOpIt)->isInteger());
    EXPECT_EQ((*adcOpIt)->getInteger()->m_sizeInBytes, 4);
    EXPECT_EQ(static_cast<uint32_t>((*adcOpIt)->getInteger()->m_value), 0x12345678);
}

TEST_F(ExpandTypeLegalizerTests, ExpandInvalidOpcodeTriggersError)
{
    MirRegister dest = emitter->createVirtualRegister(8);
    // TRUNC has NO_EQUIV, meaning it doesn't have a linear equivalent to expand to.
    MirInstruction *instr = emitter->emitTRUNC(MirOperand(dest), MirOperand(dest));

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it = list->begin();

    bool result = StandardLegalizers::expandTypeLegalizer(legalizerCtx, list, it);

    EXPECT_FALSE(result);
    EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
}

TEST_F(ExpandTypeLegalizerTests, ExpandSameRegisterReusesExpansion)
{
    MirRegister dest = emitter->createVirtualRegister(8);
    // Both operands use the same register
    MirInstruction *instr = emitter->emitADD(MirOperand(dest), MirOperand(dest));

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it = list->begin();

    bool result = StandardLegalizers::expandTypeLegalizer(legalizerCtx, list, it);
    EXPECT_TRUE(result);

    auto newList = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto nextIt = newList->begin();

    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADD);

    auto addOperands = (*nextIt)->getOperands();
    auto op1 = addOperands->begin();
    auto op2 = op1;
    ++op2;

    // The low part of the register expansion should be the same register ID.
    EXPECT_EQ((*op1)->getRegister()->m_id, (*op2)->getRegister()->m_id);

    ++nextIt;
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADC);

    auto adcOperands = (*nextIt)->getOperands();
    op1 = adcOperands->begin();
    op2 = op1;
    ++op2;

    // The high part of the register expansion should be the same register ID.
    EXPECT_EQ((*op1)->getRegister()->m_id, (*op2)->getRegister()->m_id);
}
