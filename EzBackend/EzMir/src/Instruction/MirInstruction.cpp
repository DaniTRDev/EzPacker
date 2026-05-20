#include "Instruction/MirInstruction.h"

MirInstruction::MirInstruction(MirInstructionOpCode opcode, TypedPoolLinkedList<MirOperand> *operands) :
    m_opcode(opcode), m_operands(operands)
{
}

bool MirInstruction::hasOperands() const { return m_operands && m_operands->m_numElems > 0; }

bool MirInstruction::isSigned() const { return getMetadata().m_flags & MirInstructionFlags::TreatAsSigned; }

const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

const MirInstructionLinearEquivalent &MirInstruction::getLinearEquivalent() const
{
    return getMetadata().m_linearEquivalent;
}

MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

TypedPoolLinkedList<MirOperand> *MirInstruction::getOperands() const { return m_operands; }

MirInstructionFlags MirInstruction::getFlags() const { return getMeta(getOpCode()).m_flags; }

MirTargetInstructionId MirInstruction::getTargetId() const { return m_targetId; }

void MirInstruction::setTargetId(MirTargetInstructionId id) { m_targetId = id; }

std::string MirInstruction::toString() const
{
    std::string res;
    res += getMetadata().m_name;

    for (auto operand : *m_operands)
    {
        res += " " + operand->toString() + ",";
    }
    return res;
}
