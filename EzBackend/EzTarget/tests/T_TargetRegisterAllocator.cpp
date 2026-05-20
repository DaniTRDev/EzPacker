/**
 * @file T_TargetRegisterAllocator.cpp
 * @brief Unit tests for RegisterAllocatorPass.
 */
#include <gtest/gtest.h>
#include <EzTarget.h>
#include <EzMir.h>
#include <TargetRegisterAllocator/RegisterAllocatorPass.h>
#include <TargetRegisterAllocator/RegisterAllocatorContext.h>
#include <filesystem>
#include <map>

// A configurable ABIDesc for mocking available registers
class AllocatorMockABIDesc : public ABIDesc
{
  public:
    AllocatorMockABIDesc() { setRegSizeInBits(64); }

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

    /**
     * Returns the preferred alignment for the given type.
     * Used for global variables to optimize CPU cache line fetching.
     */
    size_t getPreferredAlignment(MirType *type) const { return getAbiAlignment(type); }
};

class TargetRegisterAllocatorTests : public ::testing::Test
{
  protected:
    std::shared_ptr<ErrorCollector> ec;
    std::shared_ptr<SourceManager> sm;
    std::shared_ptr<MirEmitterContext> emitterCtx;
    MirEmitter *emitter;

    AllocatorMockABIDesc abi;
    TargetDesc *targetDesc;
    RegisterAllocatorContext *allocatorCtx;
    MirPassManager *passManager;

    void SetUp() override
    {
        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());

        // Default: 3 allocatable registers (1 caller saved, 2 callee saved)
        abi.setCallerSavedRegs({ 1 });
        abi.setCalleeSavedRegs({ 2, 3 });

        targetDesc = new TargetDesc(&abi, TargetEndianness::LittleEndian, "MockTarget");
        allocatorCtx = new RegisterAllocatorContext(emitter, targetDesc);

        passManager = new MirPassManager();
        passManager->addPass<RegisterAllocatorPass>(allocatorCtx);

        ec->beginScope();
    }

    void TearDown() override
    {
        ec->endScope(ErrorAction::Discard);
        delete passManager;
        delete allocatorCtx;
        delete targetDesc;
        delete emitter;
    }

    // Helper function to setup pass with varying number of registers
    void ReconfigureRegisters(std::vector<PhysicalRegId> callerSaved, std::vector<PhysicalRegId> calleeSaved)
    {
        TearDown();

        ec = std::make_shared<ErrorCollector>();
        sm = std::make_shared<SourceManager>(std::filesystem::current_path());
        emitterCtx = std::make_shared<MirEmitterContext>(ec, sm);
        emitter = new MirEmitter(emitterCtx.get());

        // Default: 3 allocatable registers (1 caller saved, 2 callee saved)
        abi.setCallerSavedRegs(callerSaved);
        abi.setCalleeSavedRegs(calleeSaved);

        targetDesc = new TargetDesc(&abi, TargetEndianness::LittleEndian, "MockTarget");
        allocatorCtx = new RegisterAllocatorContext(emitter, targetDesc);

        passManager = new MirPassManager();
        passManager->addPass<RegisterAllocatorPass>(allocatorCtx);

        ec->beginScope();
    }
};

TEST_F(TargetRegisterAllocatorTests, SimpleLinearAllocation)
{
    // Setup a function with sequential variable usage that shouldn't conflict heavily
    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(i64Type, nullptr, "testFunc");

    // v0 = 10
    MirRegister *v0 = emitter->createVirtualRegister(i64Type);
    MirInteger *imm10 = emitter->createImmediateInteger(i64Type, 10);
    emitter->emit(MirInstructionOpCode::MOV, { v0, imm10 });

    // v1 = v0 + 5
    MirRegister *v1 = emitter->createVirtualRegister(i64Type);
    MirInteger *imm5 = emitter->createImmediateInteger(i64Type, 5);
    emitter->emit(MirInstructionOpCode::ADD, { v1, v0, imm5 });

    // RET v1
    emitter->emit(MirInstructionOpCode::RET, { v1 });

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    // Run Register Allocator
    bool modified = passManager->run(funcList, it, passManager);
    EXPECT_TRUE(modified);

    // Verify virtual registers are now physical and valid
    EXPECT_FALSE(v0->isVirtual());
    EXPECT_FALSE(v1->isVirtual());

    // We only have registers {1, 2, 3} available.
    EXPECT_TRUE(v0->getRegId() == 1 || v0->getRegId() == 2 || v0->getRegId() == 3);
    EXPECT_TRUE(v1->getRegId() == 1 || v1->getRegId() == 2 || v1->getRegId() == 3);
}

TEST_F(TargetRegisterAllocatorTests, AllocationWithInterference)
{
    // Make only 2 physical registers available
    ReconfigureRegisters({ 1 }, { 2 });

    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(i64Type, nullptr, "testFunc");

    // All these variables will interfere with each other because they are all used at the end
    MirRegister *v0 = emitter->createVirtualRegister(i64Type);
    MirRegister *v1 = emitter->createVirtualRegister(i64Type);

    emitter->emit(MirInstructionOpCode::MOV, { v0, emitter->createImmediateInteger(i64Type, 1) });
    emitter->emit(MirInstructionOpCode::MOV, { v1, emitter->createImmediateInteger(i64Type, 2) });

    // v0 and v1 are live simultaneously here
    MirRegister *v2 = emitter->createVirtualRegister(i64Type);
    emitter->emit(MirInstructionOpCode::ADD, { v2, v0, v1 });

    emitter->emit(MirInstructionOpCode::RET, { v2 });

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    passManager->run(funcList, it, passManager);

    // They must get different physical registers
    EXPECT_FALSE(v0->isVirtual());
    EXPECT_FALSE(v1->isVirtual());
    EXPECT_FALSE(v2->isVirtual());
    EXPECT_NE(v0->getRegId(), v1->getRegId());

    // Since only {1, 2} exist, v0 and v1 must take them both.
    EXPECT_TRUE((v0->getRegId() == 1 && v1->getRegId() == 2) || (v0->getRegId() == 2 && v1->getRegId() == 1));
}

TEST_F(TargetRegisterAllocatorTests, ForcesSpillWhenNoRegistersAvailable)
{
    // Extremely constrained: only 1 physical register available
    ReconfigureRegisters({ 1 }, {});

    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(i64Type, nullptr, "testFunc");

    // Create 2 interfering registers
    MirRegister *v0 = emitter->createVirtualRegister(i64Type);
    MirRegister *v1 = emitter->createVirtualRegister(i64Type);

    emitter->emit(MirInstructionOpCode::MOV, { v0, emitter->createImmediateInteger(i64Type, 10) });
    emitter->emit(MirInstructionOpCode::MOV, { v1, emitter->createImmediateInteger(i64Type, 20) });

    // Use them together -> they overlap in liveness
    MirRegister *v2 = emitter->createVirtualRegister(i64Type);
    emitter->emit(MirInstructionOpCode::ADD, { v2, v0, v1 });
    emitter->emit(MirInstructionOpCode::RET, { v2 });

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    // Since K=1, but maximum overlap is 2 (v0 and v1 are live simultaneously), one MUST be spilled!
    // The allocator should handle spilling, rewrite the program, and color the rest.
    bool modified = passManager->run(funcList, it, passManager);
    EXPECT_TRUE(modified);

    // After rewriting and successful allocation, the program should have more instructions
    // due to STORE and LOAD for the spilled variable.
    // Original: 4 instructions
    // After Spill: at least 6 instructions (1 store + 1 load)
    EXPECT_GE(func->getEntryPoint()->getInstructions()->m_numElems, 6);

    // Check if LOAD/STORE instructions exist in the modified block
    bool foundLoad = false;
    bool foundStore = false;

    for (MirInstruction *inst : *func->getEntryPoint()->getInstructions())
    {
        if (inst->getOpCode() == MirInstructionOpCode::LOAD)
            foundLoad = true;
        if (inst->getOpCode() == MirInstructionOpCode::STORE)
            foundStore = true;
    }

    EXPECT_TRUE(foundLoad);
    EXPECT_TRUE(foundStore);

    // Ensure stack frame has spill slots created
    EXPECT_GE(func->getStackFrame()->getAllocatedObjectCount(), 1);
}

TEST_F(TargetRegisterAllocatorTests, PreservesPrecoloredRegisters)
{
    // Setup physical registers {1, 2, 3, 4}
    ReconfigureRegisters({ 1, 2 }, { 3, 4 });

    MirType *i64Type = emitterCtx->getIntegerTypeBySize(8);
    if (!i64Type)
    {
        i64Type = emitterCtx->createType(MirTypeKind::Integer, 8, nullptr, "i64");
    }

    MirFunction *func = emitterCtx->createFunction(i64Type, nullptr, "testFunc");
    // Pre-color v0 to Physical Register 3
    MirRegister *v0 = emitter->createPhysicalRegister(i64Type, 3);
    MirRegister *v1 = emitter->createVirtualRegister(i64Type);

    emitter->emit(MirInstructionOpCode::MOV, { v0, emitter->createImmediateInteger(i64Type, 10) });
    emitter->emit(MirInstructionOpCode::MOV, { v1, emitter->createImmediateInteger(i64Type, 20) });

    // Conflict v0 and v1
    MirRegister *v2 = emitter->createVirtualRegister(i64Type);
    emitter->emit(MirInstructionOpCode::ADD, { v2, v0, v1 });
    emitter->emit(MirInstructionOpCode::RET, { v2 });

    auto funcList = emitterCtx->getFunctionList();
    auto it = funcList->begin();

    passManager->run(funcList, it, passManager);

    // Verify v0 kept its original color
    EXPECT_FALSE(v0->isVirtual());
    EXPECT_EQ(v0->getRegId(), 3);

    // v1 and v2 should be allocated but not to 3 (due to interference)
    EXPECT_FALSE(v1->isVirtual());
    EXPECT_FALSE(v2->isVirtual());
    EXPECT_NE(v1->getRegId(), 3);
    EXPECT_NE(v2->getRegId(), 3);
}
