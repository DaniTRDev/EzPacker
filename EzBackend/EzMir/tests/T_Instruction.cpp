#include <gtest/gtest.h> // Ensure the IDE recognises this file as a gtest source.
#include "MirTestSuite.h"

class InstrTest : public MirTestSuiteAsGtest
{
};

TEST_F(InstrTest, SimpleBuild)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *op1 = oBuilder.build<MirRegister>(getTypeTable()->i8(), false, 1, nullptr, "testReg1");
    MirRegister *op2 = oBuilder.build<MirRegister>(getTypeTable()->i16(), false, 2, nullptr, "testReg2");

    MirInstructionVerifier verifier(iBuilder.ADD(op1, op2));

    verifier.operandCount(2).opcode(MirInstructionOpCode::ADD).targetId(MIRID_INVALID);
    verifier.operandVerifier(0).verifyRegister(getTypeTable()->i8(), false, 1);
    verifier.operandVerifier(1).verifyRegister(getTypeTable()->i16(), false, 2);
}

TEST_F(InstrTest, TargetIdSelected)
{
    MirInstructionBuilder iBuilder(getBuilderCtx(), getTestInsertionPoint());
    MirOperandBuilder oBuilder(getBuilderCtx());

    MirRegister *op1 = oBuilder.build<MirRegister>(getTypeTable()->i8(), false, 1, nullptr, "testReg1");
    MirRegister *op2 = oBuilder.build<MirRegister>(getTypeTable()->i16(), false, 2, nullptr, "testReg2");
    MirInstruction *instr = iBuilder.ADD(op1, op2);
    instr->setTargetId(10);

    MirInstructionVerifier verifier(instr);

    verifier.operandCount(2).opcode(MirInstructionOpCode::ADD).targetId(10);
    verifier.operandVerifier(0).verifyRegister(getTypeTable()->i8(), false, 1);
    verifier.operandVerifier(1).verifyRegister(getTypeTable()->i16(), false, 2);
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

    MirBlockInstructionQuery query(getTestInsertionPoint()->m_block);
    query.forEach([](MirInstruction *instr)
            {
                MirInstructionVerifier verifier(instr);
                verifier.operandCount(0).opcode(MirInstructionOpCode::NOP).targetId(MIRID_INVALID);
            });
}