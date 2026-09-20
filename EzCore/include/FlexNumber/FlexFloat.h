#ifndef EZCORE_FLEX_FLOAT_H
#define EZCORE_FLEX_FLOAT_H

#include "EzCoreCommon.h"

/**
 * This class acts as a wrapper for LibBF's arbitrary-precision bf_t engine. It provides a set of utility methods
 * and operators to work with flexible-width, any-length floating-point numbers as if they were normal hardware
 * floating-point values.
 *
 * By default, this class assumes IEEE.754 floating point rules.
 */
class FlexFloat
{
  public:
    /**
     * Initializes the LibBF calculation context and clamps it to a target precision bit-width (e.g., 32, 64, 128,
     * etc.). If internal contexts or backing structures could not be initialized, it throws std::bad_alloc.
     */
    explicit FlexFloat(size_t bitWidth);

    /**
     * Creates a DEEP copy of other, cloning its allocation values and numeric parameters safely.
     */
    FlexFloat(const FlexFloat &other);

    /**
     * Moves other into this.
     */
    FlexFloat(FlexFloat &&other) noexcept;

    /**
     * Initializes the container and sets a standard single-precision hardware float value into it.
     */
    explicit FlexFloat(float value);

    /**
     * Initializes the container and sets a standard double-precision hardware float value into it.
     */
    explicit FlexFloat(double value);

    /**
     * Initializes the container and parses a safe string slice into a floating-point number using the given base radix.
     */
    FlexFloat(std::string_view numberStr, size_t bitWidth, size_t radix = 10);

    /**
     * Clears internal LibBF registers, frees custom dynamic memory frames, and destroys the object wrapper.
     */
    ~FlexFloat();

    /**
     * Moves other into this.
     */
    FlexFloat &operator=(FlexFloat &&other) noexcept;

    /**
     * Copies other into this.
     */
    FlexFloat &operator=(const FlexFloat &other);

    /**
     * Returns true if this floating-point value is structurally negative and not a NaN.
     */
    bool isNeg() const;

    /**
     * Returns true if this floating-point value is positive, non-zero, and valid.
     */
    bool isPositive() const;

    /**
     * Returns true if this number evaluates precisely to 0.0 or -0.0.
     */
    bool isZero() const;

    /**
     * Compares this against other and returns true if this is greater.
     */
    bool operator>(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is greater or equal.
     */
    bool operator>=(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is smaller.
     */
    bool operator<(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is smaller or equal.
     */
    bool operator<=(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is equal to other.
     */
    bool operator==(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is not equal to other.
     */
    bool operator!=(const FlexFloat &other) const;

    /**
     * Extracts the upper half components (sign and exponent fields) for floating-point scalar expansion.
     */
    FlexFloat getHighHalf() const;

    /**
     * Extracts the lower half components (the fractional mantissa bits) for floating-point scalar expansion.
     */
    FlexFloat getLowHalf() const;

    /**
     * Creates a new resulting FlexFloat that's a copy of this and adds other into it.
     */
    FlexFloat operator+(const FlexFloat &other);

    /**
     * Increments this value by other and returns a reference to this instance.
     */
    FlexFloat &operator+=(const FlexFloat &other);

    /**
     * Creates a new resulting FlexFloat that's a copy of this and subtracts other from it.
     */
    FlexFloat operator-(const FlexFloat &other);

    /**
     * Decrements this value by other and returns a reference to this instance.
     */
    FlexFloat &operator-=(const FlexFloat &other);

    /**
     * Creates a new resulting FlexFloat that's a copy of this and multiplies it by other.
     */
    FlexFloat operator*(const FlexFloat &other);

    /**
     * Multiplies this value by other and returns a reference to this instance.
     */
    FlexFloat &operator*=(const FlexFloat &other);

    /**
     * Creates a new resulting FlexFloat that's a copy of this and divides it by other.
     */
    FlexFloat operator/(const FlexFloat &other);

    /**
     * Divides this value by other and returns a reference to this instance. Throws std::domain_error if other
     * evaluates exactly to zero.
     */
    FlexFloat &operator/=(const FlexFloat &other);

    /**
     * Returns the current value as a double. If number is bigger|smaller (float, float128) ROUNDING is used so it may
     * become INF.
     */
    double getDouble() const;

    /**
     * Returns the current value as a float. If number is bigger|smaller (float16, double) ROUNDING is used so it may
     * become INF.
     */
    float getFloat() const;

    /**
     * Returns the configured tracking bit size of this float container representation (e.g., 32, 64, 128, 256).
     */
    size_t getBitSize() const;

    /**
     * Extends the float value to the new bit size. If it is smaller than the previous, a
     * std::invalid_argument exception is thrown.
     */
    void extend(size_t newBitSize);

    /**
     * Returns the string representation of the number with the given radix.
     */
    std::string toString(size_t radix = 2) const;

  private:
    /**
     * Enforces precision rules and bounds handling criteria mapped against requested layout sizing parameters
     * (e.g. handling NaN allocations, checking hardware min/max overruns, or truncating/rounding mantissas using
     * BF_RNDN).
     */
    void clampToFloatBounds();

    /**
     * Computes or looks up the explicit mantissa bit precision limits used by LibBF operational formulas
     * matching the configured structural layout configurations.
     */
    libbf::limb_t getPrecBits() const;

  private:
    libbf::bf_context_t m_bfCtx; // Per-instance libbf allocator/context owning this number's memory.
    libbf::bf_t m_number;        // The underlying arbitrary-precision bf_t value, bound to m_bfCtx.
    int m_lastErr;               // libbf status flags (e.g. BF_ST_MEM_ERROR) from the last operation.
    size_t m_bitWidth;           // Configured storage width in bits that determines precision and range.
};

#endif // EZCORE_FLEX_FLOAT_H