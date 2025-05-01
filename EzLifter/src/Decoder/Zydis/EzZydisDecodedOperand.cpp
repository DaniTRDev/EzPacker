#include "Decoder/Zydis/EzZydisDecodedOperand.h"

EzZydisDecodedOperand::EzZydisDecodedOperand(std::shared_ptr<ZydisDecodedOperand> zydisOperand)
    : m_zydisOperand(zydisOperand)
{
    assert(m_zydisOperand != nullptr);
}

EzZydisDecodedOperand::~EzZydisDecodedOperand()
{
    m_zydisOperand.reset();
}

bool EzZydisDecodedOperand::hasDisplacement() const
{
    return m_zydisOperand->mem.disp.has_displacement;
}

bool EzZydisDecodedOperand::isDestinationOperand() const
{
    return m_zydisOperand->actions & static_cast<ZyanU8>(ZYDIS_OPERAND_ACTION_WRITE);
}

bool EzZydisDecodedOperand::isImmediate() const
{
    return m_zydisOperand->type == ZYDIS_OPERAND_TYPE_IMMEDIATE;
}

bool EzZydisDecodedOperand::isMemory() const
{
    return m_zydisOperand->type == ZYDIS_OPERAND_TYPE_MEMORY;
}

bool EzZydisDecodedOperand::isRead() const
{
    return (m_zydisOperand->actions & static_cast<ZyanU8>(ZYDIS_OPERAND_ACTION_READ)) ||
           (m_zydisOperand->actions & static_cast<ZyanU8>(ZYDIS_OPERAND_ACTION_CONDREAD));
}

bool EzZydisDecodedOperand::isRegister() const
{
    return m_zydisOperand->type == ZYDIS_OPERAND_TYPE_REGISTER;
}

bool EzZydisDecodedOperand::isRelative() const
{
    if (!isMemory())
        return false;

    return m_zydisOperand->mem.base == ZYDIS_REGISTER_RIP;
}

bool EzZydisDecodedOperand::isSigned() const
{
    if (!isImmediate())
        return false;
    
    return m_zydisOperand->imm.is_signed;
}

bool EzZydisDecodedOperand::isSourceOperand() const
{
    return m_zydisOperand->actions & static_cast<ZyanU8>(ZYDIS_OPERAND_ACTION_READ);
}

bool EzZydisDecodedOperand::isStackRegister() const
{
    return isRegister() && (getRegister() == ZYDIS_REGISTER_RSP || getRegister() == ZYDIS_REGISTER_ESP);
}

bool EzZydisDecodedOperand::isWritten() const
{
    return (m_zydisOperand->actions & static_cast<ZyanU8>(ZYDIS_OPERAND_ACTION_WRITE)) ||
           (m_zydisOperand->actions & static_cast<ZyanU8>(ZYDIS_OPERAND_ACTION_CONDWRITE));
}

int16_t EzZydisDecodedOperand::getRegister() const
{
    if (!isRegister())
        return -1;
    
    return static_cast<int16_t>(m_zydisOperand->reg.value);
}

int16_t EzZydisDecodedOperand::getBaseRegisterId() const
{
    if (!isMemory())
        return -1;

    return static_cast<int16_t>(m_zydisOperand->mem.base);
}

int16_t EzZydisDecodedOperand::getIndexRegisterId() const
{
    if (!isMemory())
        return -1;

    return static_cast<int16_t>(m_zydisOperand->mem.index);
}

int64_t EzZydisDecodedOperand::getDisplacement() const
{
    if (!isMemory() || !hasDisplacement())
        return 0;

    return m_zydisOperand->mem.disp.value;
}

uint64_t EzZydisDecodedOperand::getImmediateS() const
{
    return m_zydisOperand->imm.value.s;
}

uint64_t EzZydisDecodedOperand::getImmediateU() const
{
    return m_zydisOperand->imm.value.u;
}

uint8_t EzZydisDecodedOperand::getScale() const
{
    if (!isMemory())
        return 0;

    return m_zydisOperand->mem.scale;
}

std::shared_ptr<ZydisDecodedOperand> EzZydisDecodedOperand::getZydisDecodedOperand()
{
    return m_zydisOperand;
}