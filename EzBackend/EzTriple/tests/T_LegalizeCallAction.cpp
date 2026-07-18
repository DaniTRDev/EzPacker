#include "EzTripleTestSuite.h"
#include <gtest/gtest.h>

class TestLegalizeCallAct : public MirTripleTestSuiteAsGtest
{
  public:
    std::shared_ptr<MirLegalizer> createTargetLegalizer() override
    {
        auto legalizer = MirTripleTestSuiteAsGtest::createTargetLegalizer();
        LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;
        std::vector<MirType *> sizes = { getTypeTable()->i8(),
                                         getTypeTable()->i16(),
                                         getTypeTable()->i32(),
                                         getTypeTable()->i64() };

        // Ensure the legalizer loop considers PUSH_ARG and POP_RET instructions legal.
        // Both now use a tracking token (BindingToken) as their first operand.
        size_t tokenTypeId = getTypeTable()->getBindingToken()->getId();
        for (const auto &dest : sizes)
        {
            size_t destId = dest->getId();
            legalizer->addRule(legal, MirInstructionOpCode::PUSH_ARG, { tokenTypeId, destId });
            legalizer->addRule(legal, MirInstructionOpCode::POP_RET, { tokenTypeId, destId });
        }

        return legalizer;
    }
};

TEST_F(TestLegalizeCallAct, TestNoArgsVoid)
{
    const auto &t = getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    MirFunction *func = functionBuilder.build(t->i8()); // Callee function
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    // Build VOID Call (Operand 0 points directly to callee because there is no return reg)
    auto callRef = opBuilder.buildRef(func);
    builder.CALL(callRef);

    // Capture original state layout for verification tracking: [Callee]
    std::vector<MirOperand *> expectedOrigOperands{ callRef };

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // Assert: No PUSH_ARGs, CALL truncated to [Token, Callee], No POP_RET
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), expectedOrigOperands, nullptr);
}

TEST_F(TestLegalizeCallAct, Test1ArgVoid)
{
    const auto &t = getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    // Setup a single parameter on the callee function
    functionBuilder.buildParam(t->i8(), "testArg1");
    MirFunction *func = functionBuilder.build(t->i8());

    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    // Prepare the layout: [Callee, Arg0]
    auto callRef = opBuilder.buildRef(func);
    std::vector<MirOperand *> expectedOrigOperands{ callRef, opBuilder.buildInt(t->i8(), FlexInt(42, 8)) };

    // Build a VOID CALL. The first operand is the Callee Target Reference.
    auto callInstr = builder.CALL(expectedOrigOperands[0]);
    for (size_t i = 1; i < expectedOrigOperands.size(); ++i)
    {
        callInstr->getOperands().push_back(expectedOrigOperands[i]);
    }

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // We expect exactly one token-bound PUSH_ARG for our argument, followed by the CALL instruction truncated.
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), expectedOrigOperands, nullptr);
}

TEST_F(TestLegalizeCallAct, Test1ArgWithReturn)
{
    const auto &t = getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    // Setup parameter on callee
    functionBuilder.buildParam(t->i8(), "testArg1");
    MirFunction *func = functionBuilder.build(t->i8());

    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    // Original return destination register
    MirRegister *destReg = opBuilder.buildVReg(t->i8(), "returnDest");

    // Prepare the complete baseline call array layout: [DestReg, Callee, Arg0]
    std::vector<MirOperand *> expectedOrigOperands{ destReg,
                                                    opBuilder.buildRef(func),
                                                    opBuilder.buildInt(t->i8(), FlexInt(1, 8)) };

    // Build: CALL %destReg, %func, %arg
    auto callInstr = builder.CALL(expectedOrigOperands[0]);
    for (size_t i = 1; i < expectedOrigOperands.size(); ++i)
    {
        callInstr->getOperands().push_back(expectedOrigOperands[i]);
    }

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // Assert: PUSH_ARG %token, %arg, CALL %token, callee, followed by POP_RET %token, %destReg
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), expectedOrigOperands, destReg);
}

TEST_F(TestLegalizeCallAct, Test5ArgWithReturn)
{
    const auto &t = getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    functionBuilder.buildParam(t->i8(), "testArg1");
    functionBuilder.buildParam(t->i16(), "testArg2");
    functionBuilder.buildParam(t->i32(), "testArg3");
    functionBuilder.buildParam(t->i64(), "testArg4");
    functionBuilder.buildParam(t->i8(), "testArg5");

    MirFunction *func = functionBuilder.build(t->i8());
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    MirRegister *destReg = opBuilder.buildVReg(t->i16(), "returnDest");

    // Prepare complete vector layout: [DestReg, Callee, Arg0, Arg1, Arg2, Arg3, Arg4]
    std::vector<MirOperand *> expectedOrigOperands{ destReg,
                                                    opBuilder.buildRef(func),
                                                    opBuilder.buildInt(t->i8(), FlexInt(1, 8)),
                                                    opBuilder.buildInt(t->i16(), FlexInt(2, 16)),
                                                    opBuilder.buildInt(t->i32(), FlexInt(3, 32)),
                                                    opBuilder.buildVReg(t->i64()),
                                                    opBuilder.buildVReg(t->i8()) };

    auto callInstr = builder.CALL(expectedOrigOperands[0]);
    for (size_t i = 1; i < expectedOrigOperands.size(); ++i)
    {
        callInstr->getOperands().push_back(expectedOrigOperands[i]);
    }

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // Assert the exact token-bound sequence across all 5 parameter push nodes and pop return.
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), expectedOrigOperands, destReg);
}