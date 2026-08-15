#include "EzMirTestSuite.h"

class InstrTest : public MirTestSuiteAsGtest
{
};

TEST_F(InstrTest, SimpleBuild)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *op1 = oBuilder.buildVReg(getTypeTable()->i8(), "testReg1");
    MirRegister *op2 = oBuilder.buildVReg(getTypeTable()->i16(), "testReg2");

    MirInstructionVerifier verifier(iBuilder.ADD(op1, op2));

    verifier.operandCount(2).opcode(MirInstructionOpCode::ADD);
    verifier.operandVerifier(0).verifyRegister(getTypeTable()->i8(), true, MIRID_INVALID);
    verifier.operandVerifier(1).verifyRegister(getTypeTable()->i16(), true, MIRID_INVALID);
}

TEST_F(InstrTest, Multiple)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    iBuilder.NOP();
    iBuilder.NOP();
    iBuilder.NOP();
    iBuilder.NOP();
    iBuilder.NOP();

    MirBlockInstructionQuery query(getTestInsertionPoint().m_block);
    query.forEach(
            [](MirInstruction *instr)
            {
                MirInstructionVerifier verifier(instr);
                verifier.operandCount(0).opcode(MirInstructionOpCode::NOP);
            });
}
