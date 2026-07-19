#include "EzTripleTestSuite.h"

class TestScalarExpanstionAction : public MirTripleTestSuiteAsGtest
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

TEST_F(TestScalarExpanstionAction, Expand128BitAddRegReg)
{
    /**
     * Input:
     *  ADD i128 %dest, i128 %src
     *
     * Should be expanded into a 2-operand inline carry chain:
     *  ADD i64 %dest_lo, i64 %src_lo
     *  ADC i64 %dest_hi, i64 %src_hi
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128(), *i64 = t->i64();

    // Build the initial illegal wide state
    addTestInstructionRegReg(MirInstructionOpCode::ADD, i128, i128);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    // Verify that the sequence was flattened into exactly our 2-operand math steps
    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::ADD, MirInstructionOpCode::ADC });

    // Validate low and high operand components to ensure proper register type splitting
    MirInstructionVerifier lowAddVerifier(block->at(0));
    lowAddVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowAddVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    MirInstructionVerifier highAdcVerifier(block->at(1));
    highAdcVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highAdcVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);
}

TEST_F(TestScalarExpanstionAction, Expand128BitCallReg)
{
    /**
     * Input:
     *  call %testFunc, i128 %param1
     *
     * Should be expanded into a 2-operand inline carry chain:
     *  PUSH_ARG __bindToken %token, i64 %src_lo
     *  PUSH_ARG __bindToken %token, i64 %src_hi
     *  call __bindToken %token, %testFunc
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128(), *i64 = t->i64();

    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());
    builder.CALL(opBuilder.buildRef(getTestFunc()), opBuilder.buildVReg(i128, "testParam"));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence(
            { MirInstructionOpCode::PUSH_ARG, MirInstructionOpCode::PUSH_ARG, MirInstructionOpCode::CALL });

    // Validate low and high operand components to ensure proper register type splitting
    MirInstructionVerifier lowPushVerifier(block->at(0));
    lowPushVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    MirInstructionVerifier highPushVerifier(block->at(1));
    highPushVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);
}

TEST_F(TestScalarExpanstionAction, Expand256BitCallImm)
{
    /**
     * Input:
     *  call %testFunc, i256 0xBA50B51C48B0AD923D18198EC90D2F08FF9FB76E997408E473A37C572B714B52
     *
     * Should be expanded into a 2-operand inline carry chain:
     *  PUSH_ARG __bindToken %token, i64 0x73A37C572B714B52
     *  PUSH_ARG __bindToken %token, i64 0xFF9FB76E997408E4
     *  PUSH_ARG __bindToken %token, i64 0x3D18198EC90D2F08
     *  PUSH_ARG __bindToken %token, i64 0xBA50B51C48B0AD92
     *  call __bindToken %token, %testFunc
     */
    const auto *t = getTypeTable();
    MirType *i256 = t->i256(), *i64 = t->i64();

    MirInstructionBuilder builder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder opBuilder(getBuilderCtx());
    builder.CALL(opBuilder.buildRef(getTestFunc()),
                 opBuilder.buildInt(
                         i256,
                         FlexInt("BA50B51C48B0AD923D18198EC90D2F08FF9FB76E997408E473A37C572B714B52", 256, false, 16)));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::CALL });

    // Validate low and high operand components to ensure proper register type splitting
    MirInstructionVerifier lowLowAddVerifier(block->at(0));
    lowLowAddVerifier.operandVerifier(1).verifyInteger(i64, FlexInt("73A37C572B714B52", 64, false, 16));

    MirInstructionVerifier lowHighAdc1Verifier(block->at(1));
    lowHighAdc1Verifier.operandVerifier(1).verifyInteger(i64, FlexInt("FF9FB76E997408E4", 64, false, 16));

    MirInstructionVerifier highLowAdc1Verifier(block->at(2));
    highLowAdc1Verifier.operandVerifier(1).verifyInteger(i64, FlexInt("3D18198EC90D2F08", 64, false, 16));

    MirInstructionVerifier highHighAdc1Verifier(block->at(3));
    highHighAdc1Verifier.operandVerifier(1).verifyInteger(i64, FlexInt("BA50B51C48B0AD92", 64, false, 16));
}

TEST_F(TestScalarExpanstionAction, Expand256BitAddRegImm)
{
    /**
     * Input:
     *  ADD i256 %dest, i256 0xBA50B51C48B0AD923D18198EC90D2F08FF9FB76E997408E473A37C572B714B52
     *
     * Should be expanded into a 2-operand inline carry chain:
     * FIRST ITERATION
     *  ADD i128 %dest_lo, i128 0xFF9FB76E997408E473A37C572B714B52
     *  ADC i128 %dest_hi, i128 0xBA50B51C48B0AD923D18198EC90D2F08
     *
     * SECOND ITERATION
     *  ADD i64 %dest_lo_lo, i64 0x73A37C572B714B52
     *  ADC i64 %dest_lo_high, i64 0xFF9FB76E997408E4
     *  ADC i64 %dest_hi_lo, i64 0x3D18198EC90D2F08
     *  ADC i64 %dest_hi_hi, i64 0xBA50B51C48B0AD92
     */
    const auto *t = getTypeTable();
    MirType *i256 = t->i256(), *i64 = t->i64();

    // Build the initial illegal wide state
    addTestInstructionRegIntImm(
            MirInstructionOpCode::ADD,
            i256,
            i256,
            FlexInt("BA50B51C48B0AD923D18198EC90D2F08FF9FB76E997408E473A37C572B714B52", 256, false, 16));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    // Verify that the sequence was flattened into exactly our 2-operand math steps
    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::ADD,
                                               MirInstructionOpCode::ADC,
                                               MirInstructionOpCode::ADC,
                                               MirInstructionOpCode::ADC });

    // Validate low and high operand components to ensure proper register type splitting
    MirInstructionVerifier lowLowAddVerifier(block->at(0));
    lowLowAddVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowLowAddVerifier.operandVerifier(1).verifyInteger(i64, FlexInt("73A37C572B714B52", 64, false, 16));

    MirInstructionVerifier lowHighAdc1Verifier(block->at(1));
    lowHighAdc1Verifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowHighAdc1Verifier.operandVerifier(1).verifyInteger(i64, FlexInt("FF9FB76E997408E4", 64, false, 16));

    MirInstructionVerifier highLowAdc1Verifier(block->at(2));
    highLowAdc1Verifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highLowAdc1Verifier.operandVerifier(1).verifyInteger(i64, FlexInt("3D18198EC90D2F08", 64, false, 16));

    MirInstructionVerifier highHighAdc1Verifier(block->at(3));
    highHighAdc1Verifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highHighAdc1Verifier.operandVerifier(1).verifyInteger(i64, FlexInt("BA50B51C48B0AD92", 64, false, 16));
}

TEST_F(TestScalarExpanstionAction, Expand128BitAddRegRegAndReuse)
{
    /**
     * Input:
     *  ADD i128 %dest, i128 %src
     *  SUB i128 %dest2, i128 %src
     *
     * Should be expanded into a 2-operand inline carry chain:
     *  ADD i64 %dest_lo, i64 %src_lo
     *  ADC i64 %dest_hi, i64 %src_hi

     *  SUB i64 %dest2_lo, i64 %src_lo
     *  SBB i64 %dest2_hi, i64 %src_hi
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128(), *i64 = t->i64();

    // Build the initial illegal wide state
    auto instr1 = addTestInstructionRegReg(MirInstructionOpCode::ADD, i128, i128);
    auto instr2 = addTestInstructionRegReg(MirInstructionOpCode::SUB, i128, i128);

    instr2->getOperands()[1] = instr1->getOperands()[1]; // Ensure instr 2 uses same src operand.

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    // Verify that the sequence was flattened into exactly our 2-operand math steps
    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::ADD,
                                               MirInstructionOpCode::ADC,
                                               MirInstructionOpCode::SUB,
                                               MirInstructionOpCode::SBB });

    // Validate low and high operand components to ensure proper register type splitting
    MirInstructionVerifier lowAdd1Verifier(block->at(0));
    lowAdd1Verifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowAdd1Verifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    MirInstructionVerifier highAdc1Verifier(block->at(1));
    highAdc1Verifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highAdc1Verifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    // Validate low and high operand components to ensure proper register type splitting
    MirInstructionVerifier lowAddVerifier(block->at(2));
    lowAddVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowAddVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    MirInstructionVerifier highAdc2Verifier(block->at(3));
    highAdc2Verifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highAdc2Verifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);
}

TEST_F(TestScalarExpanstionAction, Expand128BitUDivRegReg)
{
    /**
     * Input:
     *  DIV i128 %dest, i128 %src
     *
     * Should be lowered via EMIT_CALL into a runtime call:
     *  PUSH_ARG __bindToken %token, i64 %dest_lo
     *  PUSH_ARG __bindToken %token, i64 %dest_high
     *  PUSH_ARG __bindToken %token, i64 src_lo
     *  PUSH_ARG __bindToken %token, i64 src_hi
     *  CALL __bindToken %token, @__udivti3
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128();

    addTestInstructionRegReg(MirInstructionOpCode::DIV, i128, i128);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    // Verify it emits a single call node and links precisely to the GCC/LLVM ABI runtime symbol.
    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::PUSH_ARG,
                                               MirInstructionOpCode::CALL });
    expandVerifier.verifyRuntimeCallSymbol(4, "__udivti3");
}

TEST_F(TestScalarExpanstionAction, Expand128BitLoadRegMem)
{
    /**
     * Input:
     *  LOAD i128 %dest, [mem_addr]
     *
     * Should be expanded into consecutive 64-bit chunk extractions with relative strides:
     *  LOAD i64 %dest_lo, [mem_addr + 0]
     *  LOAD i64 %dest_hi, [mem_addr + 8]  (8-byte offset for half chunk stride)
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->i128(), *i64 = t->i64();

    // Build wide input memory address load frame
    addTestInstructionRegMem(MirInstructionOpCode::LOAD, i128, i128, FlexInt(0, 64));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    MirOperandBuilder opBuilder(getBuilderCtx());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::LOAD, MirInstructionOpCode::LOAD });

    // Verify that instruction index 1 correctly applies the +8 byte offset layout stride
    MirInstructionVerifier highLoadVerifier(block->at(1));
    MirInteger *displ8 = opBuilder.buildInt(i64, FlexInt(8, 64));

    // Check destination operand is the split 64-bit upper virtual register
    highLoadVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highLoadVerifier.operandVerifier(1).verifyMemory(i64, nullptr, displ8);
}

TEST_F(TestScalarExpanstionAction, Expand256BitLoadRegMem)
{
    /**
     * Input:
     *  LOAD i256 %dest, [mem_addr]
     *
     * Should be expanded into consecutive 64-bit chunk extractions with relative strides:
     * FIRST ITERATION:
     *  LOAD i256 %dest_lo, [mem_addr + 0]
     *  LOAD i256 %dest_hi, [mem_addr + 16]  (16-byte offset for half chunk stride)
     * SECOND ITERATION:
     *  LOAD i64 %dest_lo_lo, [mem_addr + 0]
     *  LOAD i64 %dest_lo_hi, [mem_addr + 8]
     *  LOAD i64 %dest_hi_lo, [mem_addr + 16]
     *  LOAD i64 %dest_hi_hi, [mem_addr + 24]
     */
    const auto *t = getTypeTable();
    MirType *i256 = t->i256(), *i64 = t->i64();

    // Build wide input memory address load frame
    addTestInstructionRegMem(MirInstructionOpCode::LOAD, i256, i256, FlexInt(0, 64));

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirBlockLegalizerPass *pass = runPass<MirBlockLegalizerPass>(getBuilderCtx(), getLegalizer());
    MirOperandBuilder opBuilder(getBuilderCtx());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::LOAD,
                                               MirInstructionOpCode::LOAD,
                                               MirInstructionOpCode::LOAD,
                                               MirInstructionOpCode::LOAD });

    for (size_t i = 0; i < 4; i++)
    {
        MirInstructionVerifier highLoadVerifier(block->at(i));
        MirInteger *displ = opBuilder.buildInt(i64, FlexInt(int64_t(i * 8)));

        // Check destination operand is the split 64-bit upper virtual register
        highLoadVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
        highLoadVerifier.operandVerifier(1).verifyMemory(i64, nullptr, displ);
    }
}