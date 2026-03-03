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

IntegerImmediate::IntegerImmediate(mp_int *integer) : m_integer(integer), m_signed(integer->sign == MP_NEG) {}

bool IntegerImmediate::isSigned() const { return m_signed; }

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

std::string IntegerImmediate::getAsStr(AstNodeStringMode mode) const
{
    int radix = 16, len = 0;
    size_t written = 0;

    if (mp_radix_size(m_integer, int(radix), &len) != MP_OKAY)
        throw std::runtime_error("Could not get out number buffer size");

    std::string value;
    value.resize(len);

    if (mp_to_radix(m_integer, (char *)value.data(), len, &written, radix) != MP_OKAY)
        throw std::runtime_error("Could not convert big integer to hex number");

    value.pop_back(); // Pops back '\0' from the C-string.
    return std::format("@Immediate(type: {}, value: 0x{})", getImmediateTypeName(), value);
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

std::string FloatImmediate::getAsStr(AstNodeStringMode mode) const
{
    return std::format("@Immediate(type: {}, value: {})", getImmediateTypeName(), getFloatingValue());
}

StringImmediate::StringImmediate(std::string_view str) : m_str(std::move(str)) {}

const char *StringImmediate::getImmediateTypeName() const { return "String"; }

ImmediateType StringImmediate::getImmediateType() const { return ImmediateType::String; }

std::string StringImmediate::getAsStr(AstNodeStringMode mode) const
{
    return std::format("@Immediate(type: {}, value: {})", getImmediateTypeName(), getStr());
}

const std::string_view &StringImmediate::getStr() const { return m_str; }
