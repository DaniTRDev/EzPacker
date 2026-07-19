#include "EzTripleTestSuite.h"

class TestLegalizeReturnAct : public MirTripleTestSuiteAsGtest
{
  public:
};

TEST_F(TestLegalizeReturnAct, TestEmptyRet)
{
    const auto &t = getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->getVoidType());
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    builder.RET();

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeReturnActionVerifier verifier(getBuilderCtx(), pass);

    // Void returns require no modifications and remain empty.
    verifier.verifyRetPush(func->getEntryPoint()->getInstructions().begin(), nullptr);
}

TEST_F(TestLegalizeReturnAct, TestRetImm)
{
    const auto &t = getTypeTable();
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->i8());
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    MirOperandBuilder opBuilder(getBuilderCtx());
    std::vector<MirOperand *> operands{ opBuilder.buildInt(t->i8(), FlexInt(1, 8)) };

    builder.RET(operands[0]);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    LegalizeReturnActionVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyRetPush(func->getEntryPoint()->getInstructions().begin(), operands[0]);
}

TEST_F(TestLegalizeReturnAct, TestRetReg)
{
    const auto &t = getTypeTable();
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->i8());
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    MirOperandBuilder opBuilder(getBuilderCtx());
    std::vector<MirOperand *> operands{ opBuilder.buildVReg(t->i8(), "testRes") };

    builder.RET(operands[0]);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    LegalizeReturnActionVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyRetPush(func->getEntryPoint()->getInstructions().begin(), operands[0]);
}