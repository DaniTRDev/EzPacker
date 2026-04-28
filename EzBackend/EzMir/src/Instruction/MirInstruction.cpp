#include "Instruction/MirInstruction.h"

MirInstruction::MirInstruction(MirInstructionOpCode opcode, TypedPoolSlice<MirOperand> *operands) :
    m_opcode(opcode), m_operands(operands)
{
}

bool MirInstruction::hasOperands() const { return m_operands && m_operands->m_numElems > 0; }

const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

size_t MirInstruction::getLoweredOpCode() const { return m_loweredOpCode; }

TypedPoolSlice<MirOperand> *MirInstruction::getOperands() const { return m_operands; }

uint32_t MirInstruction::getFlags() const { return getMeta(getOpCode()).m_flags; }

void MirInstruction::setLoweredOpCode(size_t loweredOpCode) { m_loweredOpCode = loweredOpCode; }

std::string MirInstruction::toString() const
{
    std::string res = getMetadata().m_name;

    for (auto operand : *m_operands)
    {
        res += " " + operand->toString();
    }
    return res;
}
