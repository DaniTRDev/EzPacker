#include "EzTripleTestSuite.h"

class TestFunctionArgAbiLowererPass : public MirTripleTestSuiteAsGtest
{
  public:
};

TEST_F(TestFunctionArgAbiLowererPass, LowerZeroFunctionArguments)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->i8(), "testFuncNoArgs");

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: Entry block remains clean, zero parameter prep instructions generated
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), {});
}

TEST_F(TestFunctionArgAbiLowererPass, LowerDirect32BitGprFunctionArgument)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    MirRegister *param32 = opBuilder.buildVReg(t->i32(), "param32");

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFunc32Bit");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());
    iBuilder.POP_ARG(token, param32);
    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: MOV param32, physReg(R0) generated at top of entry block
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), { param32 });
}

TEST_F(TestFunctionArgAbiLowererPass, LowerDirectFprFunctionArgument)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    MirRegister *paramFp = opBuilder.buildVReg(t->f64(), "paramFp");

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFuncFp");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());
    iBuilder.POP_ARG(token, paramFp);
    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: MOV paramFp, physReg(XMM0)
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), { paramFp });
}

TEST_F(TestFunctionArgAbiLowererPass, LowerSplit128BitFunctionArgument)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    MirRegister *param128 = opBuilder.buildVReg(t->i128(), "param128");

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFunc128BitSplit");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());
    iBuilder.POP_ARG(token, param128);
    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: 128-bit splits into two 64-bit GPR chunks (R0, R1)
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), { param128 });
}

TEST_F(TestFunctionArgAbiLowererPass, LowerRegisterExhaustionFunctionArguments)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFuncExhaustion");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());

    // Allocate 10 arguments. ABI has 9 Caller-Saved GPRs available.
    std::vector<MirOperand *> args;
    for (int i = 0; i < 10; ++i)
    {
        MirRegister *arg = opBuilder.buildVReg(t->i64(), std::format("arg{}", i).c_str());
        args.push_back(arg);
        iBuilder.POP_ARG(token, arg);
    }

    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: 9 parameters extract from physRegs, 10th extracts from incoming Stack Frame Slot
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), args);
}