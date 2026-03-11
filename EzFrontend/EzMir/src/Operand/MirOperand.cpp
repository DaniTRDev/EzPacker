#include "Operand/MirOperand.h"

MirOperand::MirOperand(const MirOperand &copy) : m_data(copy.m_data) {}

MirOperand::MirOperand(const MirOperand &&other) { m_data = std::move(other.m_data); }

MirOperandType MirOperand::getType() const { return static_cast<MirOperandType>(m_data.index()); }

MirBigInteger *MirOperand::getBigInteger() { return std::get_if<MirBigInteger>(&m_data); }
const MirBigInteger *MirOperand::getBigInteger() const { return std::get_if<MirBigInteger>(&m_data); }

MirDouble *MirOperand::getDouble() { return std::get_if<MirDouble>(&m_data); }
const MirDouble *MirOperand::getDouble() const { return std::get_if<MirDouble>(&m_data); }

MirInteger *MirOperand::getInteger() { return std::get_if<MirInteger>(&m_data); }
const MirInteger *MirOperand::getInteger() const { return std::get_if<MirInteger>(&m_data); }

MirMemory *MirOperand::getMemory() { return std::get_if<MirMemory>(&m_data); }
const MirMemory *MirOperand::getMemory() const { return std::get_if<MirMemory>(&m_data); }

MirReference *MirOperand::getReference() { return std::get_if<MirReference>(&m_data); }
const MirReference *MirOperand::getReference() const { return std::get_if<MirReference>(&m_data); }

MirRegister *MirOperand::getRegister() { return std::get_if<MirRegister>(&m_data); }
const MirRegister *MirOperand::getRegister() const { return std::get_if<MirRegister>(&m_data); }

size_t MirOperand::getSize() const
{
    return std::visit(
            [&](auto &&arg) -> size_t
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, MirRegister>)
                {
                    return arg.m_size;
                }
                else if constexpr (std::is_same_v<T, MirMemory>)
                {
                    return arg.m_size;
                }
                else if constexpr (std::is_same_v<T, MirInteger>)
                {
                    return arg.m_size;
                }
                else if constexpr (std::is_same_v<T, MirDouble>)
                {
                    return 8; // IEEE 754 double is strictly 64-bit
                }
                else if constexpr (std::is_same_v<T, MirReference>)
                {
                    return 0; // We don't know the size yet, this is a task for the backend.
                }
                else if constexpr (std::is_same_v<T, MirBigInteger>)
                {
                    return arg.m_size;
                }
                else
                {
                    return 0; // Fallback for unknown types
                }
            },
            getVariant());
}

MirOperand::VariantType &MirOperand::getVariant() { return m_data; }

const MirOperand::VariantType &MirOperand::getVariant() const { return m_data; }
