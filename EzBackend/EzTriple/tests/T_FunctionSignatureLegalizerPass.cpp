#include "EzTripleTestSuite.h"

class TestLegalizeCallAct : public MirTripleTestSuiteAsGtest
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

        for (const auto &dest : sizes)
        {
            legalizer->addRule(legal, MirInstructionOpCode::POP_ARG, { dest, MIRID_INVALID });
        }

        return legalizer;
    }
};

TEST_F(TestLegalizeCallAct, TestCallNoArgs)
{
    const auto &t = getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->i8());
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    MirFunctionSignatureLegalizerPass *pass =
            runPass<MirFunctionSignatureLegalizerPass>(getBuilderCtx(), getLegalizer());

    FuncSignaturePassVerifier verifier(getBuilderCtx(), pass);
    verifier.beginFunc(func);
    verifier.verifyArgs({});
}

TEST_F(TestLegalizeCallAct, TestCall1Arg)
{
    /**
     * i8 %testFunc(i8 %param1)
     * {
     * }
     *
     * Should be transformed into:
     * i8 %testFunc()
     * {
     *      POP_ARG i8 %param1;
     * }
     */
    const auto &t = getTypeTable();

    MirOperandBuilder opBuilder(getBuilderCtx());
    std::pmr::list<MirRegister *> paramList = { opBuilder.buildVReg(t->i8()) };

    MirFunctionBuilder functionBuilder(getBuilderCtx());
    for (auto param : paramList)
    {
        functionBuilder.buildParam(param);
    }

    MirFunction *func = functionBuilder.build(t->i8(), "testFunc");

    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    MirFunctionSignatureLegalizerPass *pass =
            runPass<MirFunctionSignatureLegalizerPass>(getBuilderCtx(), getLegalizer());

    FuncSignaturePassVerifier verifier(getBuilderCtx(), pass);
    verifier.beginFunc(func);
    verifier.verifyArgs(std::vector<MirRegister *>{ std::begin(paramList), std::end(paramList) });
}

TEST_F(TestLegalizeCallAct, TestCall4Arg)
{
    /**
     * i8 %testFunc(i8 %param1, i16 %param2, i32 %param3, i64 %param4)
     * {
     * }
     *
     * Should be transformed into:
     * i8 %testFunc()
     * {
     *      POP_ARG i8 %param1;
     *      POP_ARG i16 %param2;
     *      POP_ARG i32 %param3;
     *      POP_ARG i64 %param4;
     * }
     */
    const auto &t = getTypeTable();

    MirOperandBuilder opBuilder(getBuilderCtx());
    std::pmr::list<MirRegister *> paramList = { opBuilder.buildVReg(t->i8()),
                                                opBuilder.buildVReg(t->i16()),
                                                opBuilder.buildVReg(t->i32()),
                                                opBuilder.buildVReg(t->i64()) };

    MirFunctionBuilder functionBuilder(getBuilderCtx());
    for (auto param : paramList)
    {
        functionBuilder.buildParam(param);
    }

    MirFunction *func = functionBuilder.build(t->i8(), "testFunc");

    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    MirFunctionSignatureLegalizerPass *pass =
            runPass<MirFunctionSignatureLegalizerPass>(getBuilderCtx(), getLegalizer());

    FuncSignaturePassVerifier verifier(getBuilderCtx(), pass);
    verifier.beginFunc(func);
    verifier.verifyArgs(std::vector<MirRegister *>{ std::begin(paramList), std::end(paramList) });
}