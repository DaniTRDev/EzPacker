#ifndef EZPACKER_FLEXFLOAT_H
#define EZPACKER_FLEXFLOAT_H

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
     * @param other The source FlexFloat instance to copy.
     */
    FlexFloat(const FlexFloat &other);

    /**
     * Initializes the container and sets a standard single-precision hardware float value into it.
     * @param value Real value to store.
     */
    explicit FlexFloat(float value);

    /**
     * Initializes the container and sets a standard double-precision hardware float value into it.
     * @param value Real value to store.
     */
    explicit FlexFloat(double value);

    /**
     * Initializes the container and parses a safe string slice into a floating-point number using the given base radix.
     * @param numberStr Visual character presentation representing the arbitrary value.
     * @param bitWidth Dynamic accuracy width sizing allocation target.
     * @param radix Literal numeric base tracking space (2 to 36).
     */
    FlexFloat(const std::string_view &numberStr, size_t bitWidth, size_t radix = 10);

    /**
     * Clears internal LibBF registers, frees custom dynamic memory frames, and destroys the object wrapper.
     */
    ~FlexFloat();

    /**
     * Returns true if a allocation panic or memory out-of-bounds flag occurred within the underlying library state.
     * @return true on internal memory exhaustion.
     */
    bool hasError() const;

    /**
     * Returns true if this floating-point value is structurally negative and not a NaN.
     * @return true if the numeric value drops below zero.
     */
    bool isNeg() const;

    /**
     * Returns true if this floating-point value is positive, non-zero, and valid.
     * @return true if number is greater than zero.
     */
    bool isPositive() const;

    /**
     * Returns true if this number evaluates precisely to 0.0 or -0.0.
     * @return true if value has no non-zero magnitudes.
     */
    bool isZero() const;

    /**
     * Compares this against other and returns true if this is greater.
     * @param other
     * @return
     */
    bool operator>(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is greater or equal.
     * @param other
     * @return
     */
    bool operator>=(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is smaller.
     * @param other
     * @return
     */
    bool operator<(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is smaller or equal.
     * @param other
     * @return
     */
    bool operator<=(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is equal to other.
     * @param other
     * @return
     */
    bool operator==(const FlexFloat &other) const;

    /**
     * Compares this against other and returns true if this is not equal to other.
     * @param other
     * @return
     */
    bool operator!=(const FlexFloat &other) const;

    /**
     * Extracts the upper half components (sign and exponent fields) for floating-point scalar expansion.
     * @return A new FlexFloat with a bit-width of half the current size.
     */
    FlexFloat getHighHalf() const;

    /**
     * Extracts the lower half components (the fractional mantissa bits) for floating-point scalar expansion.
     * @return A new FlexFloat with a bit-width of half the current size.
     */
    FlexFloat getLowHalf() const;

    /**
     * Creates a new resulting FlexFloat that's a copy of this and adds other into it.
     * @param other The right hand side operand.
     * @return Value computation result container.
     */
    FlexFloat operator+(const FlexFloat &other);

    /**
     * Increments this value by other and returns a reference to this instance.
     * @param other The right hand side operand.
     * @return Updated self instance.
     */
    FlexFloat &operator+=(const FlexFloat &other);

    /**
     * Creates a new resulting FlexFloat that's a copy of this and subtracts other from it.
     * @param other The right hand side operand.
     * @return Value computation result container.
     */
    FlexFloat operator-(const FlexFloat &other);

    /**
     * Decrements this value by other and returns a reference to this instance.
     * @param other The right hand side operand.
     * @return Updated self instance.
     */
    FlexFloat &operator-=(const FlexFloat &other);

    /**
     * Creates a new resulting FlexFloat that's a copy of this and multiplies it by other.
     * @param other The right hand side operand.
     * @return Value computation result container.
     */
    FlexFloat operator*(const FlexFloat &other);

    /**
     * Multiplies this value by other and returns a reference to this instance.
     * @param other The right hand side operand.
     * @return Updated self instance.
     */
    FlexFloat &operator*=(const FlexFloat &other);

    /**
     * Creates a new resulting FlexFloat that's a copy of this and divides it by other.
     * @param other The right hand side operand.
     * @return Value computation result container.
     */
    FlexFloat operator/(const FlexFloat &other);

    /**
     * Divides this value by other and returns a reference to this instance.
     * @param other The right hand side operand.
     * @throw std::runtime_error if other evaluates exactly to zero.
     * @return Updated self instance.
     */
    FlexFloat &operator/=(const FlexFloat &other);

    /**
     * Returns the configured tracking bit size of this float container representation (e.g., 32, 64, 128, 256).
     * @return Total structural bit architecture layout parameter size.
     */
    size_t getBitSize() const;

    /**
     * Dumps the arbitrary floating point number into an IEEE-754 or custom layout byte array profile.
     * Standard hardware lengths (32-bit/64-bit) match IEEE layouts perfectly, while larger styles serialize
     * structural backing components down natively.
     *
     * If bigEndian is set to true, the number will be dumped in big endian format; if it is set to false, the number
     * will be dumped in little endian.
     * @param bigEndian Byte organization flow rule flags.
     * @param alloc Target polymorphic memory tracking provider interface.
     * @return Vector chunk representing exported structural binary byte spaces.
     */
    std::pmr::vector<uint8_t> dump(bool bigEndian, std::pmr::memory_resource *alloc = std::pmr::get_default_resource());

    /**
     * Returns the string representation of the number with the given radix.
     * @param radix
     * @return
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
     * @return Exact precision bit allocation count limit.
     */
    libbf::limb_t getPrecBits() const;

  private:
    libbf::bf_context_t m_bfCtx;
    libbf::bf_t m_number;
    int m_lastErr;
    size_t m_bitWidth;
};

#endif // EZPACKER_FLEXFLOAT_H