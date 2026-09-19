#ifndef EZCORE_FLEX_INT_H
#define EZCORE_FLEX_INT_H

#include "EzCoreCommon.h"

/**
 * Multi-precision arbitrary-width integer wrapper built atop LibTomMath's mp_int structure.
 * Supports configurable bitwidths, signed/unsigned semantics, two's complement clamping,
 * arithmetic operators, radix-based parsing/formatting, endianness-aware binary serialization,
 * and high/low scalar half splitting for compiler legalization.
 */
class FlexInt
{
  public:
    /**
     * Copy constructor performing a deep copy of the underlying LibTomMath mp_int structure.
     */
    FlexInt(const FlexInt &other);

    /**
     * Move constructor transferring ownership of the mp_int representation without reallocating.
     */
    FlexInt(FlexInt &&other) noexcept;

    /**
     * Constructs a FlexInt from an unsigned 32-bit integer with the specified bitwidth.
     */
    explicit FlexInt(uint32_t value, size_t bitWidth = 32);

    /**
     * Constructs a FlexInt from an unsigned 64-bit integer with the specified bitwidth.
     */
    explicit FlexInt(uint64_t value, size_t bitWidth = 64);

    /**
     * Constructs a FlexInt from a signed 32-bit integer with the specified bitwidth.
     */
    explicit FlexInt(int32_t value, size_t bitWidth = 32);

    /**
     * Constructs a FlexInt from a signed 64-bit integer with the specified bitwidth.
     */
    explicit FlexInt(int64_t value, size_t bitWidth = 64);

    /**
     * Parses a string into a multi-precision integer using the specified radix, bitwidth, and signedness.
     * Supports prefixes like '0x', '0b', '0o', and negative sign signs.
     */
    FlexInt(std::string_view numberStr, size_t bitWidth, bool _signed, size_t radix = 10);

    /**
     * Move assignment operator transferring the underlying mp_int state.
     */
    FlexInt &operator=(FlexInt &&other) noexcept;

    /**
     * Copy assignment operator performing a deep copy of the underlying mp_int structure.
     */
    FlexInt &operator=(const FlexInt &other);

    /**
     * Destructor clearing the underlying LibTomMath mp_int memory.
     */
    ~FlexInt();

    /**
     * Checks whether the current numerical value fits within the specified bit size and signedness bounds
     * without overflow or truncation.
     */
    bool fitsIn(size_t bitSize, bool _signed);

    /**
     * Returns true if a LibTomMath error occurred during the most recent operation.
     */
    bool hasError() const;

    /**
     * Returns true if the integer value is even.
     */
    bool isEven() const;

    /**
     * Returns true if the integer value is strictly negative (< 0).
     */
    bool isNeg() const;

    /**
     * Returns true if the integer value is odd.
     */
    bool isOdd() const;

    /**
     * Returns true if the integer value is strictly positive (> 0).
     */
    bool isPositive() const;

    /**
     * Returns true if this instance was constructed with signed semantics.
     */
    bool isSigned() const;

    /**
     * Returns true if the integer value is equal to zero.
     */
    bool isZero() const;

    /**
     * Returns true if this value is strictly greater than other.
     */
    bool operator>(const FlexInt &other) const;

    /**
     * Returns true if this value is greater than or equal to other.
     */
    bool operator>=(const FlexInt &other) const;

    /**
     * Returns true if this value is strictly less than other.
     */
    bool operator<(const FlexInt &other) const;

    /**
     * Returns true if this value is less than or equal to other.
     */
    bool operator<=(const FlexInt &other) const;

    /**
     * Returns true if this value is equal to other in numerical value, sign, and bitwidth.
     */
    bool operator==(const FlexInt &other) const;

    /**
     * Returns true if this value differs from other.
     */
    bool operator!=(const FlexInt &other) const;

    /**
     * Splits this integer and extracts the most significant half (bitWidth / 2 bits).
     */
    FlexInt getHighHalf();

    /**
     * Splits this integer and extracts the least significant half (bitWidth / 2 bits).
     */
    FlexInt getLowHalf();

    /**
     * Returns the sum of this integer and other, clamped to the target bitwidth.
     */
    FlexInt operator+(const FlexInt &other);

    /**
     * Increments this value in-place by 1.
     */
    FlexInt &operator++();

    /**
     * Adds other to this value in-place.
     */
    FlexInt &operator+=(const FlexInt &other);

    /**
     * Returns the difference of this integer minus other, clamped to the target bitwidth.
     */
    FlexInt operator-(const FlexInt &other);

    /**
     * Decrements this value in-place by 1.
     */
    FlexInt &operator--();

    /**
     * Subtracts other from this value in-place.
     */
    FlexInt &operator-=(const FlexInt &other);

    /**
     * Returns the product of this integer multiplied by other.
     */
    FlexInt operator*(const FlexInt &other);

    /**
     * Multiplies this value by other in-place.
     */
    FlexInt &operator*=(const FlexInt &other);

    /**
     * Returns the quotient of this integer divided by other.
     */
    FlexInt operator/(const FlexInt &other);

    /**
     * Divides this value by other in-place.
     */
    FlexInt &operator/=(const FlexInt &other);

    /**
     * Returns the remainder (modulus) of this integer divided by other.
     */
    FlexInt operator%(const FlexInt &other);

    /**
     * Computes the modulus of this value by other in-place.
     */
    FlexInt &operator%=(const FlexInt &other);

    /**
     * Truncates or converts the value to a native signed 8-bit integer (int8_t).
     */
    int8_t getI8() const;

    /**
     * Truncates or converts the value to a native signed 16-bit integer (int16_t).
     */
    int16_t getI16() const;

    /**
     * Truncates or converts the value to a native signed 32-bit integer (int32_t).
     */
    int32_t getI32() const;

    /**
     * Truncates or converts the value to a native signed 64-bit integer (int64_t).
     */
    int64_t getI64() const;

    /**
     * Truncates or converts the value to a native unsigned 8-bit integer (uint8_t).
     */
    uint8_t getU8() const;

    /**
     * Truncates or converts the value to a native unsigned 16-bit integer (uint16_t).
     */
    uint16_t getU16() const;

    /**
     * Truncates or converts the value to a native unsigned 32-bit integer (uint32_t).
     */
    uint32_t getU32() const;

    /**
     * Truncates or converts the value to a native unsigned 64-bit integer (uint64_t).
     */
    uint64_t getU64() const;

    /**
     * Returns the configured bit size of this integer.
     */
    size_t getBitSize() const;

    /**
     * Extends or truncates the integer to a new bit size and signedness representation.
     */
    void extend(size_t newBitSize, bool isSigned);

    /**
     * Dumps the number into a binary byte vector using the specified endianness.
     * Allocates via the supplied PMR memory resource.
     */
    std::pmr::vector<uint8_t> dump(bool bigEndian, std::pmr::memory_resource *alloc = std::pmr::get_default_resource());

    /**
     * Formats the integer into a string representation in the specified radix (base 2, 8, 10, or 16).
     */
    std::string toString(size_t radix = 10) const;

  private:
    /**
     * Enforces two's complement bit bounds, wrapping or sign-extending values to strictly fit within m_bitWidth.
     */
    void clampToTwosComplement();

  private:
    bool m_isSigned;   // Whether the value is interpreted with two's-complement signed semantics.
    mp_err m_lastErr;  // Status code from the most recent libtommath operation (MP_OKAY on success).
    mp_int m_number;   // Underlying libtommath multi-precision integer holding the magnitude/sign.
    size_t m_bitWidth; // Configured storage width in bits used for clamping and serialization.
};

#endif // EZCORE_FLEX_INT_H
