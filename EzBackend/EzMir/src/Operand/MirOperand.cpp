#include "Operand/MirOperand.h"

bool MirReference::isBlock() const { return m_type == MirReferenceType::Block; }

bool MirReference::isFunction() const { return m_type == MirReferenceType::Function; }

bool MirReference::isDataEntry() const { return m_type == MirReferenceType::DataEntry; }

bool MirReference::isInvalid() const { return m_type == MirReferenceType::Invalid; }

bool MirRegister::operator==(const MirRegister &other) const
{
    return m_id == other.m_id && m_virtual == other.m_virtual;
}

MirOperand::MirOperand(const MirOperand &copy) : m_data(copy.m_data) {}

MirOperand::MirOperand(const MirOperand &&other) { m_data = std::move(other.m_data); }
MirOperandType MirOperand::getType() const { return static_cast<MirOperandType>(m_data.index()); }

MirBigInteger *MirOperand::getBigInteger() { return std::get_if<MirBigInteger>(&m_data); }
const MirBigInteger *MirOperand::getBigInteger() const { return std::get_if<MirBigInteger>(&m_data); }

MirDouble *MirOperand::getDouble() { return std::get_if<MirDouble>(&m_data); }
const MirDouble *MirOperand::getDouble() const { return std::get_if<MirDouble>(&m_data); }

MirInteger *MirOperand::getInteger() { return std::get_if<MirInteger>(&m_data); }
const MirInteger *MirOperand::getInteger() const { return std::get_if<MirInteger>(&m_data); }

MirReference *MirOperand::getReference() { return std::get_if<MirReference>(&m_data); }
const MirReference *MirOperand::getReference() const { return std::get_if<MirReference>(&m_data); }

MirRegister *MirOperand::getRegister() { return std::get_if<MirRegister>(&m_data); }

const MirRegister *MirOperand::getRegister() const { return std::get_if<MirRegister>(&m_data); }

size_t MirOperand::getSizeInBytes() const
{
    return std::visit(
            [&](auto &&arg) -> size_t
            {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, MirRegister>)
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

std::string MirOperand::toString() const
{
    return std::visit(
            [&](auto &&arg) -> std::string
            {
                using T = std::decay_t<decltype(arg)>;
                std::ostringstream oss;

                if constexpr (std::is_same_v<T, MirRegister>)
                {
                    // Formats as %vreg5 or %reg5 depending on virtuality
                    oss << (arg.m_virtual ? "%vreg" : "%reg") << arg.m_id;
                }
                else if constexpr (std::is_same_v<T, MirInteger>)
                {
                    // ASSUMPTION: MirInteger has a member holding the value (e.g., m_value)
                    oss << arg.m_value;
                }
                else if constexpr (std::is_same_v<T, MirDouble>)
                {
                    // ASSUMPTION: MirDouble has a member holding the value (e.g., m_value)
                    oss << arg.m_value;
                }
                else if constexpr (std::is_same_v<T, MirReference>)
                {
                    // Utilize the methods you provided to format the base type
                    if (arg.isBlock())
                        oss << "block_ref";
                    else if (arg.isFunction())
                        oss << "func_ref";
                    else if (arg.isDataEntry())
                        oss << "data_ref";
                    else
                        oss << "invalid_ref";

                    // ASSUMPTION: If you have a target ID or Name, append it here.
                    // e.g., oss << "_" << arg.m_id; or oss << "(@" << arg.m_name << ")";
                }
                else if constexpr (std::is_same_v<T, MirBigInteger>)
                {
                    // ASSUMPTION: Either MirBigInteger has its own toString()
                    // or you can format its internal array here.
                    oss << "BigInt(...)";
                }
                else
                {
                    oss << "UnknownOperand";
                }

                return oss.str();
            },
            getVariant());
}

void MirOperand::swapData(MirOperand::VariantType other) { m_data = other; }
