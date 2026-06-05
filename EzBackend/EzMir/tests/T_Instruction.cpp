#include <gtest/gtest.h> // Ensure the IDE recognises this file as a gtest source.
#include "MirTestSuite.h"

class InstrTest : public MirTestSuiteAsGtest
{
};

TEST_F(InstrTest, SimpleBuild)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *op1 = oBuilder.build<MirRegister>(getTypeTable()->getInt8Type(), false, 1, nullptr, "testReg1");
    MirRegister *op2 = oBuilder.build<MirRegister>(getTypeTable()->getInt16Type(), false, 2, nullptr, "testReg2");

    MirInstructionVerifier verifier(iBuilder.ADD(op1, op2));

    verifier.operandCount(2).opcode(MirInstructionOpCode::ADD).targetId(MIRID_INVALID);
    verifier.operandVerifier(0).verifyRegister(getTypeTable()->getInt8Type(), false, 1);
    verifier.operandVerifier(1).verifyRegister(getTypeTable()->getInt16Type(), false, 2);
}