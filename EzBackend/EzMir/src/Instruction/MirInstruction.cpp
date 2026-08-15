#include "Instruction/MirInstruction.h"

MirInstruction::MirInstruction(class MirBlock *owner,
                               MirInstructionOpCode opcode,
                               SourceReference *ref,
                               std::pmr::vector<MirOperand *> operands) :
    m_cachedDefinedRegisters(false), m_cachedUsedRegisters(false), m_owner(owner), m_opcode(opcode),
    m_targetDesc(nullptr), m_sourceRef(ref),
    m_definedRegisters(std::pmr::vector<RegisterRef>(operands.get_allocator().resource())),
    m_usedRegisters(std::pmr::vector<RegisterRef>(operands.get_allocator().resource())), m_operands(std::move(operands))
{
}

bool MirInstruction::hasOperands() const { return !m_operands.empty(); }

bool MirInstruction::isSelected() const
{
    return m_opcode == MirInstructionOpCode::TARGET_INST && m_targetDesc != nullptr;
}

bool MirInstruction::isSigned() const { return getMetadata().m_flags & MirInstructionFlags::TreatAsSigned; }

const char *MirInstruction::getOpCodeName() const { return getMetadata().m_name.data(); }

const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

class MirBlock *MirInstruction::getOwner() { return m_owner; }

MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

MirInstructionTier MirInstruction::getTier() const { return getMetadata().m_tier; }

MirInstructionFlags MirInstruction::getFlags() const { return getMeta(getOpCode()).m_flags; }

MirTargetInstructionDesc *MirInstruction::getTargetDesc() const { return m_targetDesc; }

OperandFlag MirInstruction::getOperandFlag(size_t index) const
{
    if (isSelected())
    {
        if (m_targetDesc)
        {
            const auto &flags = m_targetDesc->getOperandsFlags();
            if (index < flags.size())
            {
                return flags[index];
            }
        }
        // Fallback for target instructions without explicit descriptors:
        // By standard convention, operand 0 is destination (Write/ReadWrite) unless it's a store/branch.
        return OperandFlag::Read;
    }

    // High-Level IR instructions
    const auto &flags = getMetadata().m_operandFlags;
    if (index < flags.size())
    {
        return flags[index].flags;
    }

    return OperandFlag::None;
}

SourceReference *MirInstruction::getSourceRef() const { return m_sourceRef; }

void MirInstruction::addOperand(const MirOperand *operand)
{
    m_operands.push_back((MirOperand *)operand);
    invalidateCachedUsedAndDefs();
}

void MirInstruction::invalidateCachedUsedAndDefs()
{
    m_cachedDefinedRegisters = false;
    m_cachedUsedRegisters = false;
}

void MirInstruction::setOpcode(MirInstructionOpCode opcode)
{
    m_opcode = opcode;
    invalidateCachedUsedAndDefs();
}

void MirInstruction::setOperands(const std::pmr::vector<MirOperand *> &operands)
{
    m_operands = operands;
    invalidateCachedUsedAndDefs();
}

void MirInstruction::setTargetDesc(MirTargetInstructionDesc *desc)
{
    m_targetDesc = desc;
    invalidateCachedUsedAndDefs();
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
        m_definedRegisters.clear();

        for (size_t i = 0; i < m_operands.size(); i++)
        {
            MirOperand *operand = m_operands[i];
            if (!operand)
                continue;

            MirRegister *reg = operand->get<MirRegister>();
            if (reg)
            {
                OperandFlag flags = getOperandFlag(i);
                if (flags & OperandFlag::Write)
                {
                    m_definedRegisters.push_back(reg->getRef());
                }
            }
        }

        // Include implicit target registers defined by this instruction (if lowered)
        if (m_targetDesc)
        {
            for (const auto &impDef : m_targetDesc->getImplicitDefs())
            {
                m_definedRegisters.push_back(impDef);
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
        m_usedRegisters.clear();

        for (size_t i = 0; i < m_operands.size(); i++)
        {
            MirOperand *operand = m_operands[i];
            if (!operand)
                continue;

            MirRegister *reg = operand->get<MirRegister>();
            MirMemory *mem = operand->get<MirMemory>();

            if (reg)
            {
                OperandFlag flags = getOperandFlag(i);
                if (flags & OperandFlag::Read)
                {
                    m_usedRegisters.push_back(reg->getRef());
                }
            }
            else if (mem && mem->getBase())
            {
                // Memory base register is always read
                m_usedRegisters.push_back(mem->getBase()->getRef());
            }
        }

        // Include implicit target registers used by this instruction (if lowered)
        if (m_targetDesc)
        {
            for (const auto &impUse : m_targetDesc->getImplicitUses())
            {
                m_usedRegisters.push_back(impUse);
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
