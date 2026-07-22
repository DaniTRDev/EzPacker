#include "EzTripleTestSuite.h"

class TestFunctionArgAbiLowererPass : public MirTripleTestSuiteAsGtest
{
  public:
};

// =========================================================================
// 1. ZERO PARAMETERS TEST
// =========================================================================
TEST_F(TestFunctionArgAbiLowererPass, LowerZeroFunctionArguments)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirFunctionBuilder functionBuilder(getBuilderCtx());
    MirFunction *func = functionBuilder.build(t->i8(), "testFuncNoArgs");

    // Pre-lowering state: Legalized function entry has zero POP_ARG instructions
    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: Entry block remains clean, zero parameter prep instructions generated
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), {});
}

// =========================================================================
// 2. RULE 1B: 32-BIT DIRECT REGISTER PARAMETER (4 Bytes -> Single GPR)
// =========================================================================
TEST_F(TestFunctionArgAbiLowererPass, LowerDirect32BitGprFunctionArgument)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    MirRegister *param32 = opBuilder.buildVReg(t->i32(), "param32");

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFunc32Bit");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    // Post-LegalizeSignature state: POP_ARG token, param32
    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());
    iBuilder.POP_ARG(token, param32);
    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: POP_ARG erased, MOV param32, physReg(GPR 1) generated at top of entry block
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), { param32 });
}

// =========================================================================
// 3. RULE 1A: 64-BIT SPLIT PARAMETER (8 Bytes -> Two 32-bit GPRs)
// =========================================================================
TEST_F(TestFunctionArgAbiLowererPass, LowerSplit64BitGprFunctionArgument)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    MirRegister *param64 = opBuilder.buildVReg(t->i64(), "param64");

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFunc64BitSplit");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());
    iBuilder.POP_ARG(token, param64);
    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify Rule 1A: STORE [param64 + 0], physReg(1) -> STORE [param64 + 4], physReg(2)
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), { param64 });
}

// =========================================================================
// 4. RULE 1C: 256-BIT INDIRECT / BYVAL PARAMETER (32 Bytes -> Pointer GPR)
// =========================================================================
TEST_F(TestFunctionArgAbiLowererPass, LowerIndirect256BitStructFunctionArgument)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    MirType *largeType = t->i256(); // 32 bytes -> Rule 1C Indirect pass
    MirRegister *param256 = opBuilder.buildVReg(largeType, "param256");

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFunc256BitIndirect");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());
    iBuilder.POP_ARG(token, param256);
    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify Rule 1C: MOV param256, physReg(GPR 1)
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), { param256 });
}

// =========================================================================
// 5. RULE 2: PARITY INTERLEAVING PARAMETERS (Even Allocations -> Stack)
// =========================================================================
TEST_F(TestFunctionArgAbiLowererPass, LowerParityInterleavingFunctionArguments)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    // Param 0 (16-bit): AllocCount = 0 (EVEN) -> Forced to incoming Stack Slot
    // Param 1 (16-bit): AllocCount = 0 (EVEN) -> Forced to incoming Stack Slot
    // Param 2 (32-bit): Size 4 triggers Rule 1B -> GPR 1 (AllocCount becomes 1, ODD)
    // Param 3 (16-bit): AllocCount = 1 (ODD) -> Rule 3 GPR Fallback -> GPR 2
    MirRegister *p0 = opBuilder.buildVReg(t->i16(), "p0_stack");
    MirRegister *p1 = opBuilder.buildVReg(t->i16(), "p1_stack");
    MirRegister *p2 = opBuilder.buildVReg(t->i32(), "p2_gpr1");
    MirRegister *p3 = opBuilder.buildVReg(t->i16(), "p3_gpr2");

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFuncParity");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());
    iBuilder.POP_ARG(token, p0);
    iBuilder.POP_ARG(token, p1);
    iBuilder.POP_ARG(token, p2);
    iBuilder.POP_ARG(token, p3);
    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: LOAD from stack slots for p0/p1, MOV from physical GPRs for p2/p3
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), { p0, p1, p2, p3 });
}

// =========================================================================
// 6. RULE 4: REGISTER EXHAUSTION SPILL PARAMETERS
// =========================================================================
TEST_F(TestFunctionArgAbiLowererPass, LowerRegisterExhaustionSpillFunctionArguments)
{
    const auto &t = getBuilderCtx()->getTypeTable();
    MirOperandBuilder opBuilder(getBuilderCtx());
    MirFunctionBuilder functionBuilder(getBuilderCtx());

    // GPR volatile pool has 2 registers {1, 2}.
    // Param 0 (32-bit): GPR 1 (AllocCount = 1, ODD)
    // Param 1 (16-bit): Rule 3 -> GPR 2 (AllocCount = 2, EVEN)
    // Param 2 (16-bit): AllocCount = 2 (EVEN) -> Rule 2 forces Stack Slot
    // Param 3 (32-bit): GPRs exhausted -> Rule 4 forces Stack Slot
    MirRegister *p0 = opBuilder.buildVReg(t->i32(), "p0_gpr1");
    MirRegister *p1 = opBuilder.buildVReg(t->i16(), "p1_gpr2");
    MirRegister *p2 = opBuilder.buildVReg(t->i16(), "p2_parity_stack");
    MirRegister *p3 = opBuilder.buildVReg(t->i32(), "p3_exhausted_stack");

    MirFunction *func = functionBuilder.build(t->getVoidType(), "testFuncExhaustion");
    MirInstructionBuilder iBuilder(getBuilderCtx(),
                                   func->getEntryPoint(),
                                   InsertionType::InsertAfter,
                                   func->getEntryPoint()->getInstructions().begin());

    MirRegister *token = opBuilder.buildVReg(t->getBindingToken());
    iBuilder.POP_ARG(token, p0);
    iBuilder.POP_ARG(token, p1);
    iBuilder.POP_ARG(token, p2);
    iBuilder.POP_ARG(token, p3);
    iBuilder.END_ARG(token);

    FunctionAbiLowererPass *pass = runPass<FunctionAbiLowererPass>(getBuilderCtx());

    // Verify: MOVs from physical registers for p0/p1, LOADs from incoming stack slots for p2/p3
    FunctionArgAbiLowererVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyLoweredFunctionArguments(func->getEntryPoint(), { p0, p1, p2, p3 });
}