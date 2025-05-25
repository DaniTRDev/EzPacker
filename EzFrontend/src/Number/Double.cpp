#include "Number/Double.h"

BigNumberType Double::getType()
{
    return BigNumberType::Double;
}

bool Double::fromStr(const std::string &str)
{
    double result = 0.f;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);

    if (ec != std::errc())
        return false;

    m_value = result;
    return true;
}

char *Double::getEncoded()
{
    return (char *)&m_value;
}

size_t Double::getBitSize()
{
    return sizeof(m_value) * 8;
}

size_t Double::getEncodedSize()
{
    return sizeof(m_value);
}

std::string Double::toString()
{
    return std::to_string(m_value);
}
