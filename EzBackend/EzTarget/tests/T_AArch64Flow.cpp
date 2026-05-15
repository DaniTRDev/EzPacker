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

    ArgLocation getArgLoc(size_t id) const override
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
};

// --- Global state for handlers ---
static MirEmitter *g_emitter = nullptr;
static bool loadHandlerCalled = false;
static bool storeHandlerCalled = false;

// --- Custom Handlers ---
static LegalizerHandlerResult AArch64LoadHandler(TypedPoolLinkedList<struct MirInstruction> *instrList,
                                                 TypedPoolLinkedList<class MirInstruction>::Iterator it)
{
    loadHandlerCalled = true;
    MirInstruction *loadInstr = *it;
    MirOperand *dest = loadInstr->getOperands()->get<MirOperand>(0);
    MirOperand *addr = loadInstr->getOperands()->get<MirOperand>(1);

    // Replace LOAD with a dummy MOV instruction for test purposes
    g_emitter->emitMOV(dest, addr);
    instrList->m_owner->removeFromList(instrList, it);

    return { false, true, true };
}

static LegalizerHandlerResult AArch64StoreHandler(TypedPoolLinkedList<struct MirInstruction> *instrList,
                                                  TypedPoolLinkedList<class MirInstruction>::Iterator it)
{
    storeHandlerCalled = true;
    MirInstruction *storeInstr = *it;
    MirOperand *addr = storeInstr->getOperands()->get<MirOperand>(0);
    MirOperand *src = storeInstr->getOperands()->get<MirOperand>(2);

    // Replace STORE with a dummy MOV instruction for test purposes
    g_emitter->emitMOV(addr, src); // Not a valid semantic replacement, just for testing
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
    std::shared_ptr<MirTypes> m_types;
    MirEmitter *emitter;

    AArch64ABIDesc abi;
    TargetDesc *targetDesc;
    LegalizerActionList *actionList;
    LegalizerHandlerList *handlerList;
    LegalizerContext *legalizerCtx;
    TypeLegalizerPass *pass;
    MirPassManager *passManager;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());
        g_emitter = emitter; // Set global emitter

        m_types = std::make_shared<MirTypes>();
        m_types->initialize(emitterCtx.get());

        targetDesc = new TargetDesc(&abi, "AArch64");
        actionList = new LegalizerActionList();
        handlerList = new LegalizerHandlerList();
        passManager = new MirPassManager();

        legalizerCtx = new LegalizerContext(targetDesc, actionList, handlerList, emitter);
        pass = new TypeLegalizerPass(legalizerCtx);

        ec->beginScope();
        MirBlock *block = emitterCtx->createBlock();
        emitterCtx->bindToBlock(block);
        loadHandlerCalled = false;
        storeHandlerCalled = false;
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete pass;
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
    MirRegister *dest = emitter->createVirtualRegister(m_types->getInt64Type()); // 64-bit
    MirRegister *addr = emitter->createVirtualRegister(m_types->getInt64Type());
    MirRegister *src = emitter->createVirtualRegister(m_types->getInt64Type());

    MirInteger *i1 = emitter->createImmediateInteger(m_types->getInt32Type(), 0);
    MirInteger *i2 = emitter->createImmediateInteger(m_types->getInt32Type(), 0);

    emitter->emitLOAD(dest, addr, i1);
    emitter->emitSTORE(addr, i2, src);

    // Run the legalizer pass
    auto list = emitterCtx->getCurrentBoundBlock()->getInstructions();
    bool modified = true;
    while (modified)
    {
        modified = false;
        for (auto it = list->begin(); it != list->end(); ++it)
        {
            if (pass->run(list, it, passManager))
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
    auto newList = emitterCtx->getCurrentBoundBlock()->getInstructions();

    ASSERT_EQ(newList->m_numElems, 2);
    auto it = newList->begin();
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV);
    ++it;
    EXPECT_EQ((*it)->getOpCode(), MirInstructionOpCode::MOV);
}
