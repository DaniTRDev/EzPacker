#include "AstNodes/ImmediateOperand.h"

AstNodeType ImmediateOperand::getType() const { return AstNodeType::Immediate; }

const char *ImmediateOperand::getAstNodeName() const { return "Immediate"; }

void ImmediateOperand::setDataType(const std::string &dataType) { m_dataType = dataType; }

const std::string &ImmediateOperand::getDataType() { return m_dataType; }

IntegerImmediate::IntegerImmediate(std::shared_ptr<mp_int> integer) : m_integer(integer) {}

const char *IntegerImmediate::getImmediateTypeName() const { return "Integer"; }

ImmediateType IntegerImmediate::getImmediateType() const { return ImmediateType::Integer; }

std::shared_ptr<mp_int> IntegerImmediate::copy() const
{
    std::shared_ptr<mp_int> _copy = std::make_shared<mp_int>();
    if (mp_copy(m_integer.get(), _copy.get()) != MP_OKAY)
    {
        return {};
    }

    return _copy;
}

const std::shared_ptr<mp_int> &IntegerImmediate::getInteger() const { return m_integer; }

std::string IntegerImmediate::getAsStr(AstNodeStringMode mode) const
{
    int radix = 16, len = 0;
    size_t written = 0;

    if (mp_radix_size(m_integer.get(), int(radix), &len) != MP_OKAY)
        throw std::runtime_error("Could not get out number buffer size");

    std::string value;
    value.resize(len);

    if (mp_to_radix(m_integer.get(), (char *)value.data(), len, &written, radix) != MP_OKAY)
        throw std::runtime_error("Could not convert big integer to hex number");

    value.pop_back(); // Pops back '\0' from the C-string.
    return std::format("@Immediate(type: {}, value: 0x{})", getImmediateTypeName(), value);
}

FloatImmediate::FloatImmediate(double floatingValue) : m_floatingValue(floatingValue) {}

const char *FloatImmediate::getImmediateTypeName() const { return "FloatingPoint"; }

ImmediateType FloatImmediate::getImmediateType() const { return ImmediateType::FloatingPoint; }

double FloatImmediate::getFloatingValue() const { return m_floatingValue; }

std::string FloatImmediate::getAsStr(AstNodeStringMode mode) const
{
    return std::format("@Immediate(type: {}, value: {})", getImmediateTypeName(), getFloatingValue());
}

StringImmediate::StringImmediate(std::string str) : m_str(std::move(str)) {}

const char *StringImmediate::getImmediateTypeName() const { return "String"; }

ImmediateType StringImmediate::getImmediateType() const { return ImmediateType::String; }

std::string StringImmediate::getAsStr(AstNodeStringMode mode) const
{
    return std::format("@Immediate(type: {}, value: {})", getImmediateTypeName(), getStr());
}

const std::string &StringImmediate::getStr() const { return m_str; }
