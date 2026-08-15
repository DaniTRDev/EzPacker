#include "EzTripleTestSuite.h"

class TestRegisterAllocatorPass : public MirTripleTestSuiteAsGtest
{
  public:
};

// =========================================================================
// 1. SIMPLE VIRTUAL REGISTER ALLOCATION (NO SPILLS)
// =========================================================================
TEST_F(TestRegisterAllocatorPass, AllocateBasicVirtualRegisters)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType(), "AllocateBasicVirtualRegisters");

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *v0 = oBuilder.buildVReg(t->i32(), "v0");
    MirRegister *v1 = oBuilder.buildVReg(t->i32(), "v1");

    // Sequence (2-operand ISA: v0 = v0 + v1):
    // MOV %v0, 10
    // MOV %v1, 20
    // ADD %v0, %v1
    iBuilder.MOV(v0, oBuilder.buildInt(t->i32(), FlexInt(10, 32)));
    iBuilder.MOV(v1, oBuilder.buildInt(t->i32(), FlexInt(20, 32)));
    iBuilder.ADD(v0, v1);

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetBinaryDesc(), getTargetDesc());
    MirRegisterAllocatorPass *pass = runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getTargetDesc());

    RegisterAllocatorCtx *ctx = pass->getResult().m_contexts.at(func);
    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks())
            .verifyAllocationMappingComplete(ctx)
            .verifyNoInterferenceConflicts(ctx)
            .verifyReservedRegistersNotAssigned(ctx)
            .verifyFramePointerReservedOnDAlloc(ctx);
}

// =========================================================================
// 2. MIXED INTEGER AND FLOATING POINT ALLOCATION (GPR vs FPR)
// =========================================================================
TEST_F(TestRegisterAllocatorPass, AllocateMixedGprAndFprRegisters)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType(), "AllocateMixedGprAndFprRegisters");

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *int32Reg = oBuilder.buildVReg(t->i32(), "i32Val");
    MirRegister *int64SmallReg = oBuilder.buildVReg(t->i64(), "i64SmallVal");
    MirRegister *int64LargeReg = oBuilder.buildVReg(t->i64(), "i64LargeVal");

    MirRegister *fp32Reg = oBuilder.buildVReg(t->f32(), "f32Val");
    MirRegister *fp64Reg = oBuilder.buildVReg(t->f64(), "f64Val");

    // 32-bit Immediate -> Lowers to: MOV32ri
    iBuilder.MOV(int32Reg, oBuilder.buildInt(t->i32(), FlexInt(42, 32)));

    // 64-bit Small Immediate (fits in signed 32-bit) -> Lowers to: MOV64ri32
    iBuilder.MOV(int64SmallReg, oBuilder.buildInt(t->i64(), FlexInt(1000, 64)));

    // 64-bit Large Immediate (requires full 64-bit imm) -> Lowers to: MOVABS64ri
    iBuilder.MOV(int64LargeReg, oBuilder.buildInt(t->i64(), FlexInt(uint64_t(0x1122334455667788ULL), 64)));

    // 32-bit Float Immediate -> Lowers to: MOV32ri (to temp GPR32) -> MOVDtoFPR
    iBuilder.MOV(fp32Reg, oBuilder.buildFloat(t->f32(), FlexFloat(3.14159f)));

    // 64-bit Float Immediate -> Lowers to: MOVABS64ri (to temp GPR64) -> MOVQtoFPR
    iBuilder.MOV(fp64Reg, oBuilder.buildFloat(t->f64(), FlexFloat(3.141592653589793)));

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetBinaryDesc(), getTargetDesc());
    MirRegisterAllocatorPass *pass = runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getTargetDesc());

    RegisterAllocatorCtx *ctx = pass->getResult().m_contexts.at(func);
    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks())
            .verifyAllocationMappingComplete(ctx)
            .verifyNoInterferenceConflicts(ctx)
            .verifyReservedRegistersNotAssigned(ctx)
            .verifyFramePointerReservedOnDAlloc(ctx);
}

// =========================================================================
// 3. CALLER-SAVED REGISTER CLOBBER INTERFERENCE (CALL)
// =========================================================================
TEST_F(TestRegisterAllocatorPass, AllocateAcrossCallInstruction)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType(), "AllocateAcrossCallInstruction");

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *liveAcross = oBuilder.buildVReg(t->i32(), "liveAcrossCall");
    MirRegister *calleeTarget = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");

    // Sequence (2-operand ISA: liveAcross = liveAcross + 50):
    // MOV %liveAcross, 100
    // CALL %calleeTarget
    // ADD %liveAcross, 50
    iBuilder.MOV(liveAcross, oBuilder.buildInt(t->i32(), FlexInt(100, 32)));
    iBuilder.CALL(calleeTarget);
    iBuilder.ADD(liveAcross, oBuilder.buildInt(t->i32(), FlexInt(50, 32)));

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetBinaryDesc(), getTargetDesc());
    MirRegisterAllocatorPass *pass = runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getTargetDesc());

    RegisterAllocatorCtx *ctx = pass->getResult().m_contexts.at(func);
    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks())
            .verifyAllocationMappingComplete(ctx)
            .verifyNoInterferenceConflicts(ctx)
            .verifyReservedRegistersNotAssigned(ctx)
            .verifyFramePointerReservedOnDAlloc(ctx);
}

// =========================================================================
// 4. REMATERIALIZATION OPTIMIZATION (IMMEDIATES REMATERIALIZED INLINE)
// =========================================================================
TEST_F(TestRegisterAllocatorPass, ForceRegisterSpillingAndVerifyRematerialization)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType(), "ForceRematerializationTest");

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    constexpr size_t numVRegs = 5;
    std::vector<MirRegister *> vregs;
    vregs.reserve(numVRegs);

    for (size_t i = 0; i < numVRegs; ++i)
    {
        MirRegister *v = oBuilder.buildVReg(t->i32(), std::format("spill_v{}", std::to_string(i)).c_str());
        vregs.push_back(v);
        iBuilder.MOV(v, oBuilder.buildInt(t->i32(), FlexInt(static_cast<int32_t>(i + 1), 32)));
    }

    MirRegister *accum = oBuilder.buildVReg(t->i32(), "accum");
    iBuilder.MOV(accum, oBuilder.buildInt(t->i32(), FlexInt(0, 32)));

    for (size_t i = 0; i < numVRegs; ++i)
    {
        iBuilder.ADD(accum, vregs[i]);
    }

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetBinaryDesc(), getTargetDesc());
    MirRegisterAllocatorPass *pass = runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getTargetDesc());

    RegisterAllocatorCtx *ctx = pass->getResult().m_contexts.at(func);
    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks())
            .verifyAllocationMappingComplete(ctx)
            .verifyNoInterferenceConflicts(ctx)
            .verifyReservedRegistersNotAssigned(ctx)
            .verifyFramePointerReservedOnDAlloc(ctx);

    // Verify rematerialization optimization: zero stack spill slots created!
    EXPECT_EQ(func->getStackFrame()->getObjects().size(), 0);
}

// =========================================================================
// 5. STACK MEMORY SPILLING (NON-REMATERIALIZABLE COMPUTED VALUES)
// =========================================================================
TEST_F(TestRegisterAllocatorPass, ForceMemorySpillingForNonRematerializableValues)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    // Return i32 so accum is live at the end of the function!
    MirFunction *func = funcBuilder.build(t->i32(), "ForceStackSpillTest");

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    // 1. Non-rematerializable base value
    MirRegister *baseVal = oBuilder.buildVReg(t->i32(), "baseVal");
    iBuilder.MOV(baseVal, oBuilder.buildInt(t->i32(), FlexInt(10, 32)));
    iBuilder.ADD(baseVal, oBuilder.buildInt(t->i32(), FlexInt(5, 32)));

    // 2. Generate 20 live variables that depend on baseVal
    constexpr size_t numVRegs = 20;
    std::vector<MirRegister *> vregs;
    vregs.reserve(numVRegs);

    for (size_t i = 0; i < numVRegs; ++i)
    {
        MirRegister *v = oBuilder.buildVReg(t->i32(), std::format("dyn_v{}", i).c_str());
        vregs.push_back(v);
        iBuilder.MOV(v, baseVal);
        iBuilder.ADD(v, oBuilder.buildInt(t->i32(), FlexInt(static_cast<int32_t>(i + 1), 32)));
    }

    // 3. Accumulate them all
    MirRegister *accum = oBuilder.buildVReg(t->i32(), "accum");
    iBuilder.MOV(accum, oBuilder.buildInt(t->i32(), FlexInt(0, 32)));

    for (size_t i = 0; i < numVRegs; ++i)
    {
        iBuilder.ADD(accum, vregs[i]);
    }

    iBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetBinaryDesc(), getTargetDesc());
    MirRegisterAllocatorPass *pass = runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getTargetDesc());

    RegisterAllocatorCtx *ctx = pass->getResult().m_contexts.at(func);
    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks())
            .verifyAllocationMappingComplete(ctx)
            .verifyNoInterferenceConflicts(ctx)
            .verifyReservedRegistersNotAssigned(ctx)
            .verifyFramePointerReservedOnDAlloc(ctx);

    EXPECT_GT(func->getStackFrame()->getObjects().size(), 0);
}

// =========================================================================
// 6. FRAME POINTER RESERVATION ON DYNAMIC ALLOCATION (DALLOC)
// =========================================================================
TEST_F(TestRegisterAllocatorPass, ForceFramePointerReservationOnDAlloc)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType(), "ForceFramePointerReservationOnDAlloc");

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *dynPtr = oBuilder.buildVReg(t->getPtr(t->i32()), "dynPtr");
    MirOperand *allocSize = oBuilder.buildInt(t->i32(), FlexInt(64, 32));

    // Emit DALLOC instruction
    iBuilder.DALLOC(dynPtr, allocSize);

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetBinaryDesc(), getTargetDesc());
    MirRegisterAllocatorPass *pass = runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getTargetDesc());

    RegisterAllocatorCtx *ctx = pass->getResult().m_contexts.at(func);
    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks())
            .verifyAllocationMappingComplete(ctx)
            .verifyNoInterferenceConflicts(ctx)
            .verifyReservedRegistersNotAssigned(ctx)
            .verifyFramePointerReservedOnDAlloc(ctx);

    EXPECT_TRUE(func->getCallingConv()->hasFramePointer(func));
}