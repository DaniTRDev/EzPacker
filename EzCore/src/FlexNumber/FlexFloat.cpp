#include "FlexNumber/FlexFloat.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

#include <libbf.h>

static void *bf_realloc_wrapper(void *opaque, void *ptr, size_t size) { return std::realloc(ptr, size); }

FlexFloat::FlexFloat(size_t bitWidth) : m_bitWidth(bitWidth), m_lastErr(0)
{
    libbf::bf_context_init(&m_bfCtx, bf_realloc_wrapper, nullptr);
    libbf::bf_init(&m_bfCtx, &m_number);
    libbf::bf_set_zero(&m_number, 0);
}

FlexFloat::FlexFloat(const FlexFloat &other) : FlexFloat(other.getBitSize())
{
    m_lastErr = libbf::bf_set(&m_number, &other.m_number);
    if (m_lastErr & BF_ST_MEM_ERROR)
    {
        throw std::bad_alloc();
    }
}

FlexFloat::FlexFloat(FlexFloat &&other) noexcept : m_bitWidth(other.m_bitWidth), m_lastErr(other.m_lastErr)
{
    m_bfCtx = other.m_bfCtx;
    m_number = other.m_number;

    // Retarget m_number's internal context pointer to this instance's m_bfCtx
    m_number.ctx = &m_bfCtx;

    // Invalidate other so its destructor does not free shared memory
    other.m_number = {};
    other.m_bfCtx = {};
}

FlexFloat::FlexFloat(float value) : FlexFloat(size_t(32))
{
    if (std::isnan(value))
    {
        libbf::bf_set_nan(&m_number);
    }
    else
    {
        libbf::bf_set_float64(&m_number, static_cast<double>(value));
        clampToFloatBounds();
    }
}

FlexFloat::FlexFloat(double value) : FlexFloat(size_t(64))
{
    if (std::isnan(value))
    {
        libbf::bf_set_nan(&m_number);
    }
    else
    {
        libbf::bf_set_float64(&m_number, value);
        clampToFloatBounds();
    }
}

FlexFloat::FlexFloat(std::string_view numberStr, size_t bitWidth, size_t radix) : FlexFloat(bitWidth)
{
    if (numberStr.empty() || radix < 2 || radix > 36)
    {
        throw std::runtime_error("Could not decode float because string is invalid or radix is not supported");
    }

    const char *next_ptr = nullptr;
    std::string str(numberStr);

    m_lastErr = libbf::bf_atof(&m_number, str.c_str(), &next_ptr, int(radix), getPrecBits(), libbf::BF_RNDN);

    if (m_lastErr & BF_ST_MEM_ERROR)
    {
        throw std::bad_alloc();
    }
    if (next_ptr == str.c_str())
    {
        throw std::runtime_error("Error while decoding floating-point value from string");
    }
    clampToFloatBounds();
}

FlexFloat::~FlexFloat()
{
    if (m_bfCtx.realloc_func != nullptr)
    {
        libbf::bf_delete(&m_number);
        libbf::bf_context_end(&m_bfCtx);
    }
}

FlexFloat &FlexFloat::operator=(FlexFloat &&other) noexcept
{
    if (this != &other)
    {
        if (m_bfCtx.realloc_func != nullptr)
        {
            libbf::bf_delete(&m_number);
            libbf::bf_context_end(&m_bfCtx);
        }

        m_bfCtx = other.m_bfCtx;
        m_number = other.m_number;
        m_bitWidth = other.m_bitWidth;
        m_lastErr = other.m_lastErr;

        // Retarget m_number's internal context pointer to this instance's m_bfCtx
        m_number.ctx = &m_bfCtx;

        other.m_number = {};
        other.m_bfCtx = {};
    }
    return *this;
}

FlexFloat &FlexFloat::operator=(const FlexFloat &other)
{
    if (this != &other)
    {
        if (m_bfCtx.realloc_func == nullptr)
        {
            libbf::bf_context_init(&m_bfCtx, bf_realloc_wrapper, nullptr);
            libbf::bf_init(&m_bfCtx, &m_number);
        }

        m_bitWidth = other.m_bitWidth;
        m_lastErr = libbf::bf_set(&m_number, &other.m_number);
        if (m_lastErr & BF_ST_MEM_ERROR)
        {
            throw std::bad_alloc();
        }
    }
    return *this;
}

bool FlexFloat::fitsIn(size_t bitWidth) const
{
    if (bitWidth == 0)
        return false;

    if (libbf::bf_is_nan(&m_number) || !libbf::bf_is_finite(&m_number) || libbf::bf_is_zero(&m_number))
        return true;

    double currentVal;
    libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);

    if (bitWidth <= 32)
    {
        double maxFloat = static_cast<double>(std::numeric_limits<float>::max());
        return (currentVal >= -maxFloat && currentVal <= maxFloat);
    }

    if (bitWidth <= 64)
    {
        return !std::isinf(currentVal);
    }

    FlexFloat tempCopy(*this);
    tempCopy.m_bitWidth = bitWidth;
    libbf::bf_round(&tempCopy.m_number, tempCopy.getPrecBits(), libbf::BF_RNDN);

    return libbf::bf_is_finite(&tempCopy.m_number);
}

bool FlexFloat::hasError() const { return (m_lastErr & BF_ST_MEM_ERROR) != 0; }

bool FlexFloat::isNeg() const { return m_number.sign != 0 && !libbf::bf_is_nan(&m_number); }

bool FlexFloat::isZero() const { return libbf::bf_is_zero(&m_number); }

bool FlexFloat::isPositive() const { return !isNeg() && !isZero() && !libbf::bf_is_nan(&m_number); }

bool FlexFloat::operator>(const FlexFloat &other) const
{
    if (libbf::bf_is_nan(&m_number) || libbf::bf_is_nan(&other.m_number))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) > 0;
}

bool FlexFloat::operator>=(const FlexFloat &other) const
{
    if (libbf::bf_is_nan(&m_number) || libbf::bf_is_nan(&other.m_number))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) >= 0;
}

bool FlexFloat::operator<(const FlexFloat &other) const
{
    if (libbf::bf_is_nan(&m_number) || libbf::bf_is_nan(&other.m_number))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) < 0;
}

bool FlexFloat::operator<=(const FlexFloat &other) const
{
    if (libbf::bf_is_nan(&m_number) || libbf::bf_is_nan(&other.m_number))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) <= 0;
}

bool FlexFloat::operator==(const FlexFloat &other) const
{
    if (libbf::bf_is_nan(&m_number) || libbf::bf_is_nan(&other.m_number))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) == 0;
}

bool FlexFloat::operator!=(const FlexFloat &other) const
{
    if (libbf::bf_is_nan(&m_number) || libbf::bf_is_nan(&other.m_number))
        return true;
    return libbf::bf_cmp(&m_number, &other.m_number) != 0;
}

FlexFloat FlexFloat::operator+(const FlexFloat &other)
{
    FlexFloat res(*this);
    res += other;
    return res;
}

FlexFloat &FlexFloat::operator+=(const FlexFloat &other)
{
    m_lastErr = libbf::bf_add(&m_number, &m_number, &other.m_number, getPrecBits(), libbf::BF_RNDN);
    clampToFloatBounds();
    return *this;
}

FlexFloat FlexFloat::operator-(const FlexFloat &other)
{
    FlexFloat res(*this);
    res -= other;
    return res;
}

FlexFloat &FlexFloat::operator-=(const FlexFloat &other)
{
    m_lastErr = libbf::bf_sub(&m_number, &m_number, &other.m_number, getPrecBits(), libbf::BF_RNDN);
    clampToFloatBounds();
    return *this;
}

FlexFloat FlexFloat::operator*(const FlexFloat &other)
{
    FlexFloat res(*this);
    res *= other;
    return res;
}

FlexFloat &FlexFloat::operator*=(const FlexFloat &other)
{
    m_lastErr = libbf::bf_mul(&m_number, &m_number, &other.m_number, getPrecBits(), libbf::BF_RNDN);
    clampToFloatBounds();
    return *this;
}

FlexFloat FlexFloat::operator/(const FlexFloat &other)
{
    FlexFloat res(*this);
    res /= other;
    return res;
}

FlexFloat &FlexFloat::operator/=(const FlexFloat &other)
{
    if (other.isZero())
    {
        throw std::runtime_error("Floating-point division by zero.");
    }
    m_lastErr = libbf::bf_div(&m_number, &m_number, &other.m_number, getPrecBits(), libbf::BF_RNDN);
    clampToFloatBounds();
    return *this;
}

double FlexFloat::getDouble() const
{
    double currentVal = 0;
    libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);
    return currentVal;
}

float FlexFloat::getFloat() const
{
    double currentVal = 0;
    libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);
    return static_cast<float>(currentVal);
}

size_t FlexFloat::getBitSize() const { return m_bitWidth; }

libbf::limb_t FlexFloat::getPrecBits() const
{
    if (m_bitWidth <= 32)
        return 24; // binary32 mantissa
    if (m_bitWidth <= 64)
        return 53; // binary64 mantissa
    if (m_bitWidth == 128)
        return 113; // binary128 mantissa

    if (m_bitWidth > 128)
    {
        size_t expBits = static_cast<size_t>(std::round(4 * std::log2(static_cast<double>(m_bitWidth)))) - 13;
        return m_bitWidth - expBits - 1;
    }

    return m_bitWidth - 15;
}

FlexFloat FlexFloat::getHighHalf() const
{
    if (m_bitWidth % 2 != 0)
        throw std::runtime_error("Cannot execute floating-point scalar expansion split on an odd bit-width.");

    size_t splitWidth = m_bitWidth / 2;
    FlexFloat highPart(splitWidth);

    double currentVal;
    libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);

    int exp;
    std::frexp(currentVal, &exp);

    libbf::bf_set_float64(&highPart.m_number, static_cast<double>(exp));
    highPart.clampToFloatBounds();
    return highPart;
}

FlexFloat FlexFloat::getLowHalf() const
{
    if (m_bitWidth % 2 != 0)
        throw std::runtime_error("Cannot execute floating-point scalar expansion split on an odd bit-width.");

    size_t splitWidth = m_bitWidth / 2;
    FlexFloat lowPart(splitWidth);

    double currentVal;
    libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);

    int exp;
    double mantissa = std::frexp(currentVal, &exp);

    libbf::bf_set_float64(&lowPart.m_number, mantissa);
    lowPart.clampToFloatBounds();
    return lowPart;
}

void FlexFloat::extend(size_t newBitSize)
{
    if (newBitSize < m_bitWidth)
        throw std::runtime_error("FlexFloat::extend cannot be used to down-cast precision widths.");

    if (newBitSize == m_bitWidth)
        return;

    m_bitWidth = newBitSize;
    clampToFloatBounds();
}

std::pmr::vector<uint8_t> FlexFloat::dump(bool bigEndian, std::pmr::memory_resource *alloc)
{
    size_t byteSize = (m_bitWidth + 7) / 8;
    std::pmr::vector<uint8_t> buffer(byteSize, 0, alloc);

    if (byteSize == 0 || m_bitWidth == 0)
        return buffer;

    // Determine IEEE-754 exponent bits (w) and fraction bits (t)
    size_t w;
    if (m_bitWidth == 16)
        w = 5;
    else if (m_bitWidth == 32)
        w = 8;
    else if (m_bitWidth == 64)
        w = 11;
    else if (m_bitWidth == 128)
        w = 15;
    else if (m_bitWidth > 128)
        w = static_cast<size_t>(std::round(4.0 * std::log2(static_cast<double>(m_bitWidth)))) - 13;
    else
        w = (m_bitWidth >= 15) ? (m_bitWidth - getPrecBits() - 1) : 4;

    if (w >= m_bitWidth - 1)
        w = m_bitWidth / 2;

    size_t t = m_bitWidth - 1 - w;
    int64_t bias = (1LL << (w - 1)) - 1;
    int64_t maxExp = (1LL << w) - 1;

    int sign = (m_number.sign != 0) ? 1 : 0;
    int64_t expField = 0;

    auto set_be_bit = [&](size_t bitIdx, int val)
    {
        if (val && bitIdx < m_bitWidth)
        {
            size_t byteIdx = bitIdx / 8;
            size_t bitInByte = 7 - (bitIdx % 8);
            buffer[byteIdx] |= static_cast<uint8_t>(1U << bitInByte);
        }
    };

    auto get_mantissa_bit = [&](size_t d) -> int
    {
        if (m_number.tab == nullptr || m_number.len == 0)
            return 0;
        size_t limbBits = sizeof(libbf::limb_t) * 8;
        size_t limbOffset = d / limbBits;
        if (limbOffset >= static_cast<size_t>(m_number.len))
            return 0;
        size_t limbIdx = static_cast<size_t>(m_number.len) - 1 - limbOffset;
        size_t bitInLimb = (limbBits - 1) - (d % limbBits);
        return static_cast<int>((m_number.tab[limbIdx] >> bitInLimb) & 1U);
    };

    // 1. Sign bit (bit 0)
    if (sign)
    {
        set_be_bit(0, 1);
    }

    // 2. Exponent and Fraction encoding
    if (libbf::bf_is_nan(&m_number))
    {
        expField = maxExp;
        set_be_bit(1 + w, 1); // Quiet NaN flag (MSB of fraction)
    }
    else if (!libbf::bf_is_finite(&m_number))
    {
        expField = maxExp;
    }
    else if (libbf::bf_is_zero(&m_number))
    {
        expField = 0;
    }
    else
    {
        int64_t unbiasedExp = static_cast<int64_t>(m_number.expn) - 1;
        int64_t e = unbiasedExp + bias;

        if (e >= maxExp)
        {
            expField = maxExp;
        }
        else if (e >= 1)
        {
            expField = e;
            for (size_t i = 0; i < t; ++i)
            {
                int bitVal = get_mantissa_bit(1 + i);
                set_be_bit(1 + w + i, bitVal);
            }
        }
        else
        {
            // Subnormal / Denormalized
            expField = 0;
            int64_t shift = 1 - e;
            if (shift <= static_cast<int64_t>(t))
            {
                for (size_t i = 0; i < t; ++i)
                {
                    int bitVal = 0;
                    if (static_cast<int64_t>(i) == shift - 1)
                    {
                        bitVal = 1;
                    }
                    else if (static_cast<int64_t>(i) > shift - 1)
                    {
                        size_t d = static_cast<size_t>(static_cast<int64_t>(i) - shift);
                        bitVal = get_mantissa_bit(1 + d);
                    }
                    set_be_bit(1 + w + i, bitVal);
                }
            }
        }
    }

    // 3. Write Exponent bits (bits 1 to w)
    for (size_t i = 0; i < w; ++i)
    {
        int expBit = static_cast<int>((expField >> (w - 1 - i)) & 1);
        set_be_bit(1 + i, expBit);
    }

    // 4. Output Endianness Adjustment
    if (!bigEndian)
    {
        std::reverse(buffer.begin(), buffer.end());
    }

    return buffer;
}

std::string FlexFloat::toString(size_t radix) const
{
    if (radix < 2 || radix > 36)
        throw std::runtime_error("Unsupported radix for string conversion.");

    size_t length = 0;
    libbf::bf_flags_t flags = BF_FTOA_FORMAT_FREE | libbf::BF_RNDN;
    char *rawStr = libbf::bf_ftoa(&length, &m_number, static_cast<int>(radix), getPrecBits(), flags);

    if (!rawStr)
        throw std::bad_alloc();

    std::string result(rawStr, length);
    std::free(rawStr);
    return result;
}

void FlexFloat::clampToFloatBounds()
{
    if (m_bitWidth == 0 || libbf::bf_is_nan(&m_number) || !libbf::bf_is_finite(&m_number))
        return;

    if (m_bitWidth <= 64)
    {
        double currentVal;
        libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);

        if (m_bitWidth == 32)
        {
            if (currentVal > static_cast<double>(std::numeric_limits<float>::max()))
                throw std::overflow_error("FlexFloat arithmetic caused an f32 precision target overflow.");
            if (currentVal < static_cast<double>(-std::numeric_limits<float>::max()))
                throw std::underflow_error("FlexFloat arithmetic caused an f32 precision target underflow.");

            float truncated = static_cast<float>(currentVal);
            libbf::bf_set_float64(&m_number, static_cast<double>(truncated));
        }
        else if (m_bitWidth == 64)
        {
            if (std::isinf(currentVal))
                throw std::overflow_error("FlexFloat arithmetic caused an f64 precision target overflow.");
        }
    }
    else
    {
        libbf::bf_round(&m_number, getPrecBits(), libbf::BF_RNDN);
    }
}