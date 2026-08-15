#include "EzTripleTestSuite.h"

class TestFrameLowererPass : public MirTripleTestSuiteAsGtest
{
  public:
};

TEST_F(TestFrameLowererPass, LowerEmptyLeafFunction)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    iBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyPrologue(func);
    verifier.verifyEpilogue(func);
    verifier.verifyStackReferencesLowered(func);
}

TEST_F(TestFrameLowererPass, LowerFunctionWithLocalVariablesAndSpills)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirFunctionStackFrame *frame = func->getStackFrame();
    ASSERT_NE(frame, nullptr);

    StackFrameObject *var0 = frame->createStaticStackObj(t->i32());
    StackFrameObject *var1 = frame->createStaticStackObj(t->i64());
    StackFrameObject *spill0 = frame->createStackSpill(t->f64());

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *vReg1 = oBuilder.buildVReg(t->i32(), "val32");
    MirRegister *vReg2 = oBuilder.buildVReg(t->f64(), "valFp");

    MirOperand *stackRefVar0 = oBuilder.buildRef(var0);
    MirOperand *stackRefSpill0 = oBuilder.buildRef(spill0);

    iBuilder.STORE(stackRefVar0, vReg1);
    iBuilder.STORE(stackRefSpill0, vReg2);
    iBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    EXPECT_GT(func->getAnalysisData()->m_totalFrameSize, 0u);

    verifier.verifyPrologue(func);
    verifier.verifyEpilogue(func);
    verifier.verifyStackReferencesLowered(func);
}

TEST_F(TestFrameLowererPass, LowerCalleeSavedRegisters)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    CallingConvDesc *cc = func->getCallingConv();
    ASSERT_NE(cc, nullptr);

    const auto &calleeSavedList = cc->getAllCalleeSavedRegs();
    ASSERT_FALSE(calleeSavedList.empty());

    // Register R3 (Callee-saved non-FP)
    RegisterRef reg1 = calleeSavedList.front();
    func->addCalleeSavedRegUse(reg1);

    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());
    iBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    EXPECT_GT(func->getAnalysisData()->m_calleeSavedAreaSize, 0u);

    verifier.verifyPrologue(func);
    verifier.verifyEpilogue(func);
    verifier.verifyStackReferencesLowered(func);
}

TEST_F(TestFrameLowererPass, LowerMultipleReturnBlocks)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType());

    MirBlock *entryBlock = func->getEntryPoint();
    MirBlock *thenBlock = funcBuilder.blockBuilder().build(nullptr, "then_block");
    MirBlock *elseBlock = funcBuilder.blockBuilder().build(nullptr, "else_block");

    MirInstructionBuilder entryBuilder(getBuilderCtx(),
                                       entryBlock,
                                       InsertionType::InsertAfter,
                                       entryBlock->getInstructions().begin());
    MirInstructionBuilder thenBuilder(getBuilderCtx(),
                                      thenBlock,
                                      InsertionType::InsertAfter,
                                      thenBlock->getInstructions().begin());
    MirInstructionBuilder elseBuilder(getBuilderCtx(),
                                      elseBlock,
                                      InsertionType::InsertAfter,
                                      elseBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    func->getStackFrame()->createStaticStackObj(t->i64());

    entryBuilder.JE(oBuilder.buildRef(thenBlock));
    entryBuilder.JMP(oBuilder.buildRef(elseBlock));

    thenBuilder.RET();
    elseBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    MirFrameLowererPass *pass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyPrologue(func);
    verifier.verifyEpilogue(func);
    verifier.verifyStackReferencesLowered(func);
}

TEST_F(TestFrameLowererPass, LowerDynamicStackAllocation)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder funcBuilder(getBuilderCtx());
    MirFunction *func = funcBuilder.build(t->getVoidType(), "LowerDynamicStackAllocation");

    MirBlock *entryBlock = func->getEntryPoint();
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   entryBlock,
                                   InsertionType::InsertAfter,
                                   entryBlock->getInstructions().begin());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *dynPtr = oBuilder.buildVReg(t->getPtr(t->i8()), "dynPtr");
    MirRegister *runtimeSize = oBuilder.buildVReg(t->i64(), "runtimeSize");

    iBuilder.MOV(runtimeSize, oBuilder.buildInt(t->i64(), FlexInt(128, 64)));
    iBuilder.DALLOC(dynPtr, runtimeSize);
    iBuilder.RET();

    runPass<MirInstructionSelectorPass>(getBuilderCtx(), getTargetDesc());
    runPass<MirRegisterAllocatorPass>(getBuilderCtx(), getTargetDesc());

    MirFrameLowererPass *framePass = runPass<MirFrameLowererPass>(getBuilderCtx(), getTargetDesc());

    FrameLowererPassVerifier verifier(getBuilderCtx(), framePass);
    verifier.verifyDAllocLowered(func).verifyPrologue(func).verifyEpilogue(func).verifyStackReferencesLowered(func);

    EXPECT_TRUE(func->getAnalysisData()->m_hasDynamicAllocs);
}