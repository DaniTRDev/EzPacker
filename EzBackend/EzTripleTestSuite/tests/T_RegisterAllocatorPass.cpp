#include "EzTripleTestSuite.h"
#include "../include/Verifiers/RegisterAllocatorPassVerifier.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"

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
    MirFunction *func = funcBuilder.build(t->getVoidType());

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

    MirRegisterAllocatorPass *pass =
            runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getRegisterAllocator(), getTargetDesc());

    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks());
}

// =========================================================================
// 2. MIXED INTEGER AND FLOATING POINT ALLOCATION (GPR vs FPR)
// =========================================================================
TEST_F(TestRegisterAllocatorPass, AllocateMixedGprAndFprRegisters)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *intReg0 = oBuilder.buildVReg(t->i32(), "intVal0");
    MirRegister *intReg1 = oBuilder.buildVReg(t->i32(), "intVal1");
    MirRegister *floatReg0 = oBuilder.buildVReg(t->f64(), "fpVal0");
    MirRegister *floatReg1 = oBuilder.buildVReg(t->f64(), "fpVal1");

    // Integer ops: MOV int0, 42 -> MOV int1, int0
    iBuilder.MOV(intReg0, oBuilder.buildInt(t->i32(), FlexInt(42, 32)));
    iBuilder.MOV(intReg1, intReg0);

    // Floating point ops (2-operand: fp1 = fp1 + fp0):
    // MOV fp0, 3.14159 -> MOV fp1, fp0 -> ADD fp1, fp0
    iBuilder.MOV(floatReg0, oBuilder.buildFloat(t->f64(), FlexFloat(3.14159)));
    iBuilder.MOV(floatReg1, floatReg0);
    iBuilder.ADD(floatReg1, floatReg0);

    MirRegisterAllocatorPass *pass =
            runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getRegisterAllocator(), getTargetDesc());

    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks());
}

// =========================================================================
// 3. CALLER-SAVED REGISTER CLOBBER INTERFERENCE (CALL)
// =========================================================================
TEST_F(TestRegisterAllocatorPass, AllocateAcrossCallInstruction)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

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

    MirRegisterAllocatorPass *pass =
            runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getRegisterAllocator(), getTargetDesc());

    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks());
}

// =========================================================================
// 4. REGISTER EXHAUSTION AND SPILL CODE INSERTION
// =========================================================================
TEST_F(TestRegisterAllocatorPass, ForceRegisterSpillingAndVerifyRewrite)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    // Allocate high volume of concurrent virtual registers to force spilling
    constexpr size_t numVRegs = 5;
    std::vector<MirRegister *> vregs;
    vregs.reserve(numVRegs);

    for (size_t i = 0; i < numVRegs; ++i)
    {
        MirRegister *v = oBuilder.buildVReg(t->i32(), std::format("spill_v{}", std::to_string(i)).c_str());
        vregs.push_back(v);
        iBuilder.MOV(v, oBuilder.buildInt(t->i32(), FlexInt(static_cast<int32_t>(i + 1), 32)));
    }

    // Force overlapping live ranges for all created registers by accumulating them
    // (2-operand ISA: accum = accum + vregs[i])
    MirRegister *accum = oBuilder.buildVReg(t->i32(), "accum");
    iBuilder.MOV(accum, oBuilder.buildInt(t->i32(), FlexInt(0, 32)));

    for (size_t i = 0; i < numVRegs; ++i)
    {
        iBuilder.ADD(accum, vregs[i]);
    }

    MirRegisterAllocatorPass *pass =
            runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getRegisterAllocator(), getTargetDesc());

    RegisterAllocatorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyNoVirtualRegistersRemain(func->getBlocks());
}