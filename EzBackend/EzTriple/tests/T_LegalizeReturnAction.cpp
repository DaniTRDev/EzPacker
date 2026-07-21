#include "EzTripleTestSuite.h"
#include "LegalizeReturnActionVerifier.h"

class TestLegalizeReturnAct : public MirTripleTestSuiteAsGtest
{
  public:
};

TEST_F(TestLegalizeReturnAct, TestEmptyRet)
{
    const auto &t = getTypeTable();
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->getVoidType());
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    builder.RET();

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    LegalizeReturnActionVerifier verifier(getBuilderCtx(), pass);

    // Void returns are standardized to hold a single tracking token operand
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
    MirOperand *immRet = opBuilder.buildInt(t->i8(), FlexInt(1, 8));

    builder.RET(immRet);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    LegalizeReturnActionVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyRetPush(func->getEntryPoint()->getInstructions().begin(), immRet);
}

// =========================================================================
// 3. DIRECT REGISTER RETURN TEST
// =========================================================================
TEST_F(TestLegalizeReturnAct, TestRetReg)
{
    const auto &t = getTypeTable();
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->i32());
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    MirOperandBuilder opBuilder(getBuilderCtx());
    MirOperand *regRet = opBuilder.buildVReg(t->i32(), "testRes");

    builder.RET(regRet);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    LegalizeReturnActionVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyRetPush(func->getEntryPoint()->getInstructions().begin(), regRet);
}

TEST_F(TestLegalizeReturnAct, TestRetSretStruct)
{
    /**
     * To prevent Expansion and Promotion from being executed, we add a rule to mark STORE ptr [i256], i256 as legal.
     */
    // Create a 256-bit wide return type.
    const auto &t = getTypeTable();
    MirType *largeStructType = t->i256();
    MirType *sretPtrType = t->getPtr(largeStructType);

    LegalizeRuleBuilder(getBuilderCtx(), getLegalizer())
            .begin(MirInstructionOpCode::STORE)
            .legalFor({ sretPtrType })
            .dump();

    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    // Build function expecting SRET (Implicit SRET pointer injected as parameter 0)
    functionBuilder.buildParam(sretPtrType, "sretPtr");
    MirFunction *func = functionBuilder.build(largeStructType);

    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    // Virtual register containing the large value (bit-chain, it DOES not assume structs).
    MirOperand *structVal = opBuilder.buildVReg(largeStructType, "largeStructVal");

    builder.RET(structVal);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    LegalizeReturnActionVerifier verifier(getBuilderCtx(), pass);

    // Verification sequence ensures: This DOESN'T INCLUDE EXPANSION.
    // 1. STORE ptr [sretPtr + 0], largeStructVal
    // 2. PUSH_RET retToken, sretPtr
    // 3. RET retToken
    verifier.verifyRetPush(func->getEntryPoint()->getInstructions().begin(), structVal);
}

TEST_F(TestLegalizeReturnAct, TestRetFloatReg)
{
    const auto &t = getTypeTable();
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->f64());
    MirInstructionBuilder builder(getBuilderCtx(),
                                  func->getEntryPoint(),
                                  InsertionType::InsertAfter,
                                  func->getEntryPoint()->getInstructions().begin());

    MirOperandBuilder opBuilder(getBuilderCtx());
    MirOperand *floatRet = opBuilder.buildVReg(t->f64(), "fpRes");

    builder.RET(floatRet);

    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    LegalizeReturnActionVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyRetPush(func->getEntryPoint()->getInstructions().begin(), floatRet);
}