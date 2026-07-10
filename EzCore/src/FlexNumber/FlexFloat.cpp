#include "FlexNumber/FlexFloat.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <limits>

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

FlexFloat::FlexFloat(const std::string_view &numberStr, size_t bitWidth, size_t radix) : FlexFloat(bitWidth)
{
    if (numberStr.empty() || radix < 2 || radix > 36)
    {
        throw std::runtime_error("Could not decode float because string is invalid or radix is not supported");
    }

    // Force null-termination out of incoming std::string_view safely
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
    libbf::bf_delete(&m_number);
    libbf::bf_context_end(&m_bfCtx);
}

bool FlexFloat::hasError() const { return (m_lastErr & BF_ST_MEM_ERROR) != 0; }

bool FlexFloat::isNeg() const { return m_number.sign != 0 && !libbf::bf_is_nan(&m_number); }

bool FlexFloat::isZero() const { return libbf::bf_is_zero(&m_number); }

bool FlexFloat::isPositive() const { return !isNeg() && !isZero() && !libbf::bf_is_nan(&m_number); }

bool FlexFloat::operator>(const FlexFloat &other) const
{
    if (bf_is_nan(&m_number) || bf_is_nan(&other.m_number))
        return false;
    return bf_cmp(&m_number, &other.m_number) > 0;
}
bool FlexFloat::operator>=(const FlexFloat &other) const
{
    if (bf_is_nan(&m_number) || bf_is_nan(&other.m_number))
        return false;
    return bf_cmp(&m_number, &other.m_number) >= 0;
}

bool FlexFloat::operator<(const FlexFloat &other) const
{
    if (bf_is_nan(&m_number) || bf_is_nan(&other.m_number))
        return false;
    return bf_cmp(&m_number, &other.m_number) < 0;
}
bool FlexFloat::operator<=(const FlexFloat &other) const
{
    if (bf_is_nan(&m_number) || bf_is_nan(&other.m_number))
        return false;
    return bf_cmp(&m_number, &other.m_number) <= 0;
}

bool FlexFloat::operator==(const FlexFloat &other) const
{
    if (bf_is_nan(&m_number) || bf_is_nan(&other.m_number))
        return false;
    return bf_cmp(&m_number, &other.m_number) == 0;
}
bool FlexFloat::operator!=(const FlexFloat &other) const
{
    // According to IEEE-754 rules, NaN != NaN is always true,
    // and NaN != any_number is also always true.
    if (bf_is_nan(&m_number) || bf_is_nan(&other.m_number))
        return true;
    return bf_cmp(&m_number, &other.m_number) != 0;
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

size_t FlexFloat::getBitSize() const { return m_bitWidth; }

libbf::limb_t FlexFloat::getPrecBits() const
{
    if (m_bitWidth <= 32)
        return 24; // binary32 mantissa
    if (m_bitWidth <= 64)
        return 53; // binary64 mantissa
    if (m_bitWidth == 128)
        return 113; // binary128 mantissa

    // Dynamically handle arbitrary dimensions safely (IEEE style allocation rules)
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
    {
        throw std::runtime_error("Cannot execute floating-point scalar expansion split on an odd bit-width.");
    }
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
    {
        throw std::runtime_error("Cannot execute floating-point scalar expansion split on an odd bit-width.");
    }
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

// Updated serialization to support arbitrary-length structural exports safely
std::pmr::vector<uint8_t> FlexFloat::dump(bool bigEndian, std::pmr::memory_resource *alloc)
{
    size_t byteSize = (m_bitWidth + 7) / 8;
    std::pmr::vector<uint8_t> buffer(byteSize, 0, alloc);

    if (byteSize == 0)
        return buffer;

    if (m_bitWidth == 32)
    {
        double rawDouble;
        libbf::bf_get_float64(&m_number, &rawDouble, libbf::BF_RNDN);
        float rawFloat = static_cast<float>(rawDouble);
        std::copy_n(reinterpret_cast<const uint8_t *>(&rawFloat), sizeof(float), buffer.data());
    }
    else if (m_bitWidth == 64)
    {
        double rawDouble;
        libbf::bf_get_float64(&m_number, &rawDouble, libbf::BF_RNDN);
        std::copy_n(reinterpret_cast<const uint8_t *>(&rawDouble), sizeof(double), buffer.data());
    }
    else
    {
        // Safe continuous fallback for any-length layouts: serialize internal data bits
        // cleanly across raw byte space limits up to bitWidth capacity
        size_t limbsToCopy = std::min(byteSize, (size_t)(m_number.len * sizeof(libbf::limb_t)));
        if (limbsToCopy > 0 && m_number.tab != nullptr)
        {
            std::memcpy(buffer.data(), m_number.tab, limbsToCopy);
        }
    }

    bool hostBigEndian = false;
#if defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    hostBigEndian = true;
#endif

    if (bigEndian != hostBigEndian)
    {
        std::reverse(buffer.begin(), buffer.end());
    }

    return buffer;
}

std::string FlexFloat::toString(size_t radix) const
{
    if (radix < 2 || radix > 36)
    {
        throw std::runtime_error("Unsupported radix for string conversion.");
    }

    size_t length = 0;
    libbf::bf_flags_t flags = BF_FTOA_FORMAT_FREE | libbf::BF_RNDN;
    char *rawStr = bf_ftoa(&length, &m_number, static_cast<int>(radix), getPrecBits(), flags);

    if (!rawStr)
    {
        throw std::bad_alloc();
    }

    // Capture the output in a C++ string safely before releasing the raw C memory
    std::string result(rawStr, length);

    // Free using standard system free, context allocator uses std::realloc/malloc
    std::free(rawStr);
    return result;
}

void FlexFloat::clampToFloatBounds()
{
    if (m_bitWidth == 0 || libbf::bf_is_nan(&m_number) || !libbf::bf_is_finite(&m_number))
        return;

    // Only apply hard thresholds if we fit inside standard hardware type bounds
    if (m_bitWidth <= 64)
    {
        double currentVal;
        libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);

        if (m_bitWidth == 32)
        {
            if (currentVal > static_cast<double>(std::numeric_limits<float>::max()))
            {
                throw std::overflow_error("FlexFloat arithmetic caused an f32 precision target overflow.");
            }
            if (currentVal < static_cast<double>(-std::numeric_limits<float>::max()))
            {
                throw std::underflow_error("FlexFloat arithmetic caused an f32 precision target underflow.");
            }

            float truncated = static_cast<float>(currentVal);
            libbf::bf_set_float64(&m_number, static_cast<double>(truncated));
        }
        else if (m_bitWidth == 64)
        {
            if (std::isinf(currentVal))
            {
                throw std::overflow_error("FlexFloat arithmetic caused an f64 precision target overflow.");
            }
        }
    }
    else
    {
        // For arbitrary length numbers (e.g. 128+ bits), use LibBF's internal tracking round-offs
        // to conform limits to requested target structures dynamically.
        libbf::bf_round(&m_number, getPrecBits(), libbf::BF_RNDN);
    }
}
