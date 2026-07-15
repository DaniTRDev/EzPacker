#include "FlexNumber/FlexInt.h"

FlexInt::FlexInt(const FlexInt &other)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = other.m_isSigned;
    m_bitWidth = other.m_bitWidth;

    if (m_lastErr = mp_copy(&other.m_number, &m_number); m_lastErr != MP_OKAY)
        throw std::runtime_error("Could not create a copy of FlexInt");
}

FlexInt::FlexInt(uint32_t value, size_t bitWidth)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = false;
    m_bitWidth = bitWidth;

    mp_set_ul(&m_number, value);
    clampToTwosComplement();
}

FlexInt::FlexInt(uint64_t value, size_t bitWidth)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = false;
    m_bitWidth = bitWidth;

    mp_set_u64(&m_number, value);
    clampToTwosComplement();
}

FlexInt::FlexInt(int32_t value, size_t bitWidth)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = true;
    m_bitWidth = bitWidth;

    mp_set_l(&m_number, value);
    clampToTwosComplement();
}

FlexInt::FlexInt(int64_t value, size_t bitWidth)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = true;
    m_bitWidth = bitWidth;

    mp_set_i64(&m_number, value);
    clampToTwosComplement();
}

FlexInt::FlexInt(const std::string_view &numberStr, size_t bitWidth, bool _signed, size_t radix)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    if (numberStr.empty() || radix < 2 || radix > 64)
    {
        throw std::runtime_error("Could not decode number because it is invalid or radix is out of bounds");
    }

    size_t scanIdx = numberStr.find_first_not_of(" \t");
    if (scanIdx == std::string_view::npos)
    {
        throw std::runtime_error("Empty or whitespace-only string passed to FlexInt");
    }

    m_bitWidth = bitWidth;
    m_isSigned = _signed;

    // Build a safe, stripped working copy of the numeric string
    std::string safeStr(numberStr.substr(scanIdx));

    // Strip out common C-style hexadecimal prefix tags before submitting to LibTomMath
    if (radix == 16 && safeStr.size() > 2 && safeStr[0] == '0' && (safeStr[1] == 'x' || safeStr[1] == 'X'))
    {
        safeStr = safeStr.substr(2);
    }

    if (m_lastErr = mp_read_radix(&m_number, safeStr.c_str(), int(radix)); m_lastErr != MP_OKAY)
    {
        throw std::runtime_error("Error while decoding number from string: LibTomMath error " +
                                 std::to_string(m_lastErr));
    }

    clampToTwosComplement();
}

FlexInt::~FlexInt() { mp_clear(&m_number); }

bool FlexInt::hasError() const { return m_lastErr != MP_OKAY; }

bool FlexInt::isEven() const { return mp_iseven(&m_number) == MP_YES; }

bool FlexInt::isNeg() const { return m_isSigned && mp_isneg(&m_number) == MP_YES; }

bool FlexInt::isOdd() const { return mp_isodd(&m_number) == MP_YES; }

bool FlexInt::isPositive() const { return !isNeg() && !isZero(); }

bool FlexInt::isZero() const { return mp_iszero(&m_number) == MP_YES; }

bool FlexInt::operator>(const FlexInt &other) const
{
    if (m_bitWidth != other.m_bitWidth || m_isSigned != other.m_isSigned)
    {
        throw std::runtime_error("Mismatched target types in FlexInt comparison");
    }
    return mp_cmp(&m_number, &other.m_number) == MP_GT;
}
bool FlexInt::operator>=(const FlexInt &other) const
{
    if (m_bitWidth != other.m_bitWidth || m_isSigned != other.m_isSigned)
    {
        throw std::runtime_error("Mismatched target types in FlexInt comparison");
    }
    auto res = mp_cmp(&m_number, &other.m_number);
    return res == MP_GT || res == MP_EQ;
}

bool FlexInt::operator<(const FlexInt &other) const
{
    if (m_bitWidth != other.m_bitWidth || m_isSigned != other.m_isSigned)
    {
        throw std::runtime_error("Mismatched target types in FlexInt comparison");
    }
    return mp_cmp(&m_number, &other.m_number) == MP_LT;
}
bool FlexInt::operator<=(const FlexInt &other) const
{
    if (m_bitWidth != other.m_bitWidth || m_isSigned != other.m_isSigned)
    {
        throw std::runtime_error("Mismatched target types in FlexInt comparison");
    }

    auto res = mp_cmp(&m_number, &other.m_number);
    return res == MP_LT || res == MP_EQ;
}

bool FlexInt::operator==(const FlexInt &other) const
{
    if (m_bitWidth == other.m_bitWidth && m_isSigned == other.m_isSigned)
    {
        return mp_cmp(&m_number, &other.m_number) == MP_EQ;
    }

    bool thisNeg = this->isNeg();
    bool otherNeg = other.isNeg();

    if (thisNeg != otherNeg)
    {
        return false;
    }

    if (!thisNeg && !otherNeg)
    {
        return mp_cmp(&m_number, &other.m_number) == MP_EQ;
    }

    FlexInt cloneThis(*this);
    FlexInt cloneOther(other);

    size_t targetWidth = std::max(m_bitWidth, other.m_bitWidth);

    cloneThis.extend(targetWidth, true);
    cloneOther.extend(targetWidth, true);

    return mp_cmp(&cloneThis.m_number, &cloneOther.m_number) == MP_EQ;
}
bool FlexInt::operator!=(const FlexInt &other) const { return !(*this == other); }

FlexInt FlexInt::getHighHalf()
{
    if (m_bitWidth % 2 != 0)
    {
        throw std::runtime_error("Cannot execute scalar split on an odd bit-width.");
    }

    size_t splitWidth = m_bitWidth / 2;
    FlexInt highPart(uint64_t(0), splitWidth);
    highPart.m_isSigned = m_isSigned;

    if (mp_isneg(&m_number) == MP_YES)
    {
        mp_int fullRange, tempNum;
        if (mp_init_multi(&fullRange, &tempNum, nullptr) == MP_OKAY)
        {
            m_lastErr = mp_2expt(&fullRange, static_cast<int>(m_bitWidth));
            m_lastErr = mp_mod(&m_number, &fullRange, &tempNum);
            m_lastErr = mp_add(&tempNum, &fullRange, &tempNum);

            m_lastErr = mp_div_2d(&tempNum, static_cast<int>(splitWidth), &highPart.m_number, nullptr);
            mp_clear_multi(&fullRange, &tempNum, nullptr);
        }
    }
    else
    {
        m_lastErr = mp_div_2d(&m_number, static_cast<int>(splitWidth), &highPart.m_number, nullptr);
    }

    if (highPart.m_isSigned)
    {
        mp_int signBit, halfRange;
        if (mp_init_multi(&signBit, &halfRange, nullptr) == MP_OKAY)
        {
            m_lastErr = mp_2expt(&signBit, static_cast<int>(splitWidth - 1));
            if (mp_cmp(&highPart.m_number, &signBit) != MP_LT)
            {
                m_lastErr = mp_2expt(&halfRange, static_cast<int>(splitWidth));
                m_lastErr = mp_sub(&highPart.m_number, &halfRange, &highPart.m_number);
            }
            mp_clear_multi(&signBit, &halfRange, nullptr);
        }
    }

    return highPart;
}

FlexInt FlexInt::getLowHalf()
{
    if (m_bitWidth % 2 != 0)
    {
        throw std::runtime_error("Cannot execute scalar expansion split on an odd bit-width.");
    }

    size_t splitWidth = m_bitWidth / 2;
    FlexInt lowPart(uint64_t(0), splitWidth);
    lowPart.m_isSigned = false;

    mp_int mask;
    if (mp_init(&mask) != MP_OKAY)
    {
        throw std::bad_alloc();
    }

    m_lastErr = mp_2expt(&mask, static_cast<int>(splitWidth));
    m_lastErr = mp_decr(&mask);

    if (mp_isneg(&m_number) == MP_YES)
    {
        mp_int fullRange, tempNum;
        if (mp_init_multi(&fullRange, &tempNum, nullptr) == MP_OKAY)
        {
            m_lastErr = mp_2expt(&fullRange, static_cast<int>(m_bitWidth));
            m_lastErr = mp_mod(&m_number, &fullRange, &tempNum);
            m_lastErr = mp_add(&tempNum, &fullRange, &tempNum);
            m_lastErr = mp_and(&tempNum, &mask, &lowPart.m_number);
            mp_clear_multi(&fullRange, &tempNum, nullptr);
        }
    }
    else
    {
        m_lastErr = mp_and(&m_number, &mask, &lowPart.m_number);
    }

    mp_clear(&mask);
    return lowPart;
}

FlexInt FlexInt::operator+(const FlexInt &other)
{
    FlexInt res(*this);
    res += other;
    return res;
}
FlexInt &FlexInt::operator++()
{
    m_lastErr = mp_incr(&m_number);
    clampToTwosComplement();
    return *this;
}
FlexInt &FlexInt::operator+=(const FlexInt &other)
{
    m_lastErr = mp_add(&m_number, &other.m_number, &m_number);
    clampToTwosComplement();
    return *this;
}

FlexInt FlexInt::operator-(const FlexInt &other)
{
    FlexInt res(*this);
    res -= other;
    return res;
}
FlexInt &FlexInt::operator--()
{
    m_lastErr = mp_decr(&m_number);
    clampToTwosComplement();
    return *this;
}
FlexInt &FlexInt::operator-=(const FlexInt &other)
{
    m_lastErr = mp_sub(&m_number, &other.m_number, &m_number);
    clampToTwosComplement();
    return *this;
}

FlexInt FlexInt::operator*(const FlexInt &other)
{
    FlexInt res(*this);
    res *= other;
    return res;
}
FlexInt &FlexInt::operator*=(const FlexInt &other)
{
    m_lastErr = mp_mul(&m_number, &other.m_number, &m_number);
    clampToTwosComplement();
    return *this;
}

FlexInt FlexInt::operator/(const FlexInt &other)
{
    FlexInt res(*this);
    res /= other;
    return res;
}
FlexInt &FlexInt::operator/=(const FlexInt &other)
{
    m_lastErr = mp_div(&m_number, &other.m_number, &m_number, nullptr);
    clampToTwosComplement();
    return *this;
}

FlexInt FlexInt::operator%(const FlexInt &other)
{
    FlexInt res(*this);
    res %= other;
    return res;
}
FlexInt &FlexInt::operator%=(const FlexInt &other)
{
    m_lastErr = mp_mod(&m_number, &other.m_number, &m_number);
    clampToTwosComplement();
    return *this;
}

size_t FlexInt::getBitSize() const { return m_bitWidth; }

void FlexInt::extend(size_t newBitSize, bool isSigned)
{
    if (newBitSize < m_bitWidth)
    {
        throw std::bad_alloc();
    }

    if (newBitSize == m_bitWidth)
    {
        m_isSigned = isSigned;
        clampToTwosComplement();
        return;
    }

    bool wasNegative = (mp_isneg(&m_number) == MP_YES);
    size_t oldWidth = m_bitWidth;

    m_bitWidth = newBitSize;
    m_isSigned = isSigned;

    if (wasNegative)
    {
        if (isSigned)
        {
            clampToTwosComplement();
        }
        else
        {
            mp_int fullRange;
            if (m_lastErr = mp_init(&fullRange); m_lastErr == MP_OKAY)
            {
                m_lastErr = mp_2expt(&fullRange, static_cast<int>(oldWidth));
                m_lastErr = mp_add(&m_number, &fullRange, &m_number);
                mp_clear(&fullRange);
            }
            clampToTwosComplement();
        }
    }
    else
    {
        clampToTwosComplement();
    }
}

std::pmr::vector<uint8_t> FlexInt::dump(bool bigEndian, std::pmr::memory_resource *alloc)
{
    size_t byteSize = (m_bitWidth + 7) / 8;
    std::pmr::vector<uint8_t> buffer(byteSize, 0, alloc);

    if (byteSize == 0)
        return buffer;

    mp_int targetBits;
    if (mp_init(&targetBits) != MP_OKAY)
    {
        throw std::bad_alloc();
    }

    if (mp_isneg(&m_number) == MP_YES)
    {
        mp_int fullRange;
        if (mp_init(&fullRange) == MP_OKAY)
        {
            m_lastErr = mp_2expt(&fullRange, static_cast<int>(m_bitWidth));
            m_lastErr = mp_mod(&m_number, &fullRange, &targetBits);
            m_lastErr = mp_add(&targetBits, &fullRange, &targetBits);
            mp_clear(&fullRange);
        }
    }
    else
    {
        m_lastErr = mp_copy(&m_number, &targetBits);
    }

    size_t writtenBitsSize = mp_ubin_size(&targetBits);

    if (writtenBitsSize > 0)
    {
        uint8_t stackBuf[128];
        uint8_t *rawBeBytesPtr = stackBuf;
        std::pmr::vector<uint8_t> dynamicTempBuf(alloc);

        if (writtenBitsSize > sizeof(stackBuf))
        {
            dynamicTempBuf.resize(writtenBitsSize);
            rawBeBytesPtr = dynamicTempBuf.data();
        }

        m_lastErr = mp_to_ubin(&targetBits, rawBeBytesPtr, writtenBitsSize, nullptr);

        size_t offset = (byteSize >= writtenBitsSize) ? (byteSize - writtenBitsSize) : 0;
        size_t copyBytes = std::min(byteSize, writtenBitsSize);

        std::copy(rawBeBytesPtr + (writtenBitsSize - copyBytes),
                  rawBeBytesPtr + writtenBitsSize,
                  buffer.begin() + offset);
    }

    mp_clear(&targetBits);

    if (!bigEndian)
    {
        std::reverse(buffer.begin(), buffer.end());
    }

    return buffer;
}

void FlexInt::clampToTwosComplement()
{
    if (m_bitWidth == 0)
        return;

    mp_int maxVal, minVal, modMask, fullRange;
    if (mp_init_multi(&maxVal, &minVal, &modMask, &fullRange, nullptr) != MP_OKAY)
    {
        m_lastErr = MP_MEM;
        throw std::bad_alloc();
    }

    m_lastErr = mp_2expt(&fullRange, static_cast<int>(m_bitWidth));

    m_lastErr = mp_copy(&fullRange, &modMask);
    m_lastErr = mp_decr(&modMask);

    if (m_isSigned)
    {
        m_lastErr = mp_2expt(&maxVal, static_cast<int>(m_bitWidth - 1));
        m_lastErr = mp_decr(&maxVal);

        m_lastErr = mp_2expt(&minVal, static_cast<int>(m_bitWidth - 1));
        m_lastErr = mp_neg(&minVal, &minVal);
    }
    else
    {
        m_lastErr = mp_copy(&modMask, &maxVal);
        mp_set(&minVal, 0);
    }

    bool overflowDetected = (mp_cmp(&m_number, &maxVal) == MP_GT);
    bool underflowDetected = (mp_cmp(&m_number, &minVal) == MP_LT);

    if (overflowDetected || underflowDetected)
    {
        if (mp_isneg(&m_number) == MP_YES)
        {
            m_lastErr = mp_mod(&m_number, &fullRange, &m_number);
            m_lastErr = mp_add(&m_number, &fullRange, &m_number);
        }
        m_lastErr = mp_and(&m_number, &modMask, &m_number);

        if (m_isSigned)
        {
            mp_int signBit;
            m_lastErr = mp_init(&signBit);
            m_lastErr = mp_2expt(&signBit, static_cast<int>(m_bitWidth - 1));

            if (mp_cmp(&m_number, &signBit) != MP_LT)
            {
                m_lastErr = mp_sub(&m_number, &fullRange, &m_number);
            }
            mp_clear(&signBit);
        }
    }

    mp_clear_multi(&maxVal, &minVal, &modMask, &fullRange, nullptr);

    if (overflowDetected)
    {
        m_lastErr = MP_VAL;
        throw std::overflow_error("FlexInt arithmetic operation caused an overflow.");
    }
    if (underflowDetected)
    {
        m_lastErr = MP_VAL;
        throw std::underflow_error("FlexInt arithmetic operation caused an underflow.");
    }
}

std::string FlexInt::toString(size_t radix) const
{
    int size, lastErr = MP_OKAY;
    lastErr = mp_radix_size(&m_number, int(radix), &size);

    if (lastErr != MP_OKAY)
    {
        return "ERROR";
    }

    std::string res;
    res.resize(size);

    lastErr = mp_to_radix(&m_number, res.data(), size, NULL, int(radix));

    if (lastErr != MP_OKAY)
    {
        return "ERROR";
    }

    if (!res.empty() && res.back() == '\0')
    {
        res.pop_back();
    }

    return res;
}
