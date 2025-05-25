#include "number/BigInt.h"

BigInt::BigInt() : m_blocks()
{
}

BigNumberType BigInt::getType()
{
    return BigNumberType::Int;
}

bool BigInt::fromStr(const std::string &str)
{
    uint64_t base = 0xFFFFFFFF;
    std::string number = str;

    while (number != "0")
    {
        std::string quotient;
        uint64_t remainder = 0;

        for (char ch : number)
        {
            remainder = remainder * 10 + digitFromStr(ch);
            if (!quotient.empty() || remainder >= base)
            {
                quotient += digitFromStr(remainder / base);
            }
            remainder %= base;
        }

        m_blocks.push_back(static_cast<uint32_t>(remainder));
        number = quotient.empty() ? "0" : quotient;
    }

    return true;
}

char *BigInt::getEncoded()
{
    return (char *)m_blocks.data();
}

size_t BigInt::getBitSize()
{
    // Get last block, know its linear position and get the last bit set.
    // Size = Number of size IN BITS (*8 = << 3) for the N-1 blocks stored in m_blocks.
    // We need to get the MSB and add its position to size. Perform divisions of 2 until we reach 0 -> LOG2
    BlockSizeT block = m_blocks.back();
    size_t size = (m_blocks.size() - 1) * (BlockSize << 3);

    return size + LOG2(block);
}

size_t BigInt::getEncodedSize()
{
    return m_blocks.size() * BlockSize;
}

std::string BigInt::toString()
{
    if (m_blocks.empty())
        return "0";

    uint64_t divisor = 10;
    std::string result;

    while (!m_blocks.empty() && !(m_blocks.size() == 1 && m_blocks[0] == 0))
    {
        uint64_t remainder = 0;

        // Long division of the blocks by 10
        for (int i = int(m_blocks.size()) - 1; i >= 0; --i)
        {
            uint64_t current = (remainder << 32) + m_blocks[i];
            m_blocks[i] = static_cast<uint32_t>(current / divisor);
            remainder = current % divisor;
        }

        // Remove leading zeros from blocks
        while (!m_blocks.empty() && m_blocks.back() == 0)
            m_blocks.pop_back();

        // Prepend remainder digit as char
        result.push_back(digitToStr(remainder));
    }

    // Reverse since digits extracted from least significant to most
    std::reverse(result.begin(), result.end());
    return result.empty() ? "0" : result;
}

uint8_t BigInt::digitFromStr(uint8_t ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0'; // Get the numerical value.

    throw std::runtime_error("Invalid decimal digit");
}

uint8_t BigInt::digitToStr(uint8_t ch)
{
    if (ch >= 0 && ch <= 9)
        return ch + '0'; // Get the numerical value.

    throw std::runtime_error("Invalid decimal digit");
}
