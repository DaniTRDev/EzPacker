#include "number/BigInt.h"

BigInt::BigInt() : m_blocks()
{
}

bool BigInt::fromStr(const std::string &str)
{
    return false;
}

size_t BigInt::getBitSize()
{
    // Get last block, know its linear position and get the last bit set.
    // Size = number of size in bits for the N-1 blocks stored in m_blocks.
    // We need to get the MSB and add its position to size. Perform divisions of 2 until we reach 0 -> LOG2
    BlockSizeT block = m_blocks.back();
    size_t size = (m_blocks.size() - 1) * (BlockSize << 3);
    
    return size + LOG2(block);
}

std::string BigInt::toString()
{
    return std::string();
}

uint8_t BigInt::digitFromStr(uint8_t ch, size_t base)
{
    if (base <= 10)
    {
        if (ch >= '0' && ch <= '9')
            return ch - '0'; // Get the numerical value.
        
        throw std::runtime_error("Invalid digit");
    }
    
    if (ch >= '0' && ch <= '9')
        return ch - '0'; // Since our base is > 10, we can safely return base10 number.
    
    else if (ch >= 'a' && ch < 'a' + (base - 10))
        return ch - 'a' + 10; // Get the position of the character and add "how much base10 its exceeded".
    
    else if (ch >= 'A' && ch < 'A' + (base - 10))
        return ch - 'A' + 10; // Same as above but with capital letters, allow capital/non-capital conversion.
    
    throw std::runtime_error("Unknown digit format");
}
