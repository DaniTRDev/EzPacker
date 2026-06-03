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

    ArgLocation getArgLoc(size_t id, MirType *type) const override { return ArgLocation(); }
    
    size_t getAbiAlignment(MirType *type) const
    {
        if (!type)
            return 1;
        
        // Handle Basic Types (Integers, Floats)
        size_t size = type->getTotalSizeInBytes();

        // Standard rule: basic types align to their own size, capped by the target max.
        // E.g., size 4 aligns to 4. Size 8 aligns to 8 (or 4 on 32-bit systems).
        size_t align = size;

        // Ensure it's a power of 2 (rounds up sizes like 3 to 4)
        align = std::bit_ceil(align); // C++20 feature, or write a quick power-of-2 helper
        return align;
    }
    
    const char *getName() const override { return "DummyAbi"; }

    /**
     * Returns the preferred alignment for the given type.
     * Used for global variables to optimize CPU cache line fetching.
     */
    size_t getPreferredAlignment(MirType *type) const { return getAbiAlignment(type); }
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

        targetDesc = new TargetDesc(&abi, TargetEndianness::LittleEndian, "DummyTarget");
        actionList = new LegalizerActionList();
        handlerList = new LegalizerHandlerList();

        legalizerCtx = new LegalizerContext(targetDesc, actionList, handlerList, emitter);

        ec->beginScope();
        MirBlock *block = emitterCtx->createBlock();
        emitterCtx->setInsertPoint(block);
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
    // 8 bytes (illegal, needs expansion to 2x 4 bytes)
    MirRegister *dest = emitter->createVirtualRegister(emitter->getContext()->getIntegerTypeBySize(8));
    MirRegister *src = emitter->createVirtualRegister(emitter->getContext()->getIntegerTypeBySize(8));

    MirInstruction *instr = emitter->emitADD(dest, src);

    auto list = emitterCtx->getCurrentBlock()->getInstructions();
    auto it = list->begin();

    bool result = StandardLegalizers::expandTypeLegalizer(legalizerCtx, list, it);

    EXPECT_TRUE(result);

    auto newList = emitterCtx->getCurrentBlock()->getInstructions();

    // We expect ADD (low) and ADC (high) instructions to be generated, and the original ADD removed.
    auto nextIt = newList->begin();
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADD);

    // Validate low operands
    auto addOperands = (*nextIt)->getOperands();
    EXPECT_EQ(addOperands->m_numElems, 2);
    auto addOpIt = addOperands->begin();
    EXPECT_EQ((*addOpIt)->get<MirRegister>()->getSizeInBytes(), 4);

    ++nextIt;
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADC);

    // Validate high operands
    auto adcOperands = (*nextIt)->getOperands();
    EXPECT_EQ(adcOperands->m_numElems, 2);
    auto adcOpIt = adcOperands->begin();
    EXPECT_EQ((*adcOpIt)->get<MirRegister>()->getSizeInBytes(), 4);
}

TEST_F(ExpandTypeLegalizerTests, ExpandInstructionWithImmediate)
{
    MirType *type = emitter->getContext()->getIntegerTypeBySize(8);
    MirRegister *dest = emitter->createVirtualRegister(type); // 8 bytes
    MirInteger *imm = emitter->createImmediateInteger(type, 0x123456789ABCDEF0);

    MirInstruction *instr = emitter->emitADD(dest, imm);

    auto list = emitterCtx->getCurrentBlock()->getInstructions();
    auto it = list->begin();

    bool result = StandardLegalizers::expandTypeLegalizer(legalizerCtx, list, it);

    EXPECT_TRUE(result);

    auto newList = emitterCtx->getCurrentBlock()->getInstructions();
    auto nextIt = newList->begin();

    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADD);

    auto addOperands = (*nextIt)->getOperands();
    auto addOpIt = addOperands->begin();
    ++addOpIt; // Get the immediate
    ASSERT_TRUE((*addOpIt)->isOfType<MirInteger>());
    EXPECT_EQ((*addOpIt)->get<MirInteger>()->getSizeInBytes(), 4);
    EXPECT_EQ(static_cast<uint32_t>((*addOpIt)->get<MirInteger>()->getValue()), 0x9ABCDEF0);

    ++nextIt;
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADC);

    auto adcOperands = (*nextIt)->getOperands();
    auto adcOpIt = adcOperands->begin();
    ++adcOpIt; // Get the immediate
    ASSERT_TRUE((*adcOpIt)->isOfType<MirInteger>());
    EXPECT_EQ((*adcOpIt)->get<MirInteger>()->getSizeInBytes(), 4);
    EXPECT_EQ(static_cast<uint32_t>((*adcOpIt)->get<MirInteger>()->getValue()), 0x12345678);
}

TEST_F(ExpandTypeLegalizerTests, ExpandInvalidOpcodeTriggersError)
{
    MirRegister *dest = emitter->createVirtualRegister(emitter->getContext()->getIntegerTypeBySize(8));
    // TRUNC has NO_EQUIV, meaning it doesn't have a linear equivalent to expand to.
    MirInstruction *instr = emitter->emitTRUNC(dest, dest);

    auto list = emitterCtx->getCurrentBlock()->getInstructions();
    auto it = list->begin();

    bool result = StandardLegalizers::expandTypeLegalizer(legalizerCtx, list, it);

    EXPECT_FALSE(result);
    EXPECT_TRUE(ec->doesCurrentScopeHasFatalErrors());
}

TEST_F(ExpandTypeLegalizerTests, ExpandSameRegisterReusesExpansion)
{
    MirRegister *dest = emitter->createVirtualRegister(emitter->getContext()->getIntegerTypeBySize(8));
    // Both operands use the same register
    MirInstruction *instr = emitter->emitADD(dest, dest);

    auto list = emitterCtx->getCurrentBlock()->getInstructions();
    auto it = list->begin();

    bool result = StandardLegalizers::expandTypeLegalizer(legalizerCtx, list, it);
    EXPECT_TRUE(result);

    auto newList = emitterCtx->getCurrentBlock()->getInstructions();
    auto nextIt = newList->begin();

    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADD);

    auto addOperands = (*nextIt)->getOperands();
    auto op1 = addOperands->begin();
    auto op2 = op1;
    ++op2;

    // The low part of the register expansion should be the same register ID.
    EXPECT_EQ((*op1)->get<MirRegister>()->getRegId(), (*op2)->get<MirRegister>()->getRegId());

    ++nextIt;
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADC);

    auto adcOperands = (*nextIt)->getOperands();
    op1 = adcOperands->begin();
    op2 = op1;
    ++op2;

    // The high part of the register expansion should be the same register ID.
    EXPECT_EQ((*op1)->get<MirRegister>()->getRegId(), (*op2)->get<MirRegister>()->getRegId());
}
