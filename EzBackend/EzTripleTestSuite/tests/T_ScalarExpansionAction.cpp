/**
 * @file TestScalarExpansionAction.cpp
 * @brief Unit tests verifying recursive scalar expansion and multi-precision lowering
 * for arithmetic, memory, calls, and runtime library routing.
 */

#include "EzTripleTestSuite.h"

class TestScalarExpanstionAction : public MirTripleTestSuiteAsGtest
{
  public:
};

// =========================================================================
// 1. 128-BIT ADD (REGISTER - REGISTER)
// =========================================================================
TEST_F(TestScalarExpanstionAction, Expand128BitAddRegReg)
{
    /**
     * Input:
     *  ADD i128 %dest, i128 %src
     *
     * Expanded 2-operand carry chain:
     *  ADD i64 %dest_lo, i64 %src_lo
     *  ADC i64 %dest_hi, i64 %src_hi
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128();
    MirType *i64 = t->i64();

    // Build the initial illegal wide state
    addTestInstructionRegReg(MirInstructionOpCode::ADD, i128, i128);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getTargetDesc());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    // Verify sequence: ADD (sets CF) -> ADC (consumes CF)
    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::ADD, MirInstructionOpCode::ADC });

    // Validate low 64-bit pair
    MirInstructionVerifier lowAddVerifier(block->at(0));
    lowAddVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowAddVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    // Validate high 64-bit pair
    MirInstructionVerifier highAdcVerifier(block->at(1));
    highAdcVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highAdcVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);
}

// =========================================================================
// 2. 128-BIT CALL WITH REGISTER ARGUMENT
// =========================================================================
TEST_F(TestScalarExpanstionAction, Expand128BitCallReg)
{
    /**
     * Input:
     *  CALL %testFunc, i128 %param1
     *
     * Lowered / Expanded tokenized call:
     *  PUSH_ARG __bindToken %token, i64 %src_lo
     *  PUSH_ARG __bindToken %token, i64 %src_hi
     *  CALL __bindToken %token, %testFunc
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128();
    MirType *i64 = t->i64();
    MirType *tokenType = t->getBindingToken();

    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());
    builder.CALL(opBuilder.buildRef(getTestFunc()), opBuilder.buildVReg(i128, "testParam"));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getTargetDesc());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence(
            { MirInstructionOpCode::PUSH_ARG, MirInstructionOpCode::PUSH_ARG, MirInstructionOpCode::CALL });

    // Verify Token binding is preserved in operand 0 and arguments are 64-bit registers
    MirInstructionVerifier lowPushVerifier(block->at(0));
    lowPushVerifier.operandVerifier(0).verifyRegister(tokenType, true, MIRID_INVALID);
    lowPushVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    MirInstructionVerifier highPushVerifier(block->at(1));
    highPushVerifier.operandVerifier(0).verifyRegister(tokenType, true, MIRID_INVALID);
    highPushVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);
}

// =========================================================================
// 3. 256-BIT CALL WITH IMMEDIATE ARGUMENT
// =========================================================================
TEST_F(TestScalarExpanstionAction, Expand256BitCallImm)
{
    /**
     * Input:
     *  CALL %testFunc, i256 0xBA50B51C48B0AD92_3D18198EC90D2F08_FF9FB76E997408E4_73A37C572B714B52
     *
     * Lowered into 4 consecutive 64-bit PUSH_ARG instructions (Little-Endian):
     *  PUSH_ARG __bindToken %token, i64 0x73A37C572B714B52
     *  PUSH_ARG __bindToken %token, i64 0xFF9FB76E997408E4
     *  PUSH_ARG __bindToken %token, i64 0x3D18198EC90D2F08
     *  PUSH_ARG __bindToken %token, i64 0xBA50B51C48B0AD92
     *  CALL __bindToken %token, %testFunc
     */
    const auto *t = getTypeTable();
    MirType *i256 = t->i256();
    MirType *i64 = t->i64();
    MirType *tokenType = t->getBindingToken();

    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());
    builder.CALL(opBuilder.buildRef(getTestFunc()),
                 opBuilder.buildInt(
                         i256,
                         FlexInt("BA50B51C48B0AD923D18198EC90D2F08FF9FB76E997408E473A37C572B714B52", 256, false, 16)));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getTargetDesc());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::CALL });

    // Validate token binding and 64-bit immediate values in Little-Endian order
    MirInstructionVerifier lowLowPush(block->at(0));
    lowLowPush.operandVerifier(0).verifyRegister(tokenType, true, MIRID_INVALID);
    lowLowPush.operandVerifier(1).verifyInteger(i64, FlexInt("73A37C572B714B52", 64, false, 16));

    MirInstructionVerifier lowHighPush(block->at(1));
    lowHighPush.operandVerifier(0).verifyRegister(tokenType, true, MIRID_INVALID);
    lowHighPush.operandVerifier(1).verifyInteger(i64, FlexInt("FF9FB76E997408E4", 64, false, 16));

    MirInstructionVerifier highLowPush(block->at(2));
    highLowPush.operandVerifier(0).verifyRegister(tokenType, true, MIRID_INVALID);
    highLowPush.operandVerifier(1).verifyInteger(i64, FlexInt("3D18198EC90D2F08", 64, false, 16));

    MirInstructionVerifier highHighPush(block->at(3));
    highHighPush.operandVerifier(0).verifyRegister(tokenType, true, MIRID_INVALID);
    highHighPush.operandVerifier(1).verifyInteger(i64, FlexInt("BA50B51C48B0AD92", 64, false, 16));
}

// =========================================================================
// 4. 256-BIT ADD (REGISTER - IMMEDIATE)
// =========================================================================
TEST_F(TestScalarExpanstionAction, Expand256BitAddRegImm)
{
    /**
     * Input:
     *  ADD i256 %dest, i256 0xBA50B51C48B0AD92_3D18198EC90D2F08_FF9FB76E997408E4_73A37C572B714B52
     *
     * Iterative expansion produces a 4-stage carry chain:
     *  ADD i64 %dest_0, i64 0x73A37C572B714B52
     *  ADC i64 %dest_1, i64 0xFF9FB76E997408E4
     *  ADC i64 %dest_2, i64 0x3D18198EC90D2F08
     *  ADC i64 %dest_3, i64 0xBA50B51C48B0AD92
     */
    const auto *t = getTypeTable();
    MirType *i256 = t->i256();
    MirType *i64 = t->i64();

    addTestInstructionRegIntImm(
            MirInstructionOpCode::ADD,
            i256,
            i256,
            FlexInt("BA50B51C48B0AD923D18198EC90D2F08FF9FB76E997408E473A37C572B714B52", 256, false, 16));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getTargetDesc());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::ADD,
                                               MirInstructionOpCode::ADC,
                                               MirInstructionOpCode::ADC,
                                               MirInstructionOpCode::ADC });

    // Validate 4-stage carry chain
    MirInstructionVerifier lowLowAdd(block->at(0));
    lowLowAdd.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowLowAdd.operandVerifier(1).verifyInteger(i64, FlexInt("73A37C572B714B52", 64, false, 16));

    MirInstructionVerifier lowHighAdc(block->at(1));
    lowHighAdc.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowHighAdc.operandVerifier(1).verifyInteger(i64, FlexInt("FF9FB76E997408E4", 64, false, 16));

    MirInstructionVerifier highLowAdc(block->at(2));
    highLowAdc.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highLowAdc.operandVerifier(1).verifyInteger(i64, FlexInt("3D18198EC90D2F08", 64, false, 16));

    MirInstructionVerifier highHighAdc(block->at(3));
    highHighAdc.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highHighAdc.operandVerifier(1).verifyInteger(i64, FlexInt("BA50B51C48B0AD92", 64, false, 16));
}

// =========================================================================
// 5. 128-BIT ADD + SUB WITH REUSED OPERAND
// =========================================================================
TEST_F(TestScalarExpanstionAction, Expand128BitAddRegRegAndReuse)
{
    /**
     * Input:
     *  ADD i128 %dest1, i128 %src
     *  SUB i128 %dest2, i128 %src
     *
     * Expanded:
     *  ADD i64 %dest1_lo, i64 %src_lo
     *  ADC i64 %dest1_hi, i64 %src_hi
     *  SUB i64 %dest2_lo, i64 %src_lo
     *  SBB i64 %dest2_hi, i64 %src_hi
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128();
    MirType *i64 = t->i64();

    auto *instr1 = addTestInstructionRegReg(MirInstructionOpCode::ADD, i128, i128);
    auto *instr2 = addTestInstructionRegReg(MirInstructionOpCode::SUB, i128, i128);

    // Reuse src operand
    instr2->getOperands()[1] = instr1->getOperands()[1];

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getTargetDesc());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::ADD,
                                               MirInstructionOpCode::ADC,
                                               MirInstructionOpCode::SUB,
                                               MirInstructionOpCode::SBB });

    // Validate ADD/ADC
    MirInstructionVerifier lowAdd1(block->at(0));
    lowAdd1.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowAdd1.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    MirInstructionVerifier highAdc1(block->at(1));
    highAdc1.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highAdc1.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    // Validate SUB/SBB
    MirInstructionVerifier lowSub2(block->at(2));
    lowSub2.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowSub2.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    MirInstructionVerifier highSbb2(block->at(3));
    highSbb2.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highSbb2.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);
}

// =========================================================================
// 6. 128-BIT UNSIGNED DIVISION (RUNTIME CALL ROUTING)
// =========================================================================
TEST_F(TestScalarExpanstionAction, Expand128BitUDivRegReg)
{
    /**
     * Input:
     *  DIV i128 %dest, i128 %src
     *
     * Lowered into __udivti3 call passing 4 64-bit pieces:
     *  PUSH_ARG __bindToken %token, i64 %dest_lo
     *  PUSH_ARG __bindToken %token, i64 %dest_hi
     *  PUSH_ARG __bindToken %token, i64 %src_lo
     *  PUSH_ARG __bindToken %token, i64 %src_hi
     *  CALL __bindToken %token, @__udivti3
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128();

    addTestInstructionRegReg(MirInstructionOpCode::DIV, i128, i128);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getTargetDesc());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::CALL });
    expandVerifier.verifyRuntimeCallSymbol(4, "__udivti3");
}

// =========================================================================
// 7. 128-BIT LOAD FROM MEMORY
// =========================================================================
TEST_F(TestScalarExpanstionAction, Expand128BitLoadRegMem)
{
    /**
     * Input:
     *  LOAD i128 %dest, [mem_addr]
     *
     * Expanded into two 64-bit loads with +8 byte stride:
     *  LOAD i64 %dest_lo, [mem_addr + 0]
     *  LOAD i64 %dest_hi, [mem_addr + 8]
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128();
    MirType *i64 = t->i64();

    addTestInstructionRegMem(MirInstructionOpCode::LOAD, i128, i128, FlexInt(0, 64));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getTargetDesc());
    MirOperandBuilder opBuilder(getBuilderCtx());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::LOAD, MirInstructionOpCode::LOAD });

    // Verify low load has offset 0
    MirInstructionVerifier lowLoadVerifier(block->at(0));
    MirInteger *displ0 = opBuilder.buildInt(i64, FlexInt(0, 64));
    lowLoadVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowLoadVerifier.operandVerifier(1).verifyMemory(i64, nullptr, displ0);

    // Verify high load has offset +8
    MirInstructionVerifier highLoadVerifier(block->at(1));
    MirInteger *displ8 = opBuilder.buildInt(i64, FlexInt(8, 64));
    highLoadVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highLoadVerifier.operandVerifier(1).verifyMemory(i64, nullptr, displ8);
}

// =========================================================================
// 8. 256-BIT LOAD FROM MEMORY
// =========================================================================
TEST_F(TestScalarExpanstionAction, Expand256BitLoadRegMem)
{
    /**
     * Input:
     *  LOAD i256 %dest, [mem_addr]
     *
     * Expanded into 4 consecutive 64-bit loads with 8-byte strides:
     *  LOAD i64 %dest_0, [mem_addr + 0]
     *  LOAD i64 %dest_1, [mem_addr + 8]
     *  LOAD i64 %dest_2, [mem_addr + 16]
     *  LOAD i64 %dest_3, [mem_addr + 24]
     */
    const auto *t = getTypeTable();
    MirType *i256 = t->i256();
    MirType *i64 = t->i64();

    addTestInstructionRegMem(MirInstructionOpCode::LOAD, i256, i256, FlexInt(0, 64));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getTargetDesc());
    MirOperandBuilder opBuilder(getBuilderCtx());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::LOAD,
                                               MirInstructionOpCode::LOAD,
                                               MirInstructionOpCode::LOAD,
                                               MirInstructionOpCode::LOAD });

    for (size_t i = 0; i < 4; ++i)
    {
        MirInstructionVerifier loadVerifier(block->at(i));
        MirInteger *displ = opBuilder.buildInt(i64, FlexInt(static_cast<int64_t>(i * 8), 64));

        loadVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
        loadVerifier.operandVerifier(1).verifyMemory(i64, nullptr, displ);
    }
}