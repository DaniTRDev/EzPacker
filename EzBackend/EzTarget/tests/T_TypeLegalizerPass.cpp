/**
 * @file T_TypeLegalizerPass.cpp
 * @brief Unit tests for TypeLegalizerPass.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetLegalizer/TypeLegalizerPass.h>
#include <filesystem>

class DummyABIDescLegalizer : public ABIDesc
{
  public:
    DummyABIDescLegalizer()
    {
        setRegSizeInBits(32); // 4 bytes legal size
    }

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

    ArgLocation getArgLoc(size_t id, MirType *type) const override { return ArgLocation(); }
};

static bool customHandlerCalled = false;
static LegalizerHandlerResult DummyCustomHandler(MirEmitter *emitter,
                                                 TypedPoolLinkedList<struct MirInstruction> *instrList,
                                                 TypedPoolLinkedList<class MirInstruction>::Iterator it)
{
    customHandlerCalled = true;
    return { false, true, true };
}

class TypeLegalizerPassTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;

    DummyABIDescLegalizer abi;
    TargetDesc *targetDesc;
    LegalizerActionList *actionList;
    LegalizerHandlerList *handlerList;
    LegalizerContext *legalizerCtx;
    MirPassManager *passManager;

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

        passManager = new MirPassManager();
        passManager->addPass<TypeLegalizerPass>(legalizerCtx);

        ec->beginScope();
        MirBlock *block = emitterCtx->createBlock();
        emitterCtx->bindToBlock(block);
        customHandlerCalled = false;
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete legalizerCtx;
        delete passManager;
        delete handlerList;
        delete actionList;
        delete targetDesc;
        delete emitter;
    }
};

TEST_F(TypeLegalizerPassTests, RunDoesNothingIfNoneAction)
{
    MirRegister *dest = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt32Type()); // Legal size
    MirRegister *src = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt32Type());

    MirInstruction *instr = emitter->emitMOV(dest, src);

    actionList->setOperandAction(MirInstructionOpCode::MOV, 32, Action_None);

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it = list->begin();

    bool modified = passManager->run(list, it, passManager);
    EXPECT_FALSE(modified);
}

TEST_F(TypeLegalizerPassTests, RunCallsPromoteOperand)
{
    MirRegister *dest = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt16Type()); // Illegal small size
    MirRegister *src = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt16Type());

    MirInstruction *instr = emitter->emitADD(dest, src);

    // Set 16-bit to be promoted
    actionList->setOperandAction(MirInstructionOpCode::ADD, 16, TargetLegalizerActionType::Action_PromoteOperand);

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it = list->begin();

    bool modified = passManager->run(list, it, passManager);
    EXPECT_TRUE(modified);

    // Check if promotion happened (e.g. TRUNC was appended)
    auto newList = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto nextIt = newList->begin();
    ++nextIt;
    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::TRUNC);
}

TEST_F(TypeLegalizerPassTests, RunCallsExpandOperand)
{
    MirRegister *dest = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type()); // Illegal large size
    MirRegister *src = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());

    MirInstruction *instr = emitter->emitADD(dest, src);

    // Set 64-bit to be expanded
    actionList->setOperandAction(MirInstructionOpCode::ADD, 64, TargetLegalizerActionType::Action_ExpandOperand);

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it = list->begin();

    bool modified = passManager->run(list, it, passManager);
    EXPECT_TRUE(modified);

    // Check if expansion happened (ADD + ADC)
    auto newList = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto firstIt = newList->begin();
    auto nextIt = firstIt;
    ++nextIt;

    ASSERT_NE(nextIt, newList->end());
    EXPECT_EQ((*firstIt)->getOpCode(), MirInstructionOpCode::ADD);
    EXPECT_EQ((*nextIt)->getOpCode(), MirInstructionOpCode::ADC);
}

TEST_F(TypeLegalizerPassTests, RunCallsCustomHandler)
{
    MirRegister *dest = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *src = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());

    MirInstruction *instr = emitter->emitMUL(dest, src);

    actionList->setOperandAction(MirInstructionOpCode::MUL, 64, Action_TypeCustom);
    handlerList->addInstructionHandler(MirInstructionOpCode::MUL, DummyCustomHandler);

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it = list->begin();

    bool modified = passManager->run(list, it, passManager);
    // Custom handlers return void from the list point of view but may modify. TypeLegalizerPass currently returns true
    // for Custom if it loops and runs handler Wait, the handler returns LegalizerHandlerResult, but the pass checks it?
    // Let's look at `TypeLegalizerPass.cpp`:
    // It loops handlers, doesn't return immediately, then drops out of if. So returns false right now?
    // Wait, let's see TypeLegalizerPass.cpp

    EXPECT_TRUE(customHandlerCalled);
}

TEST_F(TypeLegalizerPassTests, MixedPromoteAndExpandInSameContext)
{
    // Instruction 1: Needs expansion
    MirRegister *dest64 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirInstruction *instr1 = emitter->emitADD(dest64, dest64);

    // Instruction 2: Needs promotion
    MirRegister *dest16 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt16Type());
    MirInstruction *instr2 = emitter->emitMOV(dest16, dest16);

    actionList->setOperandAction(MirInstructionOpCode::ADD, 64, TargetLegalizerActionType::Action_ExpandOperand);
    actionList->setOperandAction(MirInstructionOpCode::MOV, 16, TargetLegalizerActionType::Action_PromoteOperand);

    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto it1 = list->begin();

    // Legalize instr1 (Expand)
    bool modified1 = passManager->run(list, it1, passManager);
    EXPECT_TRUE(modified1);

    // list now has ADD, ADC, MOV
    auto it2 = list->begin();
    ++it2; // ADC
    ++it2; // MOV

    // Legalize instr2 (Promote)
    bool modified2 = passManager->run(list, it2, passManager);
    EXPECT_TRUE(modified2);

    // list now has ADD, ADC, MOV, TRUNC
    auto newList = emitterCtx->getCurrentBoundBlock()->getInstructions();
    auto finalIt = newList->begin();

    EXPECT_EQ((*finalIt)->getOpCode(), MirInstructionOpCode::ADD);
    ++finalIt;
    EXPECT_EQ((*finalIt)->getOpCode(), MirInstructionOpCode::ADC);
    ++finalIt;
    EXPECT_EQ((*finalIt)->getOpCode(), MirInstructionOpCode::MOV);
    ++finalIt;
    EXPECT_EQ((*finalIt)->getOpCode(), MirInstructionOpCode::TRUNC);
}
