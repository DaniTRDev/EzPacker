#include "EzTripleTestSuite.h"

class TestCallAbiLowererPass : public MirTripleTestSuiteAsGtest
{
  public:
};

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

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredCall(func->getEntryPoint(), {});
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), nullptr);
}

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

    iBuilder.PUSH_ARG(callToken, arg32Reg);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, ret32Reg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: MOV physReg(R0), arg32Reg inserted before CALL
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg32Reg });
    // Verify Incoming Return: MOV ret32Reg, physReg(R0) inserted after CALL
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), ret32Reg);
}

TEST_F(TestCallAbiLowererPass, LowerDirectFprCall)
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
    MirRegister *argFpReg = oBuilder.buildVReg(t->f64(), "argFp");
    MirRegister *retFpReg = oBuilder.buildVReg(t->f64(), "callRetFp");

    iBuilder.PUSH_ARG(callToken, argFpReg);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, retFpReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: MOV physReg(XMM0), argFpReg
    verifier.verifyLoweredCall(func->getEntryPoint(), { argFpReg });
    // Verify Incoming Return: MOV retFpReg, physReg(XMM0)
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), retFpReg);
}

TEST_F(TestCallAbiLowererPass, LowerSplit128BitGprCall)
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
    MirRegister *arg128Reg = oBuilder.buildVReg(t->i128(), "arg128");
    MirRegister *ret128Reg = oBuilder.buildVReg(t->i128(), "callRet128");

    iBuilder.PUSH_ARG(callToken, arg128Reg);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, ret128Reg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: Splits into R0 and R1
    verifier.verifyLoweredCall(func->getEntryPoint(), { arg128Reg });
    // Verify Incoming Return: Splits back from R0 and R2
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), ret128Reg);
}

TEST_F(TestCallAbiLowererPass, LowerIndirect256BitCallWithSretReturn)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirType *hugeType = t->i256();

    MirRegister *callToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *calleeReg = oBuilder.buildVReg(t->getPtr(t->getVoidType()), "callee");
    MirRegister *valReg = oBuilder.buildVReg(hugeType, "hugeVal256");
    MirRegister *sretRetValReg = oBuilder.buildVReg(hugeType, "sretRetVal");

    iBuilder.PUSH_ARG(callToken, valReg);
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, sretRetValReg);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: SRET pointer passed implicitly
    verifier.verifyLoweredCall(func->getEntryPoint(), { valReg });
    // Verify SRET Return (>128 bits)
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), sretRetValReg);
}

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

    // ABI Caller Saved GPRs: R0, R1, R2, R6, R7, R8, R9, R10, R11 (Total 9)
    std::vector<MirOperand *> args;
    for (int i = 0; i < 10; ++i)
    {
        MirRegister *arg = oBuilder.buildVReg(t->i32(), std::format("arg{}", i).c_str());
        args.push_back(arg);
        iBuilder.PUSH_ARG(callToken, arg);
    }

    MirRegister *retInt = oBuilder.buildVReg(t->i32(), "retInt32");
    iBuilder.CALL(callToken, calleeReg);
    iBuilder.POP_RET(callToken, retInt);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    CallAbiLowererVerifier verifier(getBuilderCtx(), pass);
    // Verify Outgoing: First 9 go to GPRs, 10th pushes to Stack Slot
    verifier.verifyLoweredCall(func->getEntryPoint(), args);
    verifier.verifyLoweredCallReturn(func->getEntryPoint(), retInt);
}