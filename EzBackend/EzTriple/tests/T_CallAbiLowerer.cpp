#include "EzTripleTestSuite.h"
#include "CallAbiLowererVerifier.h"

class TestCallAbiLowererPass : public MirTripleTestSuiteAsGtest
{
  public:
};

// =========================================================================
// 1. NO ARGUMENT CALL TEST
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerNoArgCall)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *callToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");

    iBuilder.CALL(callToken, calleeReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: CALL operands cleared, zero argument prep instructions generated
    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), {});
}

// =========================================================================
// 2. RULE 1B: 32-BIT DIRECT REGISTER RULE (4 Bytes -> Single GPR)
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerDirect32BitGprCall)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *callToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");
    MirRegister *arg32Reg  = oBuilder.buildVReg(t->i32(), "arg32");

    // Legalized 1-arg 32-bit call: PUSH_ARG callToken, arg32Reg -> CALL callToken, calleeReg
    iBuilder.PUSH_ARG(callToken, arg32Reg);
    iBuilder.CALL(callToken, calleeReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: PUSH_ARG erased, MOV physReg(GPR 1), arg32Reg inserted before CALL
    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg32Reg });
}

// =========================================================================
// 3. RULE 1A: 64-BIT SPLIT RULE (8 Bytes -> Two 32-bit GPRs)
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerSplit64BitGprCall)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *callToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");
    MirRegister *arg64Reg  = oBuilder.buildVReg(t->i64(), "arg64");

    // 8-byte value triggers Rule 1A: splits into GPR 1 (offset 0) and GPR 2 (offset 4)
    iBuilder.PUSH_ARG(callToken, arg64Reg);
    iBuilder.CALL(callToken, calleeReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: LOAD physReg(1), [arg64Reg + 0] -> LOAD physReg(2), [arg64Reg + 4] -> CALL
    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg64Reg });
}

// =========================================================================
// 4. RULE 1C: 256-BIT INDIRECT / BYVAL STRUCT RULE (32 Bytes -> Pointer)
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerIndirect256BitStructCall)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    // Build a 32-byte (256-bit) type
    MirType *largeStructType = t->i256(); 

    MirRegister *callToken    = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg    = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");
    MirRegister *structValReg = oBuilder.buildVReg(largeStructType, "largeStruct256");

    iBuilder.PUSH_ARG(callToken, structValReg);
    iBuilder.CALL(callToken, calleeReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify Rule 1C: STORE stackCopy, structValReg -> MOV physReg(GPR 1), stackCopyAddr -> CALL
    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), { structValReg });
}

// =========================================================================
// 5. RULE 2: PARITY INTERLEAVING RULE (Even Total Allocations -> Stack Slot)
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerParityInterleavingRule)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *callToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");

    // Arg 0 (16-bit): Initial usedRegs = 0 (EVEN). Rule 2 forces Arg 0 directly to Stack Slot.
    // Arg 1 (16-bit): UsedRegs = 0 (still EVEN, no reg allocated for Arg 0). Forces Arg 1 to Stack Slot.
    // Arg 2 (32-bit): Size 4 triggers Rule 1B -> allocates GPR 1. UsedRegs becomes 1 (ODD).
    // Arg 3 (16-bit): UsedRegs = 1 (ODD). Falls through to Rule 3 -> allocates GPR 2.
    MirRegister *arg0 = oBuilder.buildVReg(t->i16(), "arg0_stack");
    MirRegister *arg1 = oBuilder.buildVReg(t->i16(), "arg1_stack");
    MirRegister *arg2 = oBuilder.buildVReg(t->i32(), "arg2_gpr1");
    MirRegister *arg3 = oBuilder.buildVReg(t->i16(), "arg3_gpr2");

    iBuilder.PUSH_ARG(callToken, arg0);
    iBuilder.PUSH_ARG(callToken, arg1);
    iBuilder.PUSH_ARG(callToken, arg2);
    iBuilder.PUSH_ARG(callToken, arg3);
    iBuilder.CALL(callToken, calleeReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: Stack STOREs for arg0/arg1, MOVs to GPRs for arg2/arg3
    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg0, arg1, arg2, arg3 });
}

// =========================================================================
// 6. RULE 3: FLOATING-POINT FALLBACK RULE (FPR Allocation)
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerFloatingPointFallbackRule)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *callToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");

    // Arg 0 (32-bit): Size 4 -> GPR 1 (UsedRegs becomes 1, ODD)
    // Arg 1 (f64): UsedRegs = 1 (ODD). Triggers Rule 3 FP fallback -> allocates FPR 4
    MirRegister *gprArg = oBuilder.buildVReg(t->i32(), "gprArg");
    MirRegister *fprArg = oBuilder.buildVReg(t->f64(), "fprArg");

    iBuilder.PUSH_ARG(callToken, gprArg);
    iBuilder.PUSH_ARG(callToken, fprArg);
    iBuilder.CALL(callToken, calleeReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: MOV physReg(GPR 1), gprArg -> MOV physReg(FPR 4), fprArg -> CALL
    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), { gprArg, fprArg });
}

// =========================================================================
// 7. RULE 4: REGISTER EXHAUSTION SPILL RULE
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerRegisterExhaustionSpillRule)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *callToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");

    // GPR volatile pool has only 2 registers {1, 2}.
    // Arg 0 (32-bit): GPR 1 (UsedRegs = 1)
    // Arg 1 (16-bit): UsedRegs = 1 (ODD) -> GPR 2 (UsedRegs = 2)
    // Arg 2 (16-bit): UsedRegs = 2 (EVEN) -> Rule 2 forces Stack Slot (UsedRegs stays 2)
    // Arg 3 (32-bit): GPR pool exhausted ({1, 2} used) -> Rule 4 forces Stack Slot
    MirRegister *arg0 = oBuilder.buildVReg(t->i32(), "arg0_gpr1");
    MirRegister *arg1 = oBuilder.buildVReg(t->i16(), "arg1_gpr2");
    MirRegister *arg2 = oBuilder.buildVReg(t->i16(), "arg2_parity_stack");
    MirRegister *arg3 = oBuilder.buildVReg(t->i32(), "arg3_exhausted_stack");

    iBuilder.PUSH_ARG(callToken, arg0);
    iBuilder.PUSH_ARG(callToken, arg1);
    iBuilder.PUSH_ARG(callToken, arg2);
    iBuilder.PUSH_ARG(callToken, arg3);
    iBuilder.CALL(callToken, calleeReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: Physical GPR assignments for arg0/arg1, outgoing Stack Slot STOREs for arg2/arg3
    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg0, arg1, arg2, arg3 });
}