#include "EzTripleTestSuite.h"

class TestReturnAbiLowererPass : public MirTripleTestSuiteAsGtest
{
  public:
};

TEST_F(TestReturnAbiLowererPass, LowerVoidReturn)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    // Legalized void return sequence: RET retToken
    MirRegister *retToken = oBuilder.buildVReg(t->getBindingToken());
    iBuilder.RET(retToken);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: PUSH_RET erased, RET standardized to 0 operands
    ReturnAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredReturn(func->getEntryPoint(), {});
}

TEST_F(TestReturnAbiLowererPass, LowerDirectGprReturn)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->i32());
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *retToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *valReg = oBuilder.buildVReg(t->i32(), "retVal");

    // Legalized direct return: PUSH_RET retToken, retVal -> RET retToken
    iBuilder.PUSH_RET(retToken, valReg);
    iBuilder.RET(retToken);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: PUSH_RET erased, MOV physReg(1), retVal inserted before RET
    ReturnAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredReturn(func->getEntryPoint(), { valReg });
}

// =========================================================================
// 3. DIRECT FPR RETURN TEST (f64 -> FPR 4)
// =========================================================================
TEST_F(TestReturnAbiLowererPass, LowerDirectFprReturn)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->f64());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *retToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *fpValReg = oBuilder.buildVReg(t->f64(), "fpRetVal");

    iBuilder.PUSH_RET(retToken, fpValReg);
    iBuilder.RET(retToken);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: MOV physReg(4), fpRetVal inserted before RET
    ReturnAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredReturn(func->getEntryPoint(), { fpValReg });
}

TEST_F(TestReturnAbiLowererPass, LowerIndirectSretReturn)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder oBuilder(getBuilderCtx());
    MirFunctionBuilder funcBuilder(getBuilderCtx());

    MirType *largeType = t->i128(); // Exceeds 64 bits -> requires SRET per TestCallingConvention
    MirType *sretPtrType = t->getPtr(largeType);

    // Param 0: Implicit SRET pointer
    MirRegister *sretPtrParam = oBuilder.buildVReg(sretPtrType, "sretPtr");
    funcBuilder.buildParam(sretPtrParam);
    MirFunction *func = funcBuilder.build(largeType);

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *retToken = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *largeValReg = oBuilder.buildVReg(largeType, "largeVal");

    // Post-LegalizeReturnAction state:
    // STORE ptr[sretPtrParam], largeValReg
    // PUSH_RET retToken, sretPtrParam
    // RET retToken
    MirOperand *memDest = oBuilder.buildMem(largeType, sretPtrParam, FlexInt(0));
    iBuilder.STORE(memDest, largeValReg);
    iBuilder.PUSH_RET(retToken, sretPtrParam);
    iBuilder.RET(retToken);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: PUSH_RET erased, STORE confirmed, and copyOnReg (GPR 1) handled if requested
    ReturnAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredReturn(func->getEntryPoint(), { largeValReg });
}

TEST_F(TestReturnAbiLowererPass, LowerMultipleReturnBlocks)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder oBuilder(getBuilderCtx());
    MirFunctionBuilder funcBuilder(getBuilderCtx());

    MirFunction *func = funcBuilder.build(t->i32());

    // Block 1: True branch exit
    MirBlock *thenBlock = func->getEntryPoint();
    MirInstructionBuilder thenBuilder(getBuilderCtx(),
                                      thenBlock,
                                      InsertionType::InsertAfter,
                                      thenBlock->getInstructions().begin());

    MirRegister *token1 = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *val1 = oBuilder.buildVReg(t->i32(), "valThen");
    thenBuilder.PUSH_RET(token1, val1);
    thenBuilder.RET(token1);

    // Block 2: False branch exit
    MirBlock *elseBlock = funcBuilder.blockBuilder().build(nullptr, "elseBlock");
    MirInstructionBuilder elseBuilder(getBuilderCtx(),
                                      elseBlock,
                                      InsertionType::InsertAfter,
                                      elseBlock->getInstructions().begin());

    MirRegister *token2 = oBuilder.buildVReg(t->getBindingToken());
    MirRegister *val2 = oBuilder.buildVReg(t->i32(), "valElse");
    elseBuilder.PUSH_RET(token2, val2);
    elseBuilder.RET(token2);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify both blocks independently
    ReturnAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredReturn(thenBlock, { val1 });
    verifier.verifyLoweredReturn(elseBlock, { val2 });
}