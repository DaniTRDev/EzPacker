#include "EzTripleTestSuite.h"
using namespace EzTripleTestInstructionSet;

class TestMirInstructionSelectorPass : public MirTripleTestSuiteAsGtest
{
  public:
};

// =========================================================================
// 1. DATA MOVEMENT & MEMORY ACCESS SELECTION
// =========================================================================
TEST_F(TestMirInstructionSelectorPass, SelectDataMovementAndMemory)
{
    /**
     * Input:
     *  MOV   i8  %v8, 42
     *  MOV   i32 %v32, 100
     *  LOAD  i64 %v64, [%base + 0]
     *  STORE [%base + 0], f64 %vf64
     */
    const auto *t = getTypeTable();
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *vreg8 = oBuilder.buildVReg(t->i8(), "v8");
    MirRegister *vreg32 = oBuilder.buildVReg(t->i32(), "v32");
    MirRegister *vreg64 = oBuilder.buildVReg(t->i64(), "v64");
    MirRegister *vregF64 = oBuilder.buildVReg(t->f64(), "vf64");

    MirOperand *memOp = oBuilder.buildMem(t->i64(), vreg64, oBuilder.buildInt(t->i32(), FlexInt(0, 32)));

    // 0: MOV i8
    iBuilder.MOV(vreg8, oBuilder.buildInt(t->i8(), FlexInt(42, 8)));
    // 1: MOV i32
    iBuilder.MOV(vreg32, oBuilder.buildInt(t->i32(), FlexInt(100, 32)));
    // 2: LOAD i64
    iBuilder.LOAD(vreg64, memOp);
    // 3: STORE f64
    iBuilder.STORE(memOp, vregF64);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirInstructionSelectorPass *pass = runPass<MirInstructionSelectorPass>(getBuilderCtx(), getInstrSelector());

    InstructionSelectorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyInstructionSelected(block, 0, TargetInst::MOV8rr)
            .verifyInstructionSelected(block, 1, TargetInst::MOV32rr)
            .verifyInstructionSelected(block, 2, TargetInst::MOV64rm)
            .verifyInstructionSelected(block, 3, TargetInst::MOVSDmr);

    // Verify selected instruction operands
    MirInstructionVerifier(block->at(0)).operandVerifier(0).verifyRegister(t->i8(), true, MIRID_INVALID);
    MirInstructionVerifier(block->at(1)).operandVerifier(0).verifyRegister(t->i32(), true, MIRID_INVALID);
    MirInstructionVerifier(block->at(2)).operandVerifier(0).verifyRegister(t->i64(), true, MIRID_INVALID);
}

// =========================================================================
// 2. ARITHMETIC & LOGIC (ALU) SELECTION
// =========================================================================
TEST_F(TestMirInstructionSelectorPass, SelectAluOperations)
{
    /**
     * Input:
     *  ADD  i32 %dest, %src
     *  SUB  i64 %dest, %src
     *  AND  i32 %dest, %src
     *  XOR  i64 %dest, %src
     *  CMP  i32 %lhs,  %rhs
     *  IDIV i64 %dest, %src
     */
    const auto *t = getTypeTable();
    addTestInstructionRegReg(MirInstructionOpCode::ADD, t->i32(), t->i32());
    addTestInstructionRegReg(MirInstructionOpCode::SUB, t->i64(), t->i64());
    addTestInstructionRegReg(MirInstructionOpCode::AND, t->i32(), t->i32());
    addTestInstructionRegReg(MirInstructionOpCode::XOR, t->i64(), t->i64());
    addTestInstructionRegReg(MirInstructionOpCode::CMP, t->i32(), t->i32());
    addTestInstructionRegReg(MirInstructionOpCode::IDIV, t->i64(), t->i64());

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirInstructionSelectorPass *pass =
            runPass<MirInstructionSelectorPass>(getBuilderCtx(), createInstructionSelector().get());

    InstructionSelectorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyInstructionSelected(block, 0, TargetInst::ADD32rr)
            .verifyInstructionSelected(block, 1, TargetInst::SUB64rr)
            .verifyInstructionSelected(block, 2, TargetInst::AND32rr)
            .verifyInstructionSelected(block, 3, TargetInst::XOR64rr)
            .verifyInstructionSelected(block, 4, TargetInst::CMP32rr)
            .verifyInstructionSelected(block, 5, TargetInst::IDIV64r);

    MirInstructionVerifier(block->at(0)).operandVerifier(0).verifyRegister(t->i32(), true, MIRID_INVALID);
    MirInstructionVerifier(block->at(1)).operandVerifier(0).verifyRegister(t->i64(), true, MIRID_INVALID);
}

// =========================================================================
// 3. CASTING & CONTROL FLOW SELECTION
// =========================================================================
TEST_F(TestMirInstructionSelectorPass, SelectCastingAndControlFlow)
{
    /**
     * Input:
     *  ZEXT  i32 %dest, i8 %src
     *  SEXT  i32 %dest, i8 %src
     *  FPEXT f64 %dest, f32 %src
     *  JE    %targetBlock
     *  JMP   %targetBlock
     */
    const auto *t = getTypeTable();
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *reg8 = oBuilder.buildVReg(t->i8(), "r8");
    MirRegister *reg32 = oBuilder.buildVReg(t->i32(), "r32");
    MirRegister *regF32 = oBuilder.buildVReg(t->f32(), "rf32");
    MirRegister *regF64 = oBuilder.buildVReg(t->f64(), "rf64");

    // 0: ZEXT i8 -> i32
    iBuilder.ZEXT(reg32, reg8);
    // 1: SEXT i8 -> i32
    iBuilder.SEXT(reg32, reg8);
    // 2: FPEXT f32 -> f64
    iBuilder.FPEXT(regF64, regF32);
    // 3: JE targetBlock
    iBuilder.JE(oBuilder.buildRef(getTestFunc()->getEntryPoint()));
    // 4: JMP targetBlock
    iBuilder.JMP(oBuilder.buildRef(getTestFunc()->getEntryPoint()));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirInstructionSelectorPass *pass = runPass<MirInstructionSelectorPass>(getBuilderCtx(), getInstrSelector());

    InstructionSelectorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyInstructionSelected(block, 0, TargetInst::MOVZX32rr8)
            .verifyInstructionSelected(block, 1, TargetInst::MOVSX32rr8)
            .verifyInstructionSelected(block, 2, TargetInst::CVTSS2SDrr)
            .verifyInstructionSelected(block, 3, TargetInst::JE)
            .verifyInstructionSelected(block, 4, TargetInst::JMP);
}

// =========================================================================
// 4. CALL, RET AND SYSTEM INSTRUCTIONS SELECTION
// =========================================================================
TEST_F(TestMirInstructionSelectorPass, SelectCallRetAndSystem)
{
    /**
     * Input:
     *  CALL void* %callee
     *  NOP
     *  RET
     */
    auto *t = getTypeTable();
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirReference *calleeRef = oBuilder.buildRef(getTestFunc());

    // 0: CALL
    iBuilder.CALL(calleeRef);
    // 1: NOP
    iBuilder.NOP();
    // 2: RET
    iBuilder.RET();

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirInstructionSelectorPass *pass = runPass<MirInstructionSelectorPass>(getBuilderCtx(), getInstrSelector());

    InstructionSelectorPassVerifier verifier(getBuilderCtx(), pass);
    verifier.verifyInstructionSelected(block, 0, TargetInst::CALL)
            .verifyInstructionSelected(block, 1, TargetInst::NOP)
            .verifyInstructionSelected(block, 2, TargetInst::RET);

    MirInstructionVerifier(block->at(0))
            .operandVerifier(0)
            .verifyReference(calleeRef->getRefId(), calleeRef->getRefType());
}