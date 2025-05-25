#include "Number/Float.h"

BigNumberType Float::getType()
{
    return BigNumberType::Float;
}

bool Float::fromStr(const std::string &str)
{
    float result = 0.f;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), result);

    if (ec != std::errc())
        return false;

    m_value = result;
    return true;
}

char *Float::getEncoded()
{
    return (char *)&m_value;
}

size_t Float::getBitSize()
{
    return sizeof(m_value) * 8;
}

size_t Float::getEncodedSize()
{
    return sizeof(m_value);
}

std::string Float::toString()
{
    return std::to_string(m_value);
}
