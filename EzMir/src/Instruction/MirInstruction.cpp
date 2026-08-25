#include "Instruction/MirInstruction.h"
#include "Block/MirBlock.h"
#include "Instruction/MirInstructionSet.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperands.h"

MirInstruction::MirInstruction(MirBlock *owner,
                               MirInstructionOpCode opcode,
                               SourceReference *ref,
                               std::pmr::vector<MirOperand *> operands) :
    m_owner(owner), m_opcode(opcode), m_targetDesc(nullptr), m_sourceRef(ref), m_operands(std::move(operands))
{
}

bool MirInstruction::hasOpcode(MirInstructionOpCode opcode) const { return m_opcode == opcode; }

bool MirInstruction::hasOperands() const { return !m_operands.empty(); }

bool MirInstruction::isSelected() const
{
    return m_opcode == MirInstructionOpCode::TARGET_INST && m_targetDesc != nullptr;
}

bool MirInstruction::isSigned() const { return getMetadata().m_flags & MirInstructionFlags::TreatAsSigned; }

const char *MirInstruction::getOpCodeName() const { return getMetadata().m_name.data(); }

const MirInstructionMetadata &MirInstruction::getMetadata() const { return getMeta(getOpCode()); }

MirBlock *MirInstruction::getOwner() const { return m_owner; }

MirInstruction *MirInstruction::getPrev() const { return m_prev; }

MirInstruction *MirInstruction::getNext() const { return m_next; }

MirInstructionOpCode MirInstruction::getOpCode() const { return m_opcode; }

MirInstructionTier MirInstruction::getTier() const { return getMetadata().m_tier; }

MirInstructionFlags MirInstruction::getFlags() const { return getMetadata().m_flags; }

MirTargetInstructionDesc *MirInstruction::getTargetDesc() const { return m_targetDesc; }

MirOperand *MirInstruction::getOperand(size_t index) const
{
    if (index >= m_operands.size())
    {
        return nullptr;
    }
    return m_operands[index];
}

const MirOperand *MirInstruction::getConstOperand(size_t index) const
{
    if (index >= m_operands.size())
    {
        return nullptr;
    }
    return m_operands[index];
}

MirOperandFlag MirInstruction::getOperandFlag(size_t index) const
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
        return MirOperandFlag::Read;
    }

    const auto &opMeta = getMetadata().m_operandMeta;
    const size_t metaCount = opMeta.size();
    const size_t totalOperands = m_operands.size();

    if (metaCount == 0 || index >= totalOperands)
    {
        return MirOperandFlag::None;
    }

    // 1. Locate the variadic expansion slot if one exists
    size_t varSlot = size_t(-1);
    for (size_t i = 0; i < metaCount; ++i)
    {
        if (opMeta[i].type & ExpectedOperandType::VariadicArgs)
        {
            varSlot = i;
            break;
        }
    }

    // 2. Elastic variadic slot resolution
    if (varSlot != size_t(-1))
    {
        size_t trailingFixedCount = metaCount - 1 - varSlot;

        // Malformed operand count safety fallback
        if (totalOperands < metaCount - 1)
        {
            return (index < metaCount) ? opMeta[index].flags : MirOperandFlag::None;
        }

        // Leading fixed operands before the variadic slice
        if (index < varSlot)
        {
            return opMeta[index].flags;
        }
        // Trailing fixed operands after the variadic slice
        else if (index >= totalOperands - trailingFixedCount)
        {
            size_t offsetFromEnd = totalOperands - index;
            return opMeta[metaCount - offsetFromEnd].flags;
        }
        // In the variadic expansion range (inherits Read/Write/ReadWrite from slot descriptor)
        else
        {
            return opMeta[varSlot].flags;
        }
    }

    // 3. Standard fixed-length metadata descriptor
    if (index < metaCount)
    {
        return opMeta[index].flags;
    }
    else if (getFlags() & MirInstructionFlags::VariadicArgs)
    {
        // Fallback.
        return MirOperandFlag::Read;
    }

    return MirOperandFlag::None;
}

size_t MirInstruction::getOperandCount() const { return m_operands.size(); }

SourceReference *MirInstruction::getSourceRef() const { return m_sourceRef; }

void MirInstruction::addOperand(MirOperand *operand) { m_operands.push_back(operand); }

void MirInstruction::setOpcode(MirInstructionOpCode opcode) { m_opcode = opcode; }

void MirInstruction::setOperands(const std::pmr::vector<MirOperand *> &operands) { m_operands = operands; }

void MirInstruction::setTargetDesc(MirTargetInstructionDesc *desc) { m_targetDesc = desc; }

void MirInstruction::setPrev(MirInstruction *prev) { m_prev = prev; }

void MirInstruction::setNext(MirInstruction *next) { m_next = next; }

const std::pmr::vector<MirOperand *> &MirInstruction::getOperands() const { return m_operands; }

std::pmr::vector<MirOperand *> &MirInstruction::getOperands() { return m_operands; }

std::vector<MirRegisterRef> MirInstruction::getDefinedRegisters() const
{
    std::vector<MirRegisterRef> defs;
    defs.reserve(2);

    for (size_t i = 0; i < m_operands.size(); ++i)
    {
        MirOperand *operand = m_operands[i];
        if (!operand)
            continue;

        if (auto *reg = operand->get<MirRegister>())
        {
            if (getOperandFlag(i) & MirOperandFlag::Write)
            {
                defs.push_back(reg->getRef());
            }
        }
    }

    if (m_targetDesc)
    {
        for (const auto &impDef : m_targetDesc->getImplicitDefs())
        {
            defs.push_back(impDef);
        }
    }

    return defs;
}

std::vector<MirRegisterRef> MirInstruction::getUsedRegisters() const
{
    std::vector<MirRegisterRef> uses;
    uses.reserve(4);

    for (size_t i = 0; i < m_operands.size(); ++i)
    {
        MirOperand *operand = m_operands[i];
        if (!operand)
            continue;

        if (auto *reg = operand->get<MirRegister>())
        {
            if (getOperandFlag(i) & MirOperandFlag::Read)
            {
                uses.push_back(reg->getRef());
            }
        }
        else if (auto *mem = operand->get<MirMemory>())
        {
            if (mem->getBase())
            {
                uses.push_back(mem->getBase()->getRef());
            }
        }
    }

    if (m_targetDesc)
    {
        for (const auto &impUse : m_targetDesc->getImplicitUses())
        {
            uses.push_back(impUse);
        }
    }

    return uses;
}

std::string MirInstruction::toString() const
{
    std::string res;
    res += getMetadata().m_name;

    for (auto *operand : m_operands)
    {
        res += " " + (operand ? operand->toString() : "null") + ",";
    }
    return res;
}