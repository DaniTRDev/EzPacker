#include "EzTripleTestSuite.h"

class TestPromoteScalarAct : public MirTripleTestSuiteAsGtest
{
  public:
};

TEST_F(TestPromoteScalarAct, PromoteSingleBitRegSrc)
{
    /**
     * Input:
     *  add i64 %dest, i1 %src
     *
     * Should be expanded into:
     *  zext i8 %src_promoted, i1 %src
     *  add i64 %dest, i8 %src_promoted
     */
    const auto *t = getTypeTable();
    addTestInstructionRegReg(MirInstructionOpCode::ADD, t->i32(), t->i1());

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirLegalizerPass *pass = runPass<MirLegalizerPass>(getBuilderCtx(), getLegalizer());
    PromoteScalarActionVerifier promoteVerifier(getBuilderCtx(), pass);
    MirInstructionVerifier instrVerifier(block->at(1));

    // Verify the ZEXT insertion.
    promoteVerifier.beginBlock(block);
    promoteVerifier.verifyExtension(0, MirInstructionOpCode::ZEXT, t->i1(), t->i8());

    instrVerifier.operandVerifier(0).verifyRegister(t->i32(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyRegister(t->i8(), true, MIRID_INVALID);
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitImmSrc)
{
    /**
     * Input:
     *  add i64 %dest, i1 1
     *
     * Should be expanded into:
     *  add i64 %dest, i8 1
     */
    const auto *t = getTypeTable();
    addTestInstructionRegIntImm(MirInstructionOpCode::ADD, t->i32(), t->i1(), 1);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirInstructionVerifier instrVerifier(block->at(0));

    runPass<MirLegalizerPass>(getBuilderCtx(), getLegalizer());

    instrVerifier.operandVerifier(0).verifyRegister(t->i32(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyInteger(t->i8(), 1);
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitRegSrcDest)
{
    /**
     * Input:
     *  add i1 %dest, i1 %src
     *
     * Should be expanded into:
     *  zext i8 %dest_promoted, i1 %dest
     *  zext i8 %src_promoted, i1 %src
     *  add i8 %dest_promoted, i8 %src_promoted
     *  trunc i8 %dest_promoted, i8 1
     */
    const auto *t = getTypeTable();
    addTestInstructionRegReg(MirInstructionOpCode::ADD, t->i1(), t->i1());

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirLegalizerPass *pass = runPass<MirLegalizerPass>(getBuilderCtx(), getLegalizer());
    PromoteScalarActionVerifier promoteVerifier(getBuilderCtx(), pass);
    MirInstructionVerifier instrVerifier(block->at(2));

    // Verify the ZEXT insertion.
    promoteVerifier.beginBlock(block);
    promoteVerifier.verifyExtension(0, MirInstructionOpCode::ZEXT, t->i1(), t->i8());
    promoteVerifier.verifyExtension(1, MirInstructionOpCode::ZEXT, t->i1(), t->i8());
    promoteVerifier.verifyExtensionTruncation(3, t->i8(), t->i1());

    instrVerifier.operandVerifier(0).verifyRegister(t->i8(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyRegister(t->i8(), true, MIRID_INVALID);
}

TEST_F(TestPromoteScalarAct, PromoteSingleBitRegDestImmSrc)
{
    /**
     * Input:
     *  add i1 %dest, i1 1
     *
     * Should be expanded into:
     *  zext i8 %dest_promoted, i1 %dest
     *  add i8 %dest_promoted, i8 1
     *  trunc i8 %dest_promoted, i8 1
     */
    const auto *t = getTypeTable();
    addTestInstructionRegIntImm(MirInstructionOpCode::ADD, t->i1(), t->i1(), 1);
    
    MirBlock *block = getTestFunc()->getEntryPoint();
    MirLegalizerPass *pass = runPass<MirLegalizerPass>(getBuilderCtx(), getLegalizer());
    PromoteScalarActionVerifier promoteVerifier(getBuilderCtx(), pass);
    MirInstructionVerifier instrVerifier(block->at(1));

    // Verify the ZEXT insertion.
    promoteVerifier.beginBlock(block);
    promoteVerifier.verifyExtension(0, MirInstructionOpCode::ZEXT, t->i1(), t->i8());
    promoteVerifier.verifyExtensionTruncation(2, t->i8(), t->i1());

    instrVerifier.operandVerifier(0).verifyRegister(t->i8(), true, MIRID_INVALID);
    instrVerifier.operandVerifier(1).verifyInteger(t->i8(), 1);
}