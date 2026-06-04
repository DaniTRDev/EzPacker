#include "Instruction/MirInstruction.h"

MirInstruction::MirInstruction(MirInstructionOpCode opcode, std::pmr::vector<MirOperand *> operands) :
    m_opcode(opcode), m_operands(operands)
{
}

bool MirInstruction::hasOperands() const { return !m_operands.empty(); }

bool MirInstruction::isSigned() const { return getMetadata().m_flags & MirInstructionFlags::TreatAsSigned; }

const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

const MirInstructionLinearEquivalent &MirInstruction::getLinearEquivalent() const
{
    return getMetadata().m_linearEquivalent;
}

MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

MirInstructionFlags MirInstruction::getFlags() const { return getMeta(getOpCode()).m_flags; }

MirTargetInstructionId MirInstruction::getTargetId() const { return m_targetId; }

void MirInstruction::addOperand(const MirOperand &operand) { m_operands.emplace_back(operand); }

void MirInstruction::setTargetId(MirTargetInstructionId id) { m_targetId = id; }

std::pmr::vector<MirOperand *> MirInstruction::getOperands() const { return m_operands; }

std::string MirInstruction::toString() const
{
    std::string res;
    res += getMetadata().m_name;

    for (auto operand : m_operands)
    {
        res += " " + operand->toString() + ",";
    }
    return res;
}
