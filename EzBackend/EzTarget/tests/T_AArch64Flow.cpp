/**
 * @file T_AArch64Flow.cpp
 * @brief Unit test for a complete AArch64 flow.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetDesc.h>
#include <TargetLegalizer/TypeLegalizerPass.h>
#include <filesystem>

// --- Test-specific AArch64 ABI ---
class AArch64ABIDesc : public ABIDesc
{
  public:
    AArch64ABIDesc()
    {
        setRegSizeInBits(64);
        setStackOffsetSize(64);
        // AArch64 specific registers, not strictly needed for this test
        setStackReg(31);   // SP
        setStackFrame(29); // FP
    }

    const char *getName() const override { return "AArch64"; }

    ArgLocation getArgLoc(size_t id, MirType *type) const override
    {
        // Simplified: first 8 args in registers X0-X7
        if (id < 8)
        {
            ArgLocation loc;
            loc.setLoc(PhysicalRegLocation(static_cast<PhysicalRegId>(id), 64));

            return loc;
        }
        return ArgLocation(); // Others on stack, not handled here
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

    /**
     * Returns the preferred alignment for the given type.
     * Used for global variables to optimize CPU cache line fetching.
     */
    size_t getPreferredAlignment(MirType *type) const { return getAbiAlignment(type); }
};

// --- Global state for handlers ---
static MirEmitter *g_emitter = nullptr;
static bool loadHandlerCalled = false;
static bool storeHandlerCalled = false;

// --- Custom Handlers ---
static LegalizerHandlerResult AArch64LoadHandler(MirEmitter *emitter,
                                                 TypedPoolLinkedList<struct MirInstruction> *instrList,
                                                 TypedPoolLinkedList<class MirInstruction>::Iterator it)
{
    loadHandlerCalled = true;
    MirInstruction *loadInstr = *it;
    MirOperand *dest = loadInstr->getOperands()->get<MirOperand>(0);
    MirOperand *addr = loadInstr->getOperands()->get<MirOperand>(1);

    // Replace LOAD with a dummy MOV instruction for test purposes
    emitter->emitMOV(dest, addr);
    instrList->m_owner->removeFromList(instrList, it);

    return { false, true, true };
}

static LegalizerHandlerResult AArch64StoreHandler(MirEmitter *emitter,
                                                  TypedPoolLinkedList<struct MirInstruction> *instrList,
                                                  TypedPoolLinkedList<class MirInstruction>::Iterator it)
{
    storeHandlerCalled = true;
    MirInstruction *storeInstr = *it;
    MirOperand *addr = storeInstr->getOperands()->get<MirOperand>(0);
    MirOperand *src = storeInstr->getOperands()->get<MirOperand>(2);

    // Replace STORE with a dummy MOV instruction for test purposes
    emitter->emitMOV(addr, src); // Not a valid semantic replacement, just for testing
    instrList->m_owner->removeFromList(instrList, it);

    return { false, true, true };
}

// --- Test Fixture ---
class AArch64FlowTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;

    AArch64ABIDesc abi;
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
        g_emitter = emitter; // Set global emitter

        targetDesc = new TargetDesc(&abi, TargetEndianness::LittleEndian, "AArch64");
        actionList = new LegalizerActionList();
        handlerList = new LegalizerHandlerList();
        passManager = new MirPassManager();

        legalizerCtx = new LegalizerContext(targetDesc, actionList, handlerList, emitter);
        passManager->addPass<TypeLegalizerPass>(legalizerCtx);

        ec->beginScope();
        MirBlock *block = emitterCtx->createBlock();
        emitterCtx->setInsertPoint(block);
        loadHandlerCalled = false;
        storeHandlerCalled = false;
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
        g_emitter = nullptr;
    }
};

// --- Test Case ---
TEST_F(AArch64FlowTests, CustomLoadStoreHandlers)
{
    // Define custom handling for 64-bit LOAD and STORE
    actionList->setOperandAction(MirInstructionOpCode::LOAD, 64, TargetLegalizerActionType::Action_TypeCustom);
    actionList->setOperandAction(MirInstructionOpCode::STORE, 64, TargetLegalizerActionType::Action_TypeCustom);
    handlerList->addInstructionHandler(MirInstructionOpCode::LOAD, AArch64LoadHandler);
    handlerList->addInstructionHandler(MirInstructionOpCode::STORE, AArch64StoreHandler);

    // Emit instructions that will trigger the custom handlers
    MirRegister *dest = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type()); // 64-bit
    MirRegister *addr = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *src = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());

    MirInteger *i1 = emitter->createImmediateInteger(emitterCtx->getTypes()->getInt32Type(), 0);
    MirInteger *i2 = emitter->createImmediateInteger(emitterCtx->getTypes()->getInt32Type(), 0);

    emitter->emitLOAD(dest, addr, i1);
    emitter->emitSTORE(addr, i2, src);

    // Run the legalizer pass
    auto list = emitterCtx->getCurrentBlock()->getInstructions();
    bool modified = true;
    while (modified)
    {
        modified = false;
        for (auto it = list->begin(); it != list->end(); ++it)
        {
            if (passManager->run(list, it, passManager))
            {
                modified = true;
                break; // Restart scan
            }
        }
    }

    // Verify that the custom handlers were called
    EXPECT_TRUE(loadHandlerCalled);
    EXPECT_TRUE(storeHandlerCalled);

    // Verify that the instructions were replaced
    auto newList = emitterCtx->getCurrentBlock()->getInstructions();

    ASSERT_EQ(newList->m_numElems, 2);
    auto it = newList->begin();
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV);
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV);
}
