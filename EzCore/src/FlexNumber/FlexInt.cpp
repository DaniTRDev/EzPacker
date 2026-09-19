#include "FlexNumber/FlexInt.h"
#include <algorithm>
#include <stdexcept>

/**
 * Deep-copies other's mp_int value along with its signedness and width; throws std::bad_alloc
 * if the copy cannot be allocated.
 */
FlexInt::FlexInt(const FlexInt &other)
{
    if (m_lastErr = mp_init_copy(&m_number, &other.m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = other.m_isSigned;
    m_bitWidth = other.m_bitWidth;
}

/**
 * Move-constructs by adopting other's mp_int handle and metadata, clearing other's handle so its
 * destructor does not free the shared memory.
 */
FlexInt::FlexInt(FlexInt &&other) noexcept
{
    m_bitWidth = other.m_bitWidth;
    m_isSigned = other.m_isSigned;
    m_lastErr = other.m_lastErr;
    m_number = other.m_number;

    // Invalidate other's handle so mp_clear on destructor is a no-op
    other.m_number = {};
}

/**
 * Creates an unsigned integer from a 32-bit value, then clamps it to the requested width.
 */
FlexInt::FlexInt(uint32_t value, size_t bitWidth)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = false;
    m_bitWidth = bitWidth;

    mp_set_ul(&m_number, value);
    clampToTwosComplement();
}

/**
 * Creates an unsigned integer from a 64-bit value, then clamps it to the requested width.
 */
FlexInt::FlexInt(uint64_t value, size_t bitWidth)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = false;
    m_bitWidth = bitWidth;

    mp_set_u64(&m_number, value);
    clampToTwosComplement();
}

/**
 * Creates a signed integer from a 32-bit value, then clamps it to the requested width.
 */
FlexInt::FlexInt(int32_t value, size_t bitWidth)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = true;
    m_bitWidth = bitWidth;

    mp_set_l(&m_number, value);
    clampToTwosComplement();
}

/**
 * Creates a signed integer from a 64-bit value, then clamps it to the requested width.
 */
FlexInt::FlexInt(int64_t value, size_t bitWidth)
{
    if (m_lastErr = mp_init(&m_number); m_lastErr != MP_OKAY)
        throw std::bad_alloc();

    m_isSigned = true;
    m_bitWidth = bitWidth;

    mp_set_i64(&m_number, value);
    clampToTwosComplement();
}

/**
 * Parses a value from text in the given radix (2..64), skipping leading whitespace and an
 * optional 0x/0X prefix for radix 16, then clamps to the target width. Throws std::runtime_error
 * on malformed input, std::bad_alloc on allocation failure.
 */
FlexInt::FlexInt(std::string_view numberStr, size_t bitWidth, bool _signed, size_t radix)
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

    std::string safeStr(numberStr.substr(scanIdx));

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

/**
 * Releases the mp_int storage when the handle is still owned; moved-from instances are skipped.
 */
FlexInt::~FlexInt()
{
    if (m_number.dp != nullptr)
    {
        mp_clear(&m_number);
    }
}

/**
 * Move-assigns by freeing this instance's current value, then stealing other's handle/metadata
 * and clearing other to prevent a double free.
 */
FlexInt &FlexInt::operator=(FlexInt &&other) noexcept
{
    if (this != &other)
    {
        if (m_number.dp != nullptr)
        {
            mp_clear(&m_number);
        }

        m_bitWidth = other.m_bitWidth;
        m_isSigned = other.m_isSigned;
        m_lastErr = other.m_lastErr;
        m_number = other.m_number;

        other.m_number = {};
    }
    return *this;
}

/**
 * Copy-assigns the numeric value and metadata from other; throws std::bad_alloc if the mp_int
 * copy fails.
 */
FlexInt &FlexInt::operator=(const FlexInt &other)
{
    if (this != &other)
    {
        if (m_lastErr = mp_copy(&other.m_number, &m_number); m_lastErr != MP_OKAY)
            throw std::bad_alloc();

        m_isSigned = other.m_isSigned;
        m_bitWidth = other.m_bitWidth;
    }
    return *this;
}

/**
 * Reports whether the value is representable in bitSize bits with the requested signedness.
 * Zero always fits; signed fitting requires the sign bit and, for the exact minimum, only the
 * most-significant bit set; negative values never fit unsigned.
 */
bool FlexInt::fitsIn(size_t bitSize, bool _signed)
{
    if (bitSize == 0)
        return false;

    if (mp_iszero(&m_number) == MP_YES)
        return true;

    int bits = mp_count_bits(&m_number);

    if (_signed)
    {
        int maxBits = static_cast<int>(bitSize - 1);
        if (mp_isneg(&m_number) == MP_YES)
        {
            if (bits < static_cast<int>(bitSize))
                return true;
            if (bits == static_cast<int>(bitSize))
                return mp_cnt_lsb(&m_number) == maxBits;
            return false;
        }
        return bits <= maxBits;
    }

    if (mp_isneg(&m_number) == MP_YES)
        return false;

    return bits <= static_cast<int>(bitSize);
}

/**
 * Returns true when the last libtommath operation did not succeed.
 */
bool FlexInt::hasError() const { return m_lastErr != MP_OKAY; }

/**
 * Returns true when the value has no least-significant set bit.
 */
bool FlexInt::isEven() const { return mp_iseven(&m_number) == MP_YES; }

/**
 * Returns true when the value is signed and structurally negative.
 */
bool FlexInt::isNeg() const { return m_isSigned && mp_isneg(&m_number) == MP_YES; }

/**
 * Returns true when the value is not even.
 */
bool FlexInt::isOdd() const { return !isEven(); }

/**
 * Returns true when the value is strictly greater than zero.
 */
bool FlexInt::isPositive() const { return !isNeg() && !isZero(); }

/**
 * Returns true when this instance was constructed with signed semantics.
 */
bool FlexInt::isSigned() const { return m_isSigned; }

/**
 * Returns true when the value is exactly zero.
 */
bool FlexInt::isZero() const { return mp_iszero(&m_number) == MP_YES; }

/**
 * Compares values, requiring identical width and signedness; throws std::runtime_error otherwise.
 */
bool FlexInt::operator>(const FlexInt &other) const
{
    if (m_bitWidth != other.m_bitWidth || m_isSigned != other.m_isSigned)
        throw std::runtime_error("Mismatched target types in FlexInt comparison");
    return mp_cmp(&m_number, &other.m_number) == MP_GT;
}

/**
 * Compares values, requiring identical width and signedness; throws std::runtime_error otherwise.
 */
bool FlexInt::operator>=(const FlexInt &other) const
{
    if (m_bitWidth != other.m_bitWidth || m_isSigned != other.m_isSigned)
        throw std::runtime_error("Mismatched target types in FlexInt comparison");
    auto res = mp_cmp(&m_number, &other.m_number);
    return res == MP_GT || res == MP_EQ;
}

/**
 * Compares values, requiring identical width and signedness; throws std::runtime_error otherwise.
 */
bool FlexInt::operator<(const FlexInt &other) const
{
    if (m_bitWidth != other.m_bitWidth || m_isSigned != other.m_isSigned)
        throw std::runtime_error("Mismatched target types in FlexInt comparison");
    return mp_cmp(&m_number, &other.m_number) == MP_LT;
}

/**
 * Compares values, requiring identical width and signedness; throws std::runtime_error otherwise.
 */
bool FlexInt::operator<=(const FlexInt &other) const
{
    if (m_bitWidth != other.m_bitWidth || m_isSigned != other.m_isSigned)
        throw std::runtime_error("Mismatched target types in FlexInt comparison");
    auto res = mp_cmp(&m_number, &other.m_number);
    return res == MP_LT || res == MP_EQ;
}

/**
 * Equality that tolerates differing widths/signedness: equal metadata compares directly,
 * differing signs are unequal, non-negative values compare directly, and negative values are
 * sign-extended to the common width before comparing.
 */
bool FlexInt::operator==(const FlexInt &other) const
{
    if (m_bitWidth == other.m_bitWidth && m_isSigned == other.m_isSigned)
        return mp_cmp(&m_number, &other.m_number) == MP_EQ;

    bool thisNeg = this->isNeg();
    bool otherNeg = other.isNeg();

    if (thisNeg != otherNeg)
        return false;

    if (!thisNeg && !otherNeg)
        return mp_cmp(&m_number, &other.m_number) == MP_EQ;

    FlexInt cloneThis(*this);
    FlexInt cloneOther(other);

    size_t targetWidth = std::max(m_bitWidth, other.m_bitWidth);
    cloneThis.extend(targetWidth, true);
    cloneOther.extend(targetWidth, true);

    return mp_cmp(&cloneThis.m_number, &cloneOther.m_number) == MP_EQ;
}

/**
 * Negation of operator==, so it inherits the same cross-width comparison semantics.
 */
bool FlexInt::operator!=(const FlexInt &other) const { return !(*this == other); }

/**
 * Returns the upper half of a scalar expansion split (value >> (width/2)), reinterpreting the
 * result as signed when this instance is signed. Negative values are first normalized into the
 * unsigned range. Throws if the width is odd.
 */
FlexInt FlexInt::getHighHalf()
{
    if (m_bitWidth % 2 != 0)
        throw std::runtime_error("Cannot execute scalar split on an odd bit-width.");

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

/**
 * Returns the lower half of a scalar expansion split (value & ((1<<(width/2))-1)) as an unsigned
 * value; negative inputs are normalized into the unsigned range first. Throws if the width is odd.
 */
FlexInt FlexInt::getLowHalf()
{
    if (m_bitWidth % 2 != 0)
        throw std::runtime_error("Cannot execute scalar expansion split on an odd bit-width.");

    size_t splitWidth = m_bitWidth / 2;
    FlexInt lowPart(uint64_t(0), splitWidth);
    lowPart.m_isSigned = false;

    mp_int mask;
    if (mp_init(&mask) != MP_OKAY)
        throw std::bad_alloc();

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

/**
 * Returns the sum as a new value, leaving both operands unchanged.
 */
FlexInt FlexInt::operator+(const FlexInt &other)
{
    FlexInt res(*this);
    res += other;
    return res;
}

/**
 * Pre-increments the value and re-clamps it to the configured width.
 */
FlexInt &FlexInt::operator++()
{
    m_lastErr = mp_incr(&m_number);
    clampToTwosComplement();
    return *this;
}

/**
 * Adds other in place and re-clamps the result to the configured width.
 */
FlexInt &FlexInt::operator+=(const FlexInt &other)
{
    m_lastErr = mp_add(&m_number, &other.m_number, &m_number);
    clampToTwosComplement();
    return *this;
}

/**
 * Returns the difference as a new value, leaving both operands unchanged.
 */
FlexInt FlexInt::operator-(const FlexInt &other)
{
    FlexInt res(*this);
    res -= other;
    return res;
}

/**
 * Pre-decrements the value and re-clamps it to the configured width.
 */
FlexInt &FlexInt::operator--()
{
    m_lastErr = mp_decr(&m_number);
    clampToTwosComplement();
    return *this;
}

/**
 * Subtracts other in place and re-clamps the result to the configured width.
 */
FlexInt &FlexInt::operator-=(const FlexInt &other)
{
    m_lastErr = mp_sub(&m_number, &other.m_number, &m_number);
    clampToTwosComplement();
    return *this;
}

/**
 * Returns the product as a new value, leaving both operands unchanged.
 */
FlexInt FlexInt::operator*(const FlexInt &other)
{
    FlexInt res(*this);
    res *= other;
    return res;
}

/**
 * Multiplies in place and re-clamps the result to the configured width.
 */
FlexInt &FlexInt::operator*=(const FlexInt &other)
{
    m_lastErr = mp_mul(&m_number, &other.m_number, &m_number);
    clampToTwosComplement();
    return *this;
}

/**
 * Returns the quotient as a new value, leaving both operands unchanged.
 */
FlexInt FlexInt::operator/(const FlexInt &other)
{
    FlexInt res(*this);
    res /= other;
    return res;
}

/**
 * Divides in place (truncating quotient) and re-clamps the result to the configured width.
 */
FlexInt &FlexInt::operator/=(const FlexInt &other)
{
    m_lastErr = mp_div(&m_number, &other.m_number, &m_number, nullptr);
    clampToTwosComplement();
    return *this;
}

/**
 * Returns the remainder as a new value, leaving both operands unchanged.
 */
FlexInt FlexInt::operator%(const FlexInt &other)
{
    FlexInt res(*this);
    res %= other;
    return res;
}

/**
 * Takes the remainder in place and re-clamps the result to the configured width.
 */
FlexInt &FlexInt::operator%=(const FlexInt &other)
{
    m_lastErr = mp_mod(&m_number, &other.m_number, &m_number);
    clampToTwosComplement();
    return *this;
}

/**
 * Returns the value truncated to a signed 8-bit integer.
 */
int8_t FlexInt::getI8() const { return static_cast<int8_t>(getI64()); }
/**
 * Returns the value truncated to a signed 16-bit integer.
 */
int16_t FlexInt::getI16() const { return static_cast<int16_t>(getI64()); }
/**
 * Returns the value truncated to a signed 32-bit integer.
 */
int32_t FlexInt::getI32() const { return static_cast<int32_t>(getI64()); }

/**
 * Returns the value as a signed 64-bit integer, using the signed or unsigned accessor depending
 * on the value's sign.
 */
int64_t FlexInt::getI64() const
{
    if (mp_isneg(&m_number) == MP_YES)
        return mp_get_i64(&m_number);
    return static_cast<int64_t>(mp_get_u64(&m_number));
}

/**
 * Returns the value truncated to an unsigned 8-bit integer.
 */
uint8_t FlexInt::getU8() const { return static_cast<uint8_t>(getU64()); }
/**
 * Returns the value truncated to an unsigned 16-bit integer.
 */
uint16_t FlexInt::getU16() const { return static_cast<uint16_t>(getU64()); }
/**
 * Returns the value truncated to an unsigned 32-bit integer.
 */
uint32_t FlexInt::getU32() const { return static_cast<uint32_t>(getU64()); }

/**
 * Returns the value as an unsigned 64-bit integer, two's-complement-converting negative values.
 */
uint64_t FlexInt::getU64() const
{
    if (mp_isneg(&m_number) == MP_YES)
        return static_cast<uint64_t>(mp_get_i64(&m_number));
    return mp_get_u64(&m_number);
}

/**
 * Returns the configured storage width in bits.
 */
size_t FlexInt::getBitSize() const { return m_bitWidth; }

/**
 * Widens the storage to newBitSize, adopting the requested signedness. Widening is rejected with
 * std::bad_alloc; when the old value was negative and the target is now unsigned, the value is
 * converted into the unsigned range before clamping.
 */
void FlexInt::extend(size_t newBitSize, bool isSigned)
{
    if (newBitSize < m_bitWidth)
        throw std::bad_alloc();

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

/**
 * Serializes the value as a two's-complement byte buffer of (bitWidth+7)/8 bytes, normalizing
 * negative values into range, writing big-endian bytes and optionally reversing for little-endian.
 */
std::pmr::vector<uint8_t> FlexInt::dump(bool bigEndian, std::pmr::memory_resource *alloc)
{
    size_t byteSize = (m_bitWidth + 7) / 8;
    std::pmr::vector<uint8_t> buffer(byteSize, 0, alloc);

    if (byteSize == 0)
        return buffer;

    mp_int targetBits;
    if (mp_init(&targetBits) != MP_OKAY)
        throw std::bad_alloc();

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

/**
 * Constrains the value to the representable range of m_bitWidth bits: values already in range
 * return via a bit-count fast path, otherwise the value is truncated/two's-complement wrapped
 * (using native 64-bit arithmetic for widths <=64 and libtommath for larger widths), and any
 * overflow/underflow is reported by throwing std::overflow_error / std::underflow_error.
 */
void FlexInt::clampToTwosComplement()
{
    if (m_bitWidth == 0)
        return;

    // -------------------------------------------------------------------------
    // O(1) FAST-PATH: Inspect bit count to verify if m_number already fits
    // -------------------------------------------------------------------------
    if (mp_iszero(&m_number) == MP_YES)
        return;

    bool overflowDetected = false;
    bool underflowDetected = false;
    int bits = mp_count_bits(&m_number);

    if (m_isSigned)
    {
        int maxBits = static_cast<int>(m_bitWidth - 1);
        if (mp_isneg(&m_number) == MP_YES)
        {
            // Valid signed range: [-2^(m_bitWidth - 1), 2^(m_bitWidth - 1) - 1]
            if (bits < static_cast<int>(m_bitWidth))
            {
                return; // Strictly within (-2^(m_bitWidth - 1), 0)
            }
            else if (bits == static_cast<int>(m_bitWidth))
            {
                // Fits ONLY if magnitude is exactly 2^(m_bitWidth - 1) (single MSB set)
                if (mp_cnt_lsb(&m_number) == maxBits)
                {
                    return;
                }
                underflowDetected = true;
            }
            else
            {
                underflowDetected = true;
            }
        }
        else
        {
            // Positive signed: max value is 2^(m_bitWidth - 1) - 1 (requires <= m_bitWidth - 1 bits)
            if (bits <= maxBits)
            {
                return;
            }
            overflowDetected = true;
        }
    }
    else
    {
        // Unsigned range: [0, 2^m_bitWidth - 1]
        if (mp_isneg(&m_number) == MP_YES)
        {
            underflowDetected = true;
        }
        else
        {
            if (bits <= static_cast<int>(m_bitWidth))
            {
                return;
            }
            overflowDetected = true;
        }
    }

    // -------------------------------------------------------------------------
    // CLAMPING & TRUNCATION
    // -------------------------------------------------------------------------

    // Native integer fast-path for widths <= 64
    if (m_bitWidth <= 64)
    {
        uint64_t mag = mp_get_u64(&m_number);
        uint64_t val = (mp_isneg(&m_number) == MP_YES) ? (~mag + 1ULL) : mag;
        uint64_t mask = (m_bitWidth == 64) ? ~0ULL : ((1ULL << m_bitWidth) - 1ULL);
        val &= mask;

        if (m_isSigned)
        {
            uint64_t signBit = 1ULL << (m_bitWidth - 1);
            if ((val & signBit) != 0)
            {
                int64_t sval = (m_bitWidth == 64) ? static_cast<int64_t>(val) : static_cast<int64_t>(val | ~mask);
                mp_set_i64(&m_number, sval);
            }
            else
            {
                mp_set_u64(&m_number, val);
            }
        }
        else
        {
            mp_set_u64(&m_number, val);
        }
    }
    else
    {
        // Slow path for arbitrary widths (> 64 bits)
        mp_int fullRange, modMask;
        if (mp_init_multi(&fullRange, &modMask, nullptr) != MP_OKAY)
        {
            m_lastErr = MP_MEM;
            throw std::bad_alloc();
        }

        m_lastErr = mp_2expt(&fullRange, static_cast<int>(m_bitWidth));
        m_lastErr = mp_copy(&fullRange, &modMask);
        m_lastErr = mp_decr(&modMask);

        if (mp_isneg(&m_number) == MP_YES)
        {
            m_lastErr = mp_mod(&m_number, &fullRange, &m_number);
            m_lastErr = mp_add(&m_number, &fullRange, &m_number);
        }
        m_lastErr = mp_and(&m_number, &modMask, &m_number);

        if (m_isSigned)
        {
            mp_int signBit;
            if (mp_init(&signBit) == MP_OKAY)
            {
                m_lastErr = mp_2expt(&signBit, static_cast<int>(m_bitWidth - 1));
                if (mp_cmp(&m_number, &signBit) != MP_LT)
                {
                    m_lastErr = mp_sub(&m_number, &fullRange, &m_number);
                }
                mp_clear(&signBit);
            }
        }

        mp_clear_multi(&fullRange, &modMask, nullptr);
    }

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

/**
 * Formats the value in the given radix. Returns the literal "ERROR" if libtommath cannot size or
 * convert the number; a trailing NUL from the conversion is stripped.
 */
std::string FlexInt::toString(size_t radix) const
{
    int size, lastErr = MP_OKAY;
    lastErr = mp_radix_size(&m_number, int(radix), &size);

    if (lastErr != MP_OKAY)
        return "ERROR";

    std::string res;
    res.resize(size);

    lastErr = mp_to_radix(&m_number, res.data(), size, nullptr, int(radix));

    if (lastErr != MP_OKAY)
        return "ERROR";

    if (!res.empty() && res.back() == '\0')
        res.pop_back();

    return res;
}