#include "Decoder/Zydis/ZydisDecodedInstructionParser.h"

ZydisDecodedInstructionParser::ZydisDecodedInstructionParser(std::shared_ptr<ZydisDecodedInstruction> zydisInstruction)
    : m_zydisInstruction(zydisInstruction)
{
}

ZydisDecodedInstructionParser::~ZydisDecodedInstructionParser()
{
    m_zydisInstruction.reset();
}

bool ZydisDecodedInstructionParser::isBranch() const
{
    return m_zydisInstruction->meta.category == ZYDIS_CATEGORY_COND_BR;
}

bool ZydisDecodedInstructionParser::isCall() const
{
    return m_zydisInstruction->meta.category == ZYDIS_CATEGORY_CALL;
}

bool ZydisDecodedInstructionParser::isJump() const
{
    return m_zydisInstruction->meta.category == ZYDIS_CATEGORY_UNCOND_BR;
}

bool ZydisDecodedInstructionParser::isLoad(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const
{
    for (const std::shared_ptr<IDecodedOperand> &operand : operands)
    {
        if (operand->isMemory() && operand->isRead())
            return true;
    }

    return false;
}

bool ZydisDecodedInstructionParser::isMovRegister(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const
{
    if (getType() != DecodedInstructionType::_Mov)
        return false; // If it's not of the "special type" return right away.

    for (const std::shared_ptr<IDecodedOperand> &operand : operands)
    {
        if (operand->isMemory() || operand->isImmediate())
            return false; // If any of the operands is a memory reference or immediate, it's not of this type.
    }

    return true;
}

bool ZydisDecodedInstructionParser::isMovRegisterImm(
    const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const
{
    if (getType() != DecodedInstructionType::_Mov)
        return false; // If it's not of the "special type" return right away.

    for (const std::shared_ptr<IDecodedOperand> &operand : operands)
    {
        if (operand->isMemory())
            return false; // If any of the operands is a memory reference it's not of this type.
    }

    return true;
}

bool ZydisDecodedInstructionParser::isRet() const
{
    return m_zydisInstruction->meta.category == ZYDIS_CATEGORY_RET;
}

bool ZydisDecodedInstructionParser::isStore(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const
{
    for (const std::shared_ptr<IDecodedOperand> &operand : operands)
    {
        if (operand->isMemory() && operand->isWritten())
            return true;
    }

    return false;
}

bool ZydisDecodedInstructionParser::usesMemory(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const
{
    for (const std::shared_ptr<IDecodedOperand> &operand : operands)
    {
        if (operand->isMemory())
            return true;
    }

    return false;
}

ConditionType ZydisDecodedInstructionParser::getConditionType() const
{
    return translator::translateZydisCondition(m_zydisInstruction);
}

DecodedInstructionType ZydisDecodedInstructionParser::getType() const
{
    return translator::translateZydisMnemonic(m_zydisInstruction);
}

MemoryReferenceType ZydisDecodedInstructionParser::getMemoryRefType(
    const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const
{
    for (const std::shared_ptr<IDecodedOperand> &operand : operands)
    {
        if (!operand->isMemory())
            continue;

        if (operand->isRelative())
            return MemoryReferenceType::IPRelative; // [IP+Displacement]

        if (operand->getBaseRegisterId() == -1)
        {
            if (operand->getIndexRegisterId() == -1)
                return MemoryReferenceType::Direct; // [Absolute Address]
            else
                return MemoryReferenceType::IndexScaleDisplacement; // [Index * Scale + Displacement]
        }
        else
        {
            if (operand->getIndexRegisterId() == -1)
            {
                if (operand->getDisplacement() == 0)
                    return MemoryReferenceType::Base; // [Base]
                else
                    return MemoryReferenceType::BaseDisplacement; // [Base + Displacement]
            }
            else
            {
                return MemoryReferenceType::BaseIndexScaleDisplacement; // [Base + Index * Scale + Displacement]
            }
        }
    }

    return MemoryReferenceType::Invalid;
}

uint8_t ZydisDecodedInstructionParser::getReadFlags() const
{
    uint8_t read = static_cast<uint8_t>(FlagType::None);
    ZydisAccessedFlagsMask flags = m_zydisInstruction->cpu_flags->tested;

    if (flags & ZYDIS_CPUFLAG_AF)
    {
        read |= static_cast<uint8_t>(FlagType::Auxiliary);
    }

    if (flags & ZYDIS_CPUFLAG_CF)
    {
        read |= static_cast<uint8_t>(FlagType::Carry);
    }

    if (flags & ZYDIS_CPUFLAG_NT)
    {
        read |= static_cast<uint8_t>(FlagType::Negative);
    }

    if (flags & ZYDIS_CPUFLAG_OF)
    {
        read |= static_cast<uint8_t>(FlagType::Overflow);
    }

    if (flags & ZYDIS_CPUFLAG_PF)
    {
        read |= static_cast<uint8_t>(FlagType::Parity);
    }

    if (flags & ZYDIS_CPUFLAG_SF)
    {
        read |= static_cast<uint8_t>(FlagType::Sign);
    }

    if (flags & ZYDIS_CPUFLAG_ZF)
    {
        read |= static_cast<uint8_t>(FlagType::Zero);
    }

    return read;
}

uint8_t ZydisDecodedInstructionParser::getWrittenFlags() const
{
    uint8_t written = static_cast<uint8_t>(FlagType::None);
    ZydisAccessedFlagsMask flags = m_zydisInstruction->cpu_flags->modified;

    if (flags & ZYDIS_CPUFLAG_AF)
    {
        written |= static_cast<uint8_t>(FlagType::Auxiliary);
    }

    if (flags & ZYDIS_CPUFLAG_CF)
    {
        written |= static_cast<uint8_t>(FlagType::Carry);
    }

    if (flags & ZYDIS_CPUFLAG_NT)
    {
        written |= static_cast<uint8_t>(FlagType::Negative);
    }

    if (flags & ZYDIS_CPUFLAG_OF)
    {
        written |= static_cast<uint8_t>(FlagType::Overflow);
    }

    if (flags & ZYDIS_CPUFLAG_PF)
    {
        written |= static_cast<uint8_t>(FlagType::Parity);
    }

    if (flags & ZYDIS_CPUFLAG_SF)
    {
        written |= static_cast<uint8_t>(FlagType::Sign);
    }

    if (flags & ZYDIS_CPUFLAG_ZF)
    {
        written |= static_cast<uint8_t>(FlagType::Zero);
    }

    return written;
}

std::shared_ptr<ZydisDecodedInstruction> ZydisDecodedInstructionParser::getZydisDecodedInstruction()
{
    return m_zydisInstruction;
}
