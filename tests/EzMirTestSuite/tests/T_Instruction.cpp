#include <gtest/gtest.h>
#include "EzMirTestSuite.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirTypeTable.h"

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