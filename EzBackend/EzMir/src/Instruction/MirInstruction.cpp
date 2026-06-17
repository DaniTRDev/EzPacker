#include "Instruction/MirInstruction.h"

MirInstruction::MirInstruction(class MirBlock *owner,
                               MirInstructionOpCode opcode,
                               SourceReference *ref,
                               std::pmr::vector<MirOperand *> operands) :
    m_owner(owner), m_opcode(opcode), m_targetId(MIRID_INVALID), m_sourceRef(ref), m_operands(std::move(operands))
{
}

bool MirInstruction::hasOperands() const { return !m_operands.empty(); }

bool MirInstruction::isSigned() const { return getMetadata().m_flags & MirInstructionFlags::TreatAsSigned; }

const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

const MirInstructionLinearEquivalent &MirInstruction::getLinearEquivalent() const
{
    return getMetadata().m_linearEquivalent;
}

class MirBlock *MirInstruction::getOwner() { return m_owner; }

MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

MirInstructionFlags MirInstruction::getFlags() const { return getMeta(getOpCode()).m_flags; }

MirTargetInstructionId MirInstruction::getTargetId() const { return m_targetId; }

SourceReference *MirInstruction::getSourceRef() const { return m_sourceRef; }

void MirInstruction::addOperand(const MirOperand *operand) { m_operands.push_back((MirOperand *)operand); }

void MirInstruction::setTargetId(MirTargetInstructionId id) { m_targetId = id; }

const std::pmr::vector<MirOperand *> &MirInstruction::getOperands() const { return m_operands; }

std::pmr::vector<MirOperand *> &MirInstruction::getOperands() { return m_operands; }

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
