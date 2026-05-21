/**
 * @file T_X64Flow.cpp
 * @brief Unit test for a complete X64 compilation flow.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetDesc.h>
#include <TargetLegalizer/TypeLegalizerPass.h>
#include <TargetInstructionSelector/InstructionSelectorPass.h>
#include <filesystem>

// Include X64 components
#include "../x64TypeLegalizer.h"
#include "../x64InstrSelector.h"
#include "../x64Emitter.h"

// --- Test-specific X64 ABI ---
class X64ABIDesc : public ABIDesc
{
  public:
    X64ABIDesc()
    {
        setRegSizeInBits(64);
        setStackOffsetSize(64);

        // System V AMD64 ABI (Linux/Mac standard)
        // Caller-saved (Volatile)
        std::vector<PhysicalRegId> callerSaved = {
            asmjit::x86::rax.id(), asmjit::x86::rcx.id(), asmjit::x86::rdx.id(),
            asmjit::x86::rsi.id(), asmjit::x86::rdi.id(), asmjit::x86::r8.id(),
            asmjit::x86::r9.id(),  asmjit::x86::r10.id(), asmjit::x86::r11.id()
        };

        // Callee-saved (Non-Volatile)
        std::vector<PhysicalRegId> calleeSaved = { asmjit::x86::rbx.id(),
                                                   asmjit::x86::r12.id(),
                                                   asmjit::x86::r13.id(),
                                                   asmjit::x86::r14.id(),
                                                   asmjit::x86::r15.id() };

        setStackReg(asmjit::x86::rsp.id());
        setStackFrame(asmjit::x86::rbp.id());

        setCallerSavedRegs(callerSaved);
        setCalleeSavedRegs(calleeSaved);
    }

    const char *getName() const override { return "X64"; }

    ArgLocation getArgLoc(size_t id, MirType *type) const override
    {
        ArgLocation loc;

        // System V AMD64 Argument passing order for integers/pointers
        static const uint32_t argRegs[] = { asmjit::x86::rdi.id(), asmjit::x86::rsi.id(), asmjit::x86::rdx.id(),
                                            asmjit::x86::rcx.id(), asmjit::x86::r8.id(),  asmjit::x86::r9.id() };

        // Map the first 6 arguments to their respective physical registers
        if (id < 6)
        {
            loc.setLoc(PhysicalRegLocation(static_cast<PhysicalRegId>(argRegs[id]), 64));
            return loc;
        }

        // Arguments 7+ fall back to the stack.
        // We subtract 6 so the first stack argument starts at offset 0.
        // The StackFrameLowerer will automatically add 16 bytes to this
        // to skip over the pushed Return Address (8 bytes) and pushed RBP (8 bytes).
        // Resulting physical address for arg 7: [RBP + 16 + 0]
        // Resulting physical address for arg 8: [RBP + 16 + 8]
        size_t stackArgIndex = id - 6;
        loc.setLoc(StackLocation(stackArgIndex * 8));

        return loc;
    }

    size_t getAbiAlignment(MirType *type) const
    {
        if (!type)
            return 1;
        size_t size = type->getTotalSizeInBytes();
        size_t align = size;
        if (align > 0 && (align & (align - 1)) != 0)
        {
            size_t p = 1;
            while (p < align)
                p <<= 1;
            align = p;
        }
        if (align == 0)
            align = 1;
        return align;
    }

    size_t getPreferredAlignment(MirType *type) const { return getAbiAlignment(type); }
};

// --- Test Fixture ---
class X64FlowTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;

    X64ABIDesc abi;
    TargetDesc *targetDesc;
    MirPassManager *passManager;

    // Legalizer
    x64TypeLegalizer typeLegalizer;
    LegalizerContext *legalizerCtx;

    TargetAbiLowererContext *abiLowererContext;
    RegisterAllocatorContext *raCtx;

    StackFrameLowererContext *stackFrameLowererContext;

    // Instruction Selector
    x64InstrSelector instrSelector;
    std::shared_ptr<InstructionSelectionTable> selectionTable;
    InstructionSelectionContext *instrSelectorCtx;

    std::shared_ptr<LegalizerActionList> actionList;
    std::shared_ptr<LegalizerHandlerList> legalizerHandlerList;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());

        targetDesc = new TargetDesc(&abi, TargetEndianness::LittleEndian, "X64");
        passManager = new MirPassManager();

        // Setup Type Legalizer
        actionList = typeLegalizer.getActionList(emitterCtx->getTypes().get());
        legalizerHandlerList = typeLegalizer.getHandlerList(emitterCtx->getTypes().get());
        legalizerCtx = new LegalizerContext(targetDesc, actionList.get(), legalizerHandlerList.get(), emitter);
        abiLowererContext = new TargetAbiLowererContext(emitter, targetDesc);

        // Setup Instruction Selector
        selectionTable = instrSelector.getSelectionTable();
        instrSelectorCtx = new InstructionSelectionContext(selectionTable.get(), emitter, targetDesc);

        raCtx = new RegisterAllocatorContext(emitter, targetDesc);
        stackFrameLowererContext = new StackFrameLowererContext(emitter, targetDesc);

        ec->beginScope();
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete instrSelectorCtx;
        delete raCtx;
        delete legalizerCtx;
        delete passManager;
        delete targetDesc;
        delete emitter;
    }
};

// --- Test Case ---
TEST_F(X64FlowTests, FullX64Compilation)
{
    MirRegister *param1 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *param2 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *param3 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *param4 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *param5 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *param6 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *param7 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type()); // To mem

    MirFunction *func = emitterCtx->createFunction(emitterCtx->getTypes()->getInt8Type(), nullptr, "testFunc");
    func->appendParameter(param1, "param1");
    func->appendParameter(param2, "param2");
    func->appendParameter(param3, "param3");
    func->appendParameter(param4, "param4");
    func->appendParameter(param5, "param5");
    func->appendParameter(param6, "param6");
    func->appendParameter(param7, "param7");

    MirRegister *vreg1 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *vreg2 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirRegister *vreg3 = emitter->createVirtualRegister(emitterCtx->getTypes()->getInt64Type());
    MirInteger *imm10 = emitter->createImmediateInteger(emitterCtx->getTypes()->getInt64Type(), 10);
    MirInteger *imm20 = emitter->createImmediateInteger(emitterCtx->getTypes()->getInt64Type(), 20);

    emitter->emitMOV(vreg1, imm10);
    emitter->emitMOV(vreg2, imm20);
    emitter->emitADD(vreg3, vreg1);
    emitter->emitADD(vreg3, vreg2);
    emitter->emitSUB(vreg3, param1);
    emitter->emitSUB(vreg3, param2);
    emitter->emitSUB(vreg3, param3);
    emitter->emitSUB(vreg3, param4);
    emitter->emitSUB(vreg3, param5);
    emitter->emitSUB(vreg3, param6);
    emitter->emitSUB(vreg3, param7);
    emitter->emitRET(param7);

    passManager->addPass<TypeLegalizerPass>(legalizerCtx);
    passManager->run(emitterCtx->getFunctionList(), emitterCtx->getFunctionList()->begin(), passManager);
    passManager->clearAll();

    passManager->addPass<TargetAbiLowererPass>(abiLowererContext);
    passManager->run(emitterCtx->getFunctionList(), emitterCtx->getFunctionList()->begin(), passManager);
    passManager->clearAll();

    passManager->addPass<RegisterAllocatorPass>(raCtx);
    passManager->run(emitterCtx->getFunctionList(), emitterCtx->getFunctionList()->begin(), passManager);
    passManager->clearAll();

    passManager->addPass<StackFrameLowererPass>(stackFrameLowererContext);
    passManager->run(emitterCtx->getFunctionList(), emitterCtx->getFunctionList()->begin(), passManager);
    passManager->clearAll();

    passManager->addPass<InstructionSelectorPass>(instrSelectorCtx);
    passManager->run(emitterCtx->getFunctionList(), emitterCtx->getFunctionList()->begin(), passManager);
    passManager->clearAll();

    x64Emitter x64emitter(emitter);
    ASSERT_TRUE(x64emitter.load());

    CodeBuffer buffer;
    ASSERT_TRUE(x64emitter.emitFunc(func, &buffer));
}
