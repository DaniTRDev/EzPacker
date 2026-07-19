#include "EzTripleTestSuite.h"

class TestLegalizeReturnAct : public MirTripleTestSuiteAsGtest
{
  public:
    /**
     * Creates a simple legalizer.
     * @return
     */
    std::shared_ptr<MirLegalizer> createTargetLegalizer() override
    {
        auto legalizer = MirTripleTestSuiteAsGtest::createTargetLegalizer();
        LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;
        std::vector<size_t> sizes = { getTypeTable()->i8()->getId(),
                                      getTypeTable()->i16()->getId(),
                                      getTypeTable()->i32()->getId(),
                                      getTypeTable()->i64()->getId(),
                                      MIRLEGALIZE_POINTER_TYPE };

        size_t tokenTypeId = getTypeTable()->getBindingToken()->getId();

        // Extension instructions must act as a bridge between illegal and legal types, we need to legal them on every
        // SRC case.
        for (const auto &destId : sizes)
        {
            legalizer->addRule(legal, MirInstructionOpCode::ZEXT, { destId, MIRID_INVALID });
            legalizer->addRule(legal, MirInstructionOpCode::SEXT, { destId, MIRID_INVALID });
            legalizer->addRule(legal, MirInstructionOpCode::TRUNC, { destId, MIRID_INVALID });
            legalizer->addRule(legal, MirInstructionOpCode::BITCAST, { destId, MIRID_INVALID });
            legalizer->addRule(legal, MirInstructionOpCode::PUSH_ARG, { tokenTypeId, destId });
            legalizer->addRule(legal, MirInstructionOpCode::POP_RET, { tokenTypeId, destId });
            legalizer->addRule(legal, MirInstructionOpCode::PUSH_RET, { tokenTypeId, destId });
        }

        return legalizer;
    }
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