#include "FlexNumber/FlexFloat.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>

#include "LibBFWrapper.h"

/**
 * Allocation shim handed to libbf so its limb buffers are managed by the C allocator.
 */
static void *bf_realloc_wrapper(void *opaque, void *ptr, size_t size) { return std::realloc(ptr, size); }

/**
 * Creates a zero-valued number of the requested storage width and initializes its libbf context.
 */
FlexFloat::FlexFloat(size_t bitWidth) : m_bitWidth(bitWidth), m_lastErr(0)
{
    // Sub-32-bit layouts need dedicated exponent/mantissa handling that is not implemented, so
    // reject them instead of silently keeping binary32 precision/range.
    if (bitWidth != 0 && bitWidth < 32)
    {
        throw std::invalid_argument("FlexFloat does not support widths below 32 bits.");
    }

    libbf::bf_context_init(&m_bfCtx, bf_realloc_wrapper, nullptr);
    libbf::bf_init(&m_bfCtx, &m_number);
    libbf::bf_set_zero(&m_number, 0);
}

/**
 * Copy-constructs by first allocating the same width, then deep-copying the libbf number;
 * translates a libbf out-of-memory status into std::bad_alloc.
 */
FlexFloat::FlexFloat(const FlexFloat &other) : FlexFloat(other.getBitSize())
{
    m_lastErr = libbf::bf_set(&m_number, &other.m_number);
    if (m_lastErr & BF_ST_MEM_ERROR)
    {
        throw std::bad_alloc();
    }
}

/**
 * Move-constructs by stealing other's libbf context/number, repointing the number at this
 * instance's context and clearing other so its destructor is a no-op.
 */
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

/**
 * Creates a 32-bit value from a float, preserving NaN and clamping to single precision bounds.
 */
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

/**
 * Creates a 64-bit value from a double, preserving NaN and clamping to double precision bounds.
 */
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

/**
 * Parses a number from text in the given radix (2..36), throwing std::invalid_argument on invalid
 * input and std::bad_alloc on allocation failure, then clamps to the target width.
 */
FlexFloat::FlexFloat(std::string_view numberStr, size_t bitWidth, size_t radix) : FlexFloat(bitWidth)
{
    if (numberStr.empty() || radix < 2 || radix > 36)
    {
        throw std::invalid_argument("Could not decode float because string is invalid or radix is not supported");
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
        throw std::invalid_argument("Error while decoding floating-point value from string");
    }
    clampToFloatBounds();
}

/**
 * Frees the libbf number and context, guarding against moved-from instances whose context was
 * already cleared.
 */
FlexFloat::~FlexFloat()
{
    if (m_bfCtx.realloc_func != nullptr)
    {
        libbf::bf_delete(&m_number);
        libbf::bf_context_end(&m_bfCtx);
    }
}

/**
 * Move-assigns by releasing this instance's existing libbf state, adopting other's state, then
 * repointing the number and clearing other.
 */
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

/**
 * Copy-assigns by lazily initializing the libbf context when needed (e.g. after a move) and
 * deep-copying the numeric value; throws std::bad_alloc on allocation failure.
 */
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

/**
 * Returns true when the value carries a negative sign; NaN is treated as non-negative.
 */
bool FlexFloat::isNeg() const noexcept { return m_number.sign != 0 && !libbf::bf_is_nan(&m_number); }

/**
 * Returns true when the value is exactly zero.
 */
bool FlexFloat::isZero() const noexcept { return libbf::bf_is_zero(&m_number); }

/**
 * Returns true when the value is finite, non-zero and not negative.
 */
bool FlexFloat::isPositive() const noexcept { return !isNeg() && !isZero() && !libbf::bf_is_nan(&m_number); }

/**
 * Returns true when either this value or other is NaN, in which case ordered comparisons are false.
 */
bool FlexFloat::hasNaNWith(const FlexFloat &other) const noexcept
{
    return libbf::bf_is_nan(&m_number) || libbf::bf_is_nan(&other.m_number);
}

/**
 * Ordering comparison; any NaN operand yields false.
 */
bool FlexFloat::operator>(const FlexFloat &other) const noexcept
{
    if (hasNaNWith(other))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) > 0;
}

/**
 * Ordering comparison; any NaN operand yields false.
 */
bool FlexFloat::operator>=(const FlexFloat &other) const noexcept
{
    if (hasNaNWith(other))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) >= 0;
}

/**
 * Ordering comparison; any NaN operand yields false.
 */
bool FlexFloat::operator<(const FlexFloat &other) const noexcept
{
    if (hasNaNWith(other))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) < 0;
}

/**
 * Ordering comparison; any NaN operand yields false.
 */
bool FlexFloat::operator<=(const FlexFloat &other) const noexcept
{
    if (hasNaNWith(other))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) <= 0;
}

/**
 * Equality comparison; any NaN operand yields false (IEEE-754 semantics).
 */
bool FlexFloat::operator==(const FlexFloat &other) const noexcept
{
    if (hasNaNWith(other))
        return false;
    return libbf::bf_cmp(&m_number, &other.m_number) == 0;
}

/**
 * Inequality comparison; any NaN operand yields true (IEEE-754 semantics).
 */
bool FlexFloat::operator!=(const FlexFloat &other) const noexcept
{
    if (hasNaNWith(other))
        return true;
    return libbf::bf_cmp(&m_number, &other.m_number) != 0;
}

/**
 * Returns the sum as a new value, leaving both operands unchanged.
 */
FlexFloat FlexFloat::operator+(const FlexFloat &other) const { return applyBinary(other, &FlexFloat::operator+=); }

/**
 * Adds other in place using the current precision and rounds the result to this width.
 */
FlexFloat &FlexFloat::operator+=(const FlexFloat &other)
{
    m_lastErr = libbf::bf_add(&m_number, &m_number, &other.m_number, getPrecBits(), libbf::BF_RNDN);
    clampToFloatBounds();
    return *this;
}

/**
 * Returns the difference as a new value, leaving both operands unchanged.
 */
FlexFloat FlexFloat::operator-(const FlexFloat &other) const { return applyBinary(other, &FlexFloat::operator-=); }

/**
 * Subtracts other in place using the current precision and rounds the result to this width.
 */
FlexFloat &FlexFloat::operator-=(const FlexFloat &other)
{
    m_lastErr = libbf::bf_sub(&m_number, &m_number, &other.m_number, getPrecBits(), libbf::BF_RNDN);
    clampToFloatBounds();
    return *this;
}

/**
 * Returns the product as a new value, leaving both operands unchanged.
 */
FlexFloat FlexFloat::operator*(const FlexFloat &other) const { return applyBinary(other, &FlexFloat::operator*=); }

/**
 * Multiplies in place using the current precision and rounds the result to this width.
 */
FlexFloat &FlexFloat::operator*=(const FlexFloat &other)
{
    m_lastErr = libbf::bf_mul(&m_number, &m_number, &other.m_number, getPrecBits(), libbf::BF_RNDN);
    clampToFloatBounds();
    return *this;
}

/**
 * Returns the quotient as a new value, leaving both operands unchanged.
 */
FlexFloat FlexFloat::operator/(const FlexFloat &other) const { return applyBinary(other, &FlexFloat::operator/=); }

/**
 * Divides in place, throwing std::domain_error when the divisor is zero and rounding the
 * result to this width.
 */
FlexFloat &FlexFloat::operator/=(const FlexFloat &other)
{
    if (other.isZero())
    {
        throw std::domain_error("Floating-point division by zero.");
    }
    m_lastErr = libbf::bf_div(&m_number, &m_number, &other.m_number, getPrecBits(), libbf::BF_RNDN);
    clampToFloatBounds();
    return *this;
}

/**
 * Converts the value to double using round-to-nearest-even.
 */
double FlexFloat::getDouble() const
{
    double currentVal = 0;
    libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);
    return currentVal;
}

/**
 * Converts the value to double and narrows it to float.
 */
float FlexFloat::getFloat() const
{
    double currentVal = 0;
    libbf::bf_get_float64(&m_number, &currentVal, libbf::BF_RNDN);
    return static_cast<float>(currentVal);
}

/**
 * Returns the configured storage width in bits.
 */
size_t FlexFloat::getBitSize() const noexcept { return m_bitWidth; }

/**
 * Returns the significand precision in bits for the configured width: 24 for binary32, 53 for
 * binary64, 113 for binary128, and a width-derived estimate otherwise.
 */
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

/**
 * Throws std::invalid_argument when the configured width cannot be split into two equal halves.
 */
void FlexFloat::ensureSplittableWidth() const
{
    if (m_bitWidth % 2 != 0)
        throw std::invalid_argument("Cannot execute floating-point scalar expansion split on an odd bit-width.");
}

/**
 * Returns the "high" piece of a scalar expansion split: the base-2 exponent of the value as
 * returned by std::frexp. Throws if the width is odd (not evenly splittable).
 */
FlexFloat FlexFloat::getHighHalf() const
{
    ensureSplittableWidth();

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

/**
 * Returns the "low" piece of a scalar expansion split: the normalized mantissa in [0.5, 1) as
 * returned by std::frexp. Throws if the width is odd (not evenly splittable).
 */
FlexFloat FlexFloat::getLowHalf() const
{
    ensureSplittableWidth();

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

/**
 * Widens the storage precision to newBitSize (a no-op at equal width) and re-clamps; narrowing
 * is rejected with std::invalid_argument.
 */
void FlexFloat::extend(size_t newBitSize)
{
    if (newBitSize < m_bitWidth)
        throw std::invalid_argument("FlexFloat::extend cannot be used to down-cast precision widths.");

    if (newBitSize != 0 && newBitSize < 32)
        throw std::invalid_argument("FlexFloat does not support widths below 32 bits.");

    if (newBitSize == m_bitWidth)
        return;

    m_bitWidth = newBitSize;
    clampToFloatBounds();
}

/**
 * Formats the value using libbf's free-format conversion in the given radix, copying the
 * heap-allocated result into a std::string and freeing it.
 */
std::string FlexFloat::toString(size_t radix) const
{
    if (radix < 2 || radix > 36)
        throw std::invalid_argument("Unsupported radix for string conversion.");

    size_t length = 0;
    libbf::bf_flags_t flags = BF_FTOA_FORMAT_FREE | libbf::BF_RNDN;
    char *rawStr = libbf::bf_ftoa(&length, &m_number, static_cast<int>(radix), getPrecBits(), flags);

    if (!rawStr)
        throw std::bad_alloc();

    std::string result(rawStr, length);
    std::free(rawStr);
    return result;
}

/**
 * Enforces the numeric range implied by the storage width: 32-bit values are checked against
 * float range (throwing overflow/underflow), 64-bit values against double infinity, and wider
 * targets are rounded to their precision. NaN/Inf/zero and zero-width values are left untouched.
 */
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