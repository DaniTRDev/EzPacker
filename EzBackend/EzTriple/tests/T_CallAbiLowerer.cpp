#include "EzTripleTestSuite.h"
#include "CallAbiLowererVerifier.h"

class TestCallAbiLowererPass : public MirTripleTestSuiteAsGtest
{
  public:
};

// =========================================================================
// 1. NO ARGUMENT CALL TEST (VOID RETURN)
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

    // CALL callToken, calleeReg
    iBuilder.CALL(callToken, calleeReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: CALL operands cleared, zero argument prep instructions generated, zero return extraction
    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), {});
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), nullptr);
}

// =========================================================================
// 2. RULE 1B: 32-BIT DIRECT REGISTER RULE + GPR RETURN (32-Bit Return -> GPR 1)
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
    MirRegister *arg32Reg = oBuilder.buildVReg(t->i32(), "arg32");
    MirRegister *ret32Reg = oBuilder.buildVReg(t->i32(), "callRet32");

    // Legalized sequence: PUSH_ARG callToken, arg32Reg -> CALL callToken, calleeReg -> POP_RET callToken, ret32Reg
    iBuilder.PUSH_ARG(callToken, arg32Reg);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, ret32Reg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: MOV physReg(GPR 1), arg32Reg inserted before CALL
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg32Reg });
    // Verify Incoming Return: MOV ret32Reg, physReg(GPR 1) inserted after CALL
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), ret32Reg);
}

// =========================================================================
// 3. RULE 1A: 64-BIT SPLIT RULE + FPR RETURN (f64 Return -> FPR 4)
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerSplit64BitGprCallWithFpReturn)
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
    MirRegister *arg64Reg = oBuilder.buildVReg(t->i64(), "arg64");
    MirRegister *retFpReg = oBuilder.buildVReg(t->f64(), "callRetFp");

    // 8-byte value splits into GPR 1 and GPR 2. Return value is f64 (FPR 4).
    iBuilder.PUSH_ARG(callToken, arg64Reg);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, retFpReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: LOAD physReg(1), [arg64Reg+0] -> LOAD physReg(2), [arg64Reg+4] -> CALL
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg64Reg });
    // Verify Incoming Return: MOV retFpReg, physReg(FPR 4)
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), retFpReg);
}

// =========================================================================
// 4. RULE 1C: 256-BIT INDIRECT STRUCT + INDIRECT SRET RETURN (>64 bits -> GPR 1)
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerIndirect256BitStructCallWithSretReturn)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirType *largeStructType = t->i256();

    MirRegister *callToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");
    MirRegister *structValReg = oBuilder.buildVReg(largeStructType, "largeStruct256");
    MirRegister *sretRetValReg = oBuilder.buildVReg(largeStructType, "sretRetVal");

    iBuilder.PUSH_ARG(callToken, structValReg);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, sretRetValReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Rule 1C Outgoing: STORE stackCopy, structValReg -> MOV physReg(GPR 1), stackCopyAddr -> CALL
    verifier.verifyLoweredCall(func->getEntryPoint(), { structValReg });
    // Verify SRET Return (>64 bits): MOV sretRetValReg, physReg(GPR 1)
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), sretRetValReg);
}

// =========================================================================
// 5. RULE 2: PARITY INTERLEAVING RULE + GPR RETURN
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerParityInterleavingRuleWithReturn)
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

    MirRegister *arg0 = oBuilder.buildVReg(t->i16(), "arg0_stack");
    MirRegister *arg1 = oBuilder.buildVReg(t->i16(), "arg1_stack");
    MirRegister *arg2 = oBuilder.buildVReg(t->i32(), "arg2_gpr1");
    MirRegister *arg3 = oBuilder.buildVReg(t->i16(), "arg3_gpr2");
    MirRegister *retVal = oBuilder.buildVReg(t->i16(), "retVal16");

    iBuilder.PUSH_ARG(callToken, arg0);
    iBuilder.PUSH_ARG(callToken, arg1);
    iBuilder.PUSH_ARG(callToken, arg2);
    iBuilder.PUSH_ARG(callToken, arg3);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, retVal);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: Stack STOREs for arg0/arg1, MOVs to GPRs for arg2/arg3
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg0, arg1, arg2, arg3 });
    // Verify Incoming Return: MOV retVal, physReg(GPR 1)
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), retVal);
}

// =========================================================================
// 6. RULE 3: FLOATING-POINT FALLBACK RULE + FPR RETURN
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerFloatingPointFallbackRuleWithReturn)
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

    MirRegister *gprArg = oBuilder.buildVReg(t->i32(), "gprArg");
    MirRegister *fprArg = oBuilder.buildVReg(t->f64(), "fprArg");
    MirRegister *retFp = oBuilder.buildVReg(t->f64(), "retFp64");

    iBuilder.PUSH_ARG(callToken, gprArg);
    iBuilder.PUSH_ARG(callToken, fprArg);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, retFp);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: MOV physReg(GPR 1), gprArg -> MOV physReg(FPR 4), fprArg -> CALL
    verifier.verifyLoweredCall(func->getEntryPoint(), { gprArg, fprArg });
    // Verify Incoming Return: MOV retFp, physReg(FPR 4)
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), retFp);
}

// =========================================================================
// 7. RULE 4: REGISTER EXHAUSTION SPILL RULE + GPR RETURN
// =========================================================================
TEST_F(TestCallAbiLowererPass, LowerRegisterExhaustionSpillRuleWithReturn)
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

    MirRegister *arg0 = oBuilder.buildVReg(t->i32(), "arg0_gpr1");
    MirRegister *arg1 = oBuilder.buildVReg(t->i16(), "arg1_gpr2");
    MirRegister *arg2 = oBuilder.buildVReg(t->i16(), "arg2_parity_stack");
    MirRegister *arg3 = oBuilder.buildVReg(t->i32(), "arg3_exhausted_stack");
    MirRegister *retInt = oBuilder.buildVReg(t->i32(), "retInt32");

    iBuilder.PUSH_ARG(callToken, arg0);
    iBuilder.PUSH_ARG(callToken, arg1);
    iBuilder.PUSH_ARG(callToken, arg2);
    iBuilder.PUSH_ARG(callToken, arg3);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, retInt);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: GPR assignments for arg0/arg1, Stack Slot STOREs for arg2/arg3
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg0, arg1, arg2, arg3 });
    // Verify Incoming Return: MOV retInt, physReg(GPR 1)
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), retInt);
}