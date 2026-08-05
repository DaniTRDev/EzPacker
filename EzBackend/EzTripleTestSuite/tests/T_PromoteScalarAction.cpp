#include "EzTripleTestSuite.h"

class TestPromoteScalarAct : public MirTripleTestSuiteAsGtest
{
  public:
};

TEST_F(TestPromoteScalarAct, PromoteSingleBitRegSrc)
{
    /**
     * Input:
     *  ADD i32 %dest, i1 %src
     *
     * Should be transformed into:
     *  ZEXT i8 %src_promoted, i1 %src
     *  ADD  i32 %dest, i8 %src_promoted
     */
    const auto *t = getTypeTable();
    addTestInstructionRegReg(MirInstructionOpCode::ADD, t->i32(), t->i1());

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
    PromoteScalarActionVerifier promoteVerifier(getBuilderCtx(), pass);

    // The main ADD instruction is pushed down to index 1 by the injected ZEXT
    MirInstructionVerifier instrVerifier(block->at(1));

    promoteVerifier.beginBlock(block);
    // Verifies that index 0 contains the extension: Dest is wide (i8), Src is narrow (i1)
    promoteVerifier.verifyExtension(0, MirInstructionOpCode::ZEXT, t->i1(), t->i8());

    // Verify current instruction arguments
    instrVerifier.operandVerifier(0).verifyRegister(t->i32(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyRegister(t->i8(), true, MIRID_INVALID); // Now reads wide promoted register
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitImmSrc)
{
    /**
     * Input:
     *  ADD i32 %dest, i1 1
     *
     * Should be transformed into (No extension instructions needed for literals):
     *  ADD i32 %dest, i8 1
     */
    const auto *t = getTypeTable();
    addTestInstructionRegIntImm(MirInstructionOpCode::ADD, t->i32(), t->i1(), FlexInt(uint32_t(1), 1));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirInstructionVerifier instrVerifier(block->at(0));

    runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());

    instrVerifier.operandVerifier(0).verifyRegister(t->i32(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyInteger(t->i8(),
                                                   FlexInt(uint32_t(1), 1)); // Literal matches promoted target width
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitRegSrcDest)
{
    /**
     * Input:
     *  ADD i1 %dest, i1 %src
     *
     * Should be transformed into:
     *  ZEXT i8 %dest_promoted, i1 %dest
     *  ZEXT i8 %src_promoted, i1 %src
     *  ADD  i8 %dest_promoted, i8 %src_promoted
     */
    const auto *t = getTypeTable();
    addTestInstructionRegReg(MirInstructionOpCode::ADD, t->i1(), t->i1());

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
    PromoteScalarActionVerifier promoteVerifier(getBuilderCtx(), pass);

    // Main ADD instruction sits at index 2 after both injected ZEXT calculations
    MirInstructionVerifier instrVerifier(block->at(2));

    promoteVerifier.beginBlock(block);
    promoteVerifier.verifyExtension(0, MirInstructionOpCode::ZEXT, t->i1(), t->i8());
    promoteVerifier.verifyExtension(1, MirInstructionOpCode::ZEXT, t->i1(), t->i8());

    // Both instruction arguments are now cleanly substituted with wide promoted types
    instrVerifier.operandVerifier(0).verifyRegister(t->i8(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyRegister(t->i8(), true, MIRID_INVALID);
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitRegDestImmSrc)
{
    /**
     * Input:
     *  ADD i1 %dest, i2 1
     *
     * Should be transformed into:
     *  ZEXT i8 %dest_promoted, i1 %dest
     *  ADD  i8 %dest_promoted, i8 1
     */
    const auto *t = getTypeTable();
    addTestInstructionRegIntImm(MirInstructionOpCode::ADD, t->i1(), t->i1(), FlexInt(uint32_t(1), 1));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
    PromoteScalarActionVerifier promoteVerifier(getBuilderCtx(), pass);

    // The main ADD instruction sits exactly at index 1
    MirInstructionVerifier instrVerifier(block->at(1));

    promoteVerifier.beginBlock(block);
    promoteVerifier.verifyExtension(0, MirInstructionOpCode::ZEXT, t->i1(), t->i8());

    // Validate that the math operation works fully in the wide promoted space
    instrVerifier.operandVerifier(0).verifyRegister(t->i8(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyInteger(t->i8(), FlexInt(uint32_t(1), 1));
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitRegSrcAndSecondUse)
{
    /**
     * Input:
     *  ADD i32 %dest, i1 %src
     *  SUB i32 %dest_2, i1 %src
     *
     * Should be transformed into:
     *  ZEXT i8 %src_promoted, i1 %src
     *  ADD  i32 %dest, i8 %src_promoted
     *  SUB  i64 %dest_2, i8 %src_promoted
     */
    const auto *t = getTypeTable();
    auto instr = addTestInstructionRegReg(MirInstructionOpCode::ADD, t->i32(), t->i1());
    auto instr2 = addTestInstructionRegReg(MirInstructionOpCode::SUB, t->i64(), t->i1());

    instr2->getOperands()[1] = instr->getOperands()[1]; // Ensure src operands are the same.

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
    PromoteScalarActionVerifier promoteVerifier(getBuilderCtx(), pass);

    promoteVerifier.beginBlock(block);
    // Verifies that index 0 contains the extension: Dest is wide (i8), Src is narrow (i1)
    promoteVerifier.verifyExtension(0, MirInstructionOpCode::ZEXT, t->i1(), t->i8());

    // The ADD instruction is pushed down to index 1 by the injected ZEXT
    MirInstructionVerifier instr1Verifier(block->at(1));
    instr1Verifier.opcode(MirInstructionOpCode::ADD);
    instr1Verifier.operandVerifier(0).verifyRegister(t->i32(), true, MIRID_INVALID);
    instr1Verifier.operandVerifier(1).verifyRegister(t->i8(), true, MIRID_INVALID); // Now reads wide promoted register

    // The SUB instruction is pushed down to index 2 by the injected ZEXT
    MirInstructionVerifier instr2Verifier(block->at(2));
    instr2Verifier.opcode(MirInstructionOpCode::SUB);
    instr2Verifier.operandVerifier(0).verifyRegister(t->i64(), true, MIRID_INVALID);
    instr2Verifier.operandVerifier(1).verifyRegister(t->i8(), true, MIRID_INVALID); // Now reads wide promoted register
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitCallImm)
{
    /**
     * Input:
     *  CALL void* %testFunc, i1 1
     *
     * Should be transformed into (No extension instructions needed for literals):
     *  PUSH_ARG __bindingToken %bindingToken, i8 1
     *  CALL __bindingToken %bindingToken, void* %testFunc
     */
    const auto *t = getTypeTable();
    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());

    builder.CALL(opBuilder.buildRef(getTestFunc()), opBuilder.buildInt(t->i1(), FlexInt(0, 1)));

    MirBlock *block = getTestFunc()->getEntryPoint();

    runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());

    MirInstructionVerifier instrVerifier(block->at(0));
    instrVerifier.operandVerifier(0).verifyRegister(t->getBindingToken(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyInteger(t->i8(), FlexInt(0, 1)); // Literal matches promoted target width
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitCallReg)
{
    /**
     * Input:
     *  CALL void* %testFunc, i1 %reg
     *
     * Should be transformed into:
     *  ZEXT %reg_promoted, %reg
     *  PUSH_ARG __bindingToken %bindingToken, i8 %reg_promoted
     *  CALL __bindingToken %bindingToken, void* %testFunc
     */
    const auto *t = getTypeTable();
    MirBlock *block = getTestFunc()->getEntryPoint();
    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());

    builder.CALL(opBuilder.buildRef(getTestFunc()), opBuilder.buildVReg(t->i1(), "testParam"));

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
    PromoteScalarActionVerifier promoteVerifier(getBuilderCtx(), pass);
    MirInstructionVerifier instrVerifier(block->at(1));

    promoteVerifier.beginBlock(block);
    promoteVerifier.beginBlock(getTestFunc()->getEntryPoint());
    promoteVerifier.verifyExtension(0, MirInstructionOpCode::ZEXT, t->i1(), t->i8());
    instrVerifier.operandVerifier(0).verifyRegister(t->getBindingToken(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyRegister(t->i8(), true, MIRID_INVALID);
}
