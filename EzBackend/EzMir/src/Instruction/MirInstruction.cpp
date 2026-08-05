#include "Instruction/MirInstruction.h"

MirInstruction::MirInstruction(class MirBlock *owner,
                               MirInstructionOpCode opcode,
                               SourceReference *ref,
                               std::pmr::vector<MirOperand *> operands) :
    m_cachedDefinedRegisters(false), m_cachedUsedRegisters(false), m_owner(owner), m_opcode(opcode),
    m_targetId(MIRID_INVALID), m_sourceRef(ref),
    m_definedRegisters(std::pmr::vector<RegisterRef>(operands.get_allocator().resource())),
    m_usedRegisters(std::pmr::vector<RegisterRef>(operands.get_allocator().resource())), m_operands(std::move(operands))
{
}

bool MirInstruction::hasOperands() const { return !m_operands.empty(); }

bool MirInstruction::isSigned() const { return getMetadata().m_flags & MirInstructionFlags::TreatAsSigned; }

const char *MirInstruction::getOpCodeName() const { return getMetadata().m_name.data(); }

const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

class MirBlock *MirInstruction::getOwner() { return m_owner; }

MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

MirInstructionFlags MirInstruction::getFlags() const { return getMeta(getOpCode()).m_flags; }

MirTargetInstructionId MirInstruction::getTargetId() const { return m_targetId; }

SourceReference *MirInstruction::getSourceRef() const { return m_sourceRef; }

void MirInstruction::addOperand(const MirOperand *operand)
{
    m_operands.push_back((MirOperand *)operand);

    m_cachedDefinedRegisters = false;
    m_cachedUsedRegisters = false;
}

void MirInstruction::setOpcode(MirInstructionOpCode opcode) { m_opcode = opcode; }

void MirInstruction::setTargetId(MirTargetInstructionId id) { m_targetId = id; }

void MirInstruction::invalidateCachedUsedAndDefs()
{
    m_cachedDefinedRegisters = false;
    m_cachedUsedRegisters = false;
}

const std::pmr::vector<MirOperand *> &MirInstruction::getOperands() const { return m_operands; }

std::pmr::vector<MirOperand *> &MirInstruction::getOperands()
{
    m_cachedDefinedRegisters = false;
    m_cachedUsedRegisters = false;

    m_definedRegisters.clear();
    m_usedRegisters.clear();

    return m_operands;
}

const std::pmr::vector<RegisterRef> &MirInstruction::getDefinedRegisters()
{
    if (!m_cachedDefinedRegisters)
    {
        m_cachedDefinedRegisters = true;
        for (size_t i = 0; i < m_operands.size(); i++)
        {
            MirOperand *operand = m_operands[i];
            MirRegister *reg = operand->get<MirRegister>();

            if (reg)
            {
                OperandFlag flags = getMetadata().m_operandConstraints[i].flags;
                if (flags & OperandFlag::Write)
                {
                    m_definedRegisters.push_back(operand->get<MirRegister>()->getRef());
                }
            }
        }
    }

    return m_definedRegisters;
}

const std::pmr::vector<RegisterRef> &MirInstruction::getUsedRegisters()
{
    if (!m_cachedUsedRegisters)
    {
        m_cachedUsedRegisters = true;
        for (size_t i = 0; i < m_operands.size(); i++)
        {
            MirOperand *operand = m_operands[i];
            MirRegister *reg = operand->get<MirRegister>();
            MirMemory *mem = operand->get<MirMemory>();

            if (reg)
            {
                OperandFlag flags = getMetadata().m_operandConstraints[i].flags;
                if (flags & OperandFlag::Read)
                {
                    m_usedRegisters.push_back(operand->get<MirRegister>()->getRef());
                }
            }
            else if (mem && mem->getBase())
            {
                m_usedRegisters.push_back(mem->getBase()->getRef());
            }
        }
    }

    return m_usedRegisters;
}

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
