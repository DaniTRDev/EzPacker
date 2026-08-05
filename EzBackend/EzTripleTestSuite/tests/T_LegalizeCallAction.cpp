#include "EzTripleTestSuite.h"

class TestLegalizeCallAct : public MirTripleTestSuiteAsGtest
{
  public:
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

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
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
    auto callInstr = builder.CALL(expectedOrigOperands[0], expectedOrigOperands[1]);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
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
    auto callInstr = builder.CALL(expectedOrigOperands[0], expectedOrigOperands[1], expectedOrigOperands[2]);
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
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

    auto callInstr = builder.CALL(expectedOrigOperands[0],
                                  expectedOrigOperands[1],
                                  expectedOrigOperands[2],
                                  expectedOrigOperands[3],
                                  expectedOrigOperands[4],
                                  expectedOrigOperands[5],
                                  expectedOrigOperands[6]);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // Assert the exact token-bound sequence across all 5 parameter push nodes and pop return.
    verifier.verifyCallSequence(func->getEntryPoint()->getInstructions().begin(), expectedOrigOperands, destReg);
}

TEST_F(TestLegalizeCallAct, TestSretCallLegalization)
{
    const auto &t = getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    // Force a type that triggers the SRET path (wider than 64 bits)
    MirType *wideStructType = t->i128();

    functionBuilder.buildParam(t->i32(), "userArg0");
    MirFunction *func = functionBuilder.build(wideStructType);

    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    // Virtual destination register where the high-level code expects the output struct payload
    MirRegister *destReg = opBuilder.buildVReg(wideStructType, "sret_dest_var");

    // The original standard high-level layout array: [destReg, calleeRef, userArg0]
    std::vector<MirOperand *> expectedOrigOperands{ destReg,
                                                    opBuilder.buildRef(func),
                                                    opBuilder.buildInt(t->i32(), FlexInt(77, 32)) };

    // Build standard high-level CALL: CALL %destReg, %func, %arg
    auto callInstr = builder.CALL(expectedOrigOperands[0], expectedOrigOperands[1], expectedOrigOperands[2]);

    // Execute the legalizer pass over the block stream
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer(), getTargetDesc());
    LegalizeCallActionVerifier verifier(getBuilderCtx(), pass);

    // Assert: ALLOC, PUSH_ARG sret_ptr, PUSH_ARG user_arg, CALL call_token, callee (No POP_RET)
    verifier.verifySretCallSequence(func->getEntryPoint()->getInstructions().begin(), expectedOrigOperands);
}