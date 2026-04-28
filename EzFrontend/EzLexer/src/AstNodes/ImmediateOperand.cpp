#include "AstNodes/ImmediateOperand.h"

AstNodeType ImmediateOperand::getType() const { return AstNodeType::Immediate; }

bool ImmediateOperand::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *ImmediateOperand::getAstNodeName() const { return "Immediate"; }

void ImmediateOperand::setDataType(const std::string_view &dataType) { m_dataType = dataType; }

const std::string_view &ImmediateOperand::getDataType() { return m_dataType; }

IntegerImmediate::IntegerImmediate(mp_int *integer) : m_integer(integer) {}

const char *IntegerImmediate::getImmediateTypeName() const { return "Integer"; }

mp_int *IntegerImmediate::getInteger() { return m_integer; }

ImmediateType IntegerImmediate::getImmediateType() const { return ImmediateType::Integer; }

void IntegerImmediate::copy(mp_int *destination)
{
    if (mp_copy(getInteger(), destination) != MP_OKAY)
    {
        throw std::runtime_error("Could not copy integer number");
    }
}

std::string IntegerImmediate::getAsBin() const
{
    std::string str;
    size_t written = 0;
    size_t size = mp_sbin_size(m_integer);

    str.reserve(size);

    if (mp_to_ubin(m_integer, (unsigned char *)str.data(), size, &written) != MP_OKAY)
    {
        return "";
    }

    std::reverse(str.begin(), str.end()); // Converts BigEndian to LittleEndian.
    if (m_integer->sign)
    {
        // Calculate Two's Complement
        uint8_t carry = 1;
        for (size_t i = 0; i < size; i++)
        {
            str[i] = ~char(str[i]);        // Invert all bits
            uint16_t sum = str[i] + carry; // Add 1
            str[i] = static_cast<uint8_t>(sum & 0xFF);
            carry = static_cast<uint8_t>(sum >> 8);
        }
    }

    if (written == 0)
    {
        return "";
    }

    return str;
}

FloatImmediate::FloatImmediate(double floatingValue) : m_floatingValue(floatingValue) {}

const char *FloatImmediate::getImmediateTypeName() const { return "FloatingPoint"; }

ImmediateType FloatImmediate::getImmediateType() const { return ImmediateType::FloatingPoint; }

double FloatImmediate::getFloatingValue() const { return m_floatingValue; }

StringImmediate::StringImmediate(std::string_view str) : m_str(std::move(str)) {}

const char *StringImmediate::getImmediateTypeName() const { return "String"; }

ImmediateType StringImmediate::getImmediateType() const { return ImmediateType::String; }

const std::string_view &StringImmediate::getStr() const { return m_str; }
