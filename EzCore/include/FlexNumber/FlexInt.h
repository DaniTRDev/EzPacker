#ifndef EZPACKER_FLEXINT_H
#define EZPACKER_FLEXINT_H

#include "EzCoreCommon.h"

/**
 * This class acts as a wrapper for libtommath's mp_int. It provides a set of utility and operators to be able to work
 * with flexible-width integers as if they were normal integers.
 */
class FlexInt
{
  public:
    /**
     * Creates a DEEP copy of other.
     * @param other
     */
    FlexInt(const FlexInt &other);

    /**
     * Initializes the container and sets an unsigned int32 value into it. If the container could not be initialized, it
     * throws std::bad_alloc.
     * @param value
     * @param bitWidth
     */
    explicit FlexInt(uint32_t value, size_t bitWidth = 32);

    /**
     * Initializes the container and sets an unsigned int64 value into it. If the container could not be initialized, it
     * throws std::bad_alloc.
     * @param value
     * @param bitWidth
     */
    explicit FlexInt(uint64_t value, size_t bitWidth = 64);

    /**
     * Initializes the container and sets an signed int32 value into it. If the container could not be initialized, it
     * throws std::bad_alloc.
     * @param value
     * @param bitWidth
     */
    explicit FlexInt(int32_t value, size_t bitWidth = 32);

    /**
     * Initializes the container and sets an signed int64 value into it. If the container could not be initialized, it
     * throws std::bad_alloc.
     * @param value
     * @param bitWidth
     */
    explicit FlexInt(int64_t value, size_t bitWidth = 64);

    /**
     * Initializes the container and converts the string to a number using the given base. If the number starts with
     * a '-' it will be treat as a signed integer, if it doesn't it will be unsigned.
     * @param numberStr
     * @param bitWidth
     * @param _signed
     * @param radix
     */
    FlexInt(const std::string_view &numberStr, size_t bitWidth, bool _signed, size_t radix = 10);

    /**
     * Copies other into this.
     */
    FlexInt &operator=(const FlexInt &other);

    /**
     * Clears the container of the number and destroys the object.
     */
    ~FlexInt();

    /**
     * Returns true if this number fits in a container of the given bitSize and signedess.
     * @param bitSize
     * @param _signed
     * @return
     */
    bool fitsIn(size_t bitSize, bool _signed);

    /**
     * Returns true if this number threw an error somewhere during its uses.
     * @return
     */
    bool hasError() const;

    /**
     * Returns true if this number is even.
     * @return
     */
    bool isEven() const;

    /**
     * Returns true if this number is negative.
     * @return
     */
    bool isNeg() const;

    /**
     * Returns true if this number is odd.
     * @return
     */
    bool isOdd() const;

    /**
     * Returns true if this number is positive.
     * @return
     */
    bool isPositive() const;

    /**
     * Returns true if this is a signed number.
     */
    bool isSigned() const;

    /**
     * Returns true if this number is zero.
     * @return
     */
    bool isZero() const;

    /**
     * Compares this against other and returns true if this is greater.
     * @param other
     * @return
     */
    bool operator>(const FlexInt &other) const;

    /**
     * Compares this against other and returns true if this is greater or equal.
     * @param other
     * @return
     */
    bool operator>=(const FlexInt &other) const;

    /**
     * Compares this against other and returns true if this is smaller.
     * @param other
     * @return
     */
    bool operator<(const FlexInt &other) const;

    /**
     * Compares this against other and returns true if this is smaller or equal.
     * @param other
     * @return
     */
    bool operator<=(const FlexInt &other) const;

    /**
     * Compares this against other and returns true if this is equal to other.
     * @param other
     * @return
     */
    bool operator==(const FlexInt &other) const;

    /**
     * Compares this against other and returns true if this is not equal to other.
     * @param other
     * @return
     */
    bool operator!=(const FlexInt &other) const;

    /**
     * Extracts the upper half of this integer.
     * @return A new FlexInt inheriting the current signedness, with a bit-width of half the current size.
     */
    FlexInt getHighHalf();

    /**
     * Extracts the lower half of this integer .
     * @return A new unsigned FlexInt with a bit-width of exactly half the current size.
     */
    FlexInt getLowHalf();

    /**
     * Creates a new resulting FlexInt that's a copy of this and adds other into it.
     * @param other
     * @return
     */
    FlexInt operator+(const FlexInt &other);

    /**
     * Increments by 1 this and returns a reference to this.
     * @param other
     * @return
     */
    FlexInt &operator++();

    /**
     * Increments by other this and returns a reference to this.
     * @param other
     * @return
     */
    FlexInt &operator+=(const FlexInt &other);

    /**
     * Creates a new resulting FlexInt that's a copy of this and subtracts other into it.
     * @param other
     * @return
     */
    FlexInt operator-(const FlexInt &other);

    /**
     * Decrements by 1 this and returns a reference to this.
     * @param other
     * @return
     */
    FlexInt &operator--();

    /**
     * Decrements by other this and returns a reference to this.
     * @param other
     * @return
     */
    FlexInt &operator-=(const FlexInt &other);

    /**
     * Creates a new resulting FlexInt that's a copy of this and multiplies by other.
     * @param other
     * @return
     */
    FlexInt operator*(const FlexInt &other);

    /**
     * Multiplies this by other this and returns a reference to this.
     * @param other
     * @return
     */
    FlexInt &operator*=(const FlexInt &other);

    /**
     * Creates a new resulting FlexInt that's a copy of this and divides by other.
     * @param other
     * @return
     */
    FlexInt operator/(const FlexInt &other);

    /**
     * Divides this by other this and returns a reference to this.
     * @param other
     * @return
     */
    FlexInt &operator/=(const FlexInt &other);

    /**
     * Creates a new resulting FlexInt that's a copy of this and calculates this mod other
     * @param other
     * @return
     */
    FlexInt operator%(const FlexInt &other);

    /**
     * Calculates this mod other this and returns a reference to this.
     * @param other
     * @return
     */
    FlexInt &operator%=(const FlexInt &other);

    /**
     * Returns the integer in 8 bits, WILL TRUNC IF BIGGER.
     */
    int8_t getI8() const;

    /**
     * Returns the integer in 8 bits, WILL TRUNC IF BIGGER.
     */
    int16_t getI16() const;

    /**
     * Returns the integer in 8 bits, WILL TRUNC IF BIGGER.
     */
    int32_t getI32() const;

    /**
     * Returns the integer in 8 bits, WILL TRUNC IF BIGGER.
     */
    int64_t getI64() const;

    /**
     * Returns the UNSIGNED VERSION of the integer in 8 bits, WILL TRUNC IF BIGGER.
     */
    uint8_t getU8() const;

    /**
     * Returns the UNSIGNED VERSION of the integer in 16 bits, WILL TRUNC IF BIGGER.
     */
    uint16_t getU16() const;

    /**
     * Returns the UNSIGNED VERSION of the integer in 32 bits, WILL TRUNC IF BIGGER.
     */
    uint32_t getU32() const;

    /**
     * Returns the UNSIGNED VERSION of the integer in 64 bits, WILL TRUNC IF BIGGER.
     */
    uint64_t getU64() const;

    /**
     * Returns the bit size of this number.
     * @return
     */
    size_t getBitSize() const;

    /**
     * Extends the value to the given bitsize and sign. If newBitSize is smaller than current bit size,
     * a bad_alloc exception is thrown.
     * @param newBitSize
     * @param isSigned
     */
    void extend(size_t newBitSize, bool isSigned);

    /**
     * Dumps the number into a binary-encoded byte array. If alloc is provided, the resulting vector will be allocated
     * using it.
     *
     * If bigEndian is set to true, the number will be dumped in big endian format; if it is set to false, the number
     * will be dumped in little endian.
     * @param bigEndian
     * @param alloc
     * @return
     */
    std::pmr::vector<uint8_t> dump(bool bigEndian, std::pmr::memory_resource *alloc = std::pmr::get_default_resource());

    /**
     * Returns the string representation of the number with the given radix.
     * @param radix
     * @return
     */
    std::string toString(size_t radix = 10) const;

  private:
    /**
     * Ensure the number is ALWAYS in Ca2 and it fits in the given bit width, if it isn't an exception is thrown and
     * m_lastErr is updated.
     */
    void clampToTwosComplement();

  private:
    bool m_isSigned;
    mp_err m_lastErr;
    mp_int m_number;
    size_t m_bitWidth;
};

#endif // EZPACKER_FLEXINT_H
