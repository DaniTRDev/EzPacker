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

        // Ensure the legalizer loop considers PUSH_ARG and POP_RET instructions legal
        // to prevent false alarms during verification loops
        for (const auto &dest : sizes)
        {
            size_t destId = dest->getId();
            legalizer->addRule(legal, MirInstructionOpCode::PUSH_ARG, { destId, MIRID_INVALID });
            legalizer->addRule(legal, MirInstructionOpCode::POP_RET, { MIRID_INVALID, destId });
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
    auto callRef = opBuilder.buildRef(func->getEntryPoint());
    builder.CALL(callRef);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // Assert: No PUSH_ARGs, CALL truncated, No POP_RET
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), {}, nullptr);
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

    // Prepare the single argument value
    std::vector<MirOperand *> operands{ opBuilder.buildInt(t->i8(), FlexInt(42, 8)) };

    // Build a VOID CALL. The first operand is the Callee Target Reference.
    // There is no return destination register.
    auto callInstr = builder.CALL(opBuilder.buildRef(func));
    for (auto op : operands)
    {
        callInstr->getOperands().push_back(op);
    }

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // We expect exactly one PUSH_ARG for our argument, followed by the CALL instruction truncated to [Callee], and NO
    // POP_RET instruction.
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), operands, nullptr);
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
    std::vector<MirOperand *> operands{ opBuilder.buildInt(t->i8(), FlexInt(1, 8)) };

    // Build:  CALL %destReg, %func, %arg
    auto callInstr = builder.CALL(destReg);
    callInstr->getOperands().push_back(opBuilder.buildRef(func)); // Callee
    for (auto op : operands)
    {
        callInstr->getOperands().push_back(op);
    }

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // Assert: PUSH_ARG %arg, CALL %token, callee, followed by POP_RET %token, %destReg
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), operands, destReg);
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
    std::vector<MirOperand *> operands{ opBuilder.buildInt(t->i8(), FlexInt(1, 8)),
                                        opBuilder.buildInt(t->i16(), FlexInt(2, 16)),
                                        opBuilder.buildInt(t->i32(), FlexInt(3, 32)),
                                        opBuilder.buildVReg(t->i64()),
                                        opBuilder.buildVReg(t->i8()) };

    auto callInstr = builder.CALL(destReg);
    callInstr->getOperands().push_back(opBuilder.buildRef(func));
    for (auto op : operands)
    {
        callInstr->getOperands().push_back(op);
    }

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // Assert the exact sequence including all 5 PUSH_ARGS, token assignment, and final POP_RET
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), operands, destReg);
}