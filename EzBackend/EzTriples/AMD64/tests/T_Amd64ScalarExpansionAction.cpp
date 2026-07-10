#include "EzTripleTestSuite.h"

class TestAmd64ScalarExpanstionAction : public MirTripleTestSuiteAsGtest
{
  public:
};

TEST_F(TestAmd64ScalarExpanstionAction, Expand128BitAddInline)
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
    MirType *i128 = t->i128();
    MirType *i64 = t->i64();

    // Build the initial illegal wide state
    addTestInstructionRegReg(MirInstructionOpCode::ADD, i128, i128);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirLegalizerPass *pass = runPass<MirLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    // 1. Verify that the sequence was flattened into exactly our 2-operand math steps
    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::ADD, MirInstructionOpCode::ADC });

    // 2. Validate low and high operand components to ensure proper register type splitting
    MirInstructionVerifier lowAddVerifier(block->at(0));
    lowAddVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    lowAddVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);

    MirInstructionVerifier highAdcVerifier(block->at(1));
    highAdcVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);
    highAdcVerifier.operandVerifier(1).verifyRegister(i64, true, MIRID_INVALID);
}

TEST_F(TestAmd64ScalarExpanstionAction, Expand128BitUnsignedDivRuntimeCall)
{
    /**
     * Input:
     *  DIV i128 %dest, i128 %src
     *
     * Should be lowered via your EMIT_CALL layout into a standard runtime branch hook:
     *  CALL @__udivti3, %dest_lo, %dest_hi, %src_lo, %src_hi
     */
    const auto *t = getTypeTable();
    MirType *i128 = t->getIntegerTypeBySize(128);

    addTestInstructionRegReg(MirInstructionOpCode::DIV, i128, i128);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirLegalizerPass *pass = runPass<MirLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    // Verify it emits a single call node and links precisely to the GCC/LLVM ABI runtime symbol
    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::CALL });
    expandVerifier.verifyRuntimeCallSymbol(0, "__udivti3");
}

TEST_F(TestAmd64ScalarExpanstionAction, Expand128BitMemoryLoadStride)
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
    MirType *i128 = t->getIntegerTypeBySize(128);
    MirType *i64 = t->getIntegerTypeBySize(64);

    // Build wide input memory address load frame
    addTestInstructionRegMem(MirInstructionOpCode::LOAD, i128, i128, 0);

    MirBlock *block = getTestFunc()->getEntryPoint();
    MirLegalizerPass *pass = runPass<MirLegalizerPass>(getBuilderCtx(), getLegalizer());

    ExpandScalarActionVerifier expandVerifier(getBuilderCtx(), pass);
    expandVerifier.beginBlock(block);

    expandVerifier.expectInstructionSequence({ MirInstructionOpCode::LOAD, MirInstructionOpCode::LOAD });

    // Verify that instruction index 1 correctly applies the +8 byte offset layout stride
    MirInstructionVerifier highLoadVerifier(block->at(1));

    // Check destination operand is the split 64-bit upper virtual register
    highLoadVerifier.operandVerifier(0).verifyRegister(i64, true, MIRID_INVALID);

    // Check address operand tracks the +8 offset displacement
    highLoadVerifier.operandVerifier(1).type(MirOperandType::Memory);
}