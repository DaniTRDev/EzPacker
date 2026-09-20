#include <gtest/gtest.h>
#include "EzMirTestSuite.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"
#include <stdexcept>

/**
 * Test fixture for MIR instruction building, opcode assignments, and operand bindings.
 */
class InstrTest : public MirTestSuiteAsGtest
{
  public:
  protected:
};

namespace
{

/**
 * Custom GoogleTest assertion verifying that an instruction has the expected opcode and operand count.
 */
::testing::AssertionResult
IsInstruction(MirInstruction *instr, MirInstructionOpCode expectedOpcode, size_t expectedOperandCount)
{
    if (!instr)
        return ::testing::AssertionFailure() << "Instruction is nullptr";

    if (instr->getOpCode() != expectedOpcode)
        return ::testing::AssertionFailure() << "Expected opcode " << static_cast<int>(expectedOpcode) << ", got "
                                             << static_cast<int>(instr->getOpCode());

    if (instr->getOperands().size() != expectedOperandCount)
        return ::testing::AssertionFailure()
                << "Expected " << expectedOperandCount << " operands, got " << instr->getOperands().size();

    return ::testing::AssertionSuccess();
}

/**
 * Custom GoogleTest assertion verifying that an operand is a virtual register of the expected MIR type.
 */
::testing::AssertionResult IsVirtualRegister(MirOperand *operand, MirType *expectedType)
{
    if (!operand)
        return ::testing::AssertionFailure() << "Operand is nullptr";

    if (operand->getType() != MirOperandType::Register)
        return ::testing::AssertionFailure()
                << "Operand is not a register, got type " << static_cast<int>(operand->getType());

    MirRegister *reg = static_cast<MirRegister *>(operand);

    if (!reg->isVirtual())
        return ::testing::AssertionFailure() << "Expected virtual register, but register is physical";

    if (expectedType && reg->getMirType() != expectedType)
        return ::testing::AssertionFailure()
                << "Expected register type " << expectedType << ", got " << reg->getMirType();

    return ::testing::AssertionSuccess();
}
} // anonymous namespace

/**
 * Verifies basic instruction generation by constructing an ADD instruction with two virtual registers
 * (i8 destination and i16 source) at the test insertion point.
 */
TEST_F(InstrTest, SimpleBuild)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *op1 = oBuilder.buildVReg(getTypeTable()->i8(), "testReg1");
    MirRegister *op2 = oBuilder.buildVReg(getTypeTable()->i16(), "testReg2");

    MirInstruction *instr = iBuilder.ADD(op1, op2);

    ASSERT_TRUE(IsInstruction(instr, MirInstructionOpCode::ADD, 2));

    EXPECT_TRUE(IsVirtualRegister(instr->getOperands()[0], getTypeTable()->i8()));
    EXPECT_TRUE(IsVirtualRegister(instr->getOperands()[1], getTypeTable()->i16()));
}

/**
 * LEG-10: the variadic expansion slot encoded in the generated metadata must give every extra
 * destination of UNMERGE_VALUES a Write flag, while every extra operand of MERGE_VALUES stays a Read.
 */
TEST_F(InstrTest, VariadicOperandFlags)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirType *i32 = getTypeTable()->i32();

    // UNMERGE_VALUES: [dst0, dst1, src] -> Write, Write, Read.
    MirInstruction *unmerge = iBuilder.build(MirInstructionOpCode::UNMERGE_VALUES,
                                             nullptr,
                                             { oBuilder.buildVReg(i32, "lo"),
                                               oBuilder.buildVReg(i32, "hi"),
                                               oBuilder.buildVReg(getTypeTable()->i64(), "wide") });
    ASSERT_NE(unmerge, nullptr);
    EXPECT_TRUE(unmerge->getOperandFlag(0) & MirOperandFlag::Write);
    EXPECT_FALSE(unmerge->getOperandFlag(0) & MirOperandFlag::Read);
    EXPECT_TRUE(unmerge->getOperandFlag(1) & MirOperandFlag::Write);
    EXPECT_FALSE(unmerge->getOperandFlag(1) & MirOperandFlag::Read);
    EXPECT_TRUE(unmerge->getOperandFlag(2) & MirOperandFlag::Read);
    EXPECT_FALSE(unmerge->getOperandFlag(2) & MirOperandFlag::Write);

    // MERGE_VALUES: [dst, src0, src1, src2] -> Write, Read, Read, Read.
    MirInstruction *merge = iBuilder.build(MirInstructionOpCode::MERGE_VALUES,
                                           nullptr,
                                           { oBuilder.buildVReg(getTypeTable()->i64(), "wide_dst"),
                                             oBuilder.buildVReg(i32, "a"),
                                             oBuilder.buildVReg(i32, "b"),
                                             oBuilder.buildVReg(i32, "c") });
    ASSERT_NE(merge, nullptr);
    EXPECT_TRUE(merge->getOperandFlag(0) & MirOperandFlag::Write);
    for (size_t i = 1; i < 4; ++i)
    {
        EXPECT_TRUE(merge->getOperandFlag(i) & MirOperandFlag::Read);
        EXPECT_FALSE(merge->getOperandFlag(i) & MirOperandFlag::Write);
    }
}

/**
 * WEI-12: the instruction builder reports programming errors through one channel. Null instructions
 * or operands and out-of-range operand indices must throw std::runtime_error rather than no-op.
 */
TEST_F(InstrTest, BuilderRejectsInvalidOperands)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirInstruction *instr = iBuilder.build(MirInstructionOpCode::ADD,
                                           nullptr,
                                           { oBuilder.buildVReg(getTypeTable()->i32(), "lhs"),
                                             oBuilder.buildVReg(getTypeTable()->i32(), "rhs") });
    ASSERT_NE(instr, nullptr);

    EXPECT_THROW(iBuilder.addOperand(nullptr, nullptr), std::runtime_error);
    EXPECT_THROW(iBuilder.clearOperand(nullptr, 0), std::runtime_error);
    EXPECT_THROW(iBuilder.clearOperand(instr, 5), std::runtime_error);
    EXPECT_THROW(iBuilder.swapOperand(instr, oBuilder.buildVReg(getTypeTable()->i32(), "fresh"), 5),
                 std::runtime_error);
}