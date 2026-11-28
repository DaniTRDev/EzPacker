#ifndef EZCORE_DENSE_BIT_SET_H
#define EZCORE_DENSE_BIT_SET_H

#include "EzCoreCommon.h"
#include <bit>

/**
 * Capacity-configured 64-bit word-aligned dense bitset used for compiler dataflow analysis and
 * interference graphs. Stores contiguous bitfields inside a PMR vector of uint64_t words,
 * supporting uses-allocator construction, fast bitwise union, population count, and bit iteration.
 */
class DenseBitSet
{
  public:
    using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

    /**
     * Default constructor creating an empty bitset with zero words allocated.
     */
    DenseBitSet() = default;

    /**
     * Constructs an empty bitset bound to the given PMR allocator.
     */
    explicit DenseBitSet(allocator_type alloc) : m_words(alloc) {}

    /**
     * Constructs a bitset capable of holding at least numBits, sizing internal 64-bit storage words accordingly.
     */
    explicit DenseBitSet(size_t numBits, allocator_type alloc = {});

    DenseBitSet(const DenseBitSet &) = default;
    DenseBitSet(DenseBitSet &&) noexcept = default;
    DenseBitSet(const DenseBitSet &other, allocator_type alloc) : m_words(other.m_words, alloc) {}
    DenseBitSet(DenseBitSet &&other, allocator_type alloc) noexcept : m_words(std::move(other.m_words), alloc) {}

    DenseBitSet &operator=(const DenseBitSet &) = default;
    DenseBitSet &operator=(DenseBitSet &&) noexcept = default;

    /**
     * Sets the bit at the specified 0-based index to 1.
     */
    void set(size_t bit);

    /**
     * Clears the bit at the specified 0-based index to 0.
     */
    void clearBit(size_t bit);

    /**
     * Tests whether the bit at the specified index is set to 1.
     * Returns true if the bit is 1, or false if the bit is 0 or out of bounds.
     */
    bool test(size_t bit) const;

    /**
     * Clears all bits to 0 while retaining allocated word capacity.
     */
    void reset();

    /**
     * Resizes the bitset to hold at least numBits, zero-initializing any newly added words.
     */
    void resize(size_t numBits);

    /**
     * Returns the total number of set bits in this bitset.
     */
    [[nodiscard]] size_t count() const;

    /**
     * Performs an in-place bitwise OR operation (this |= other) across common word bounds.
     * Returns true if any word in this bitset was modified (i.e. new bits were added), false otherwise.
     */
    bool unionWith(const DenseBitSet &other);

    /**
     * Computes (this = a | b) across the maximum word bounds of a and b, resizing if needed.
     */
    void copyOr(const DenseBitSet &a, const DenseBitSet &b);

    /**
     * Evaluates standard compiler liveness transfer equation: LiveIn = Use | (LiveOut & ~Def).
     * Updates internal words in-place and returns true if any word changed value.
     */
    bool computeLiveIn(const DenseBitSet &use, const DenseBitSet &liveOut, const DenseBitSet &def);

    /**
     * Invokes fn(size_t bitIndex) in ascending order for every set bit using hardware ctz.
     */
    template <typename Fn>
    void forEachSetBit(Fn &&fn) const
    {
        for (size_t wordIdx = 0; wordIdx < m_words.size(); ++wordIdx)
        {
            uint64_t word = m_words[wordIdx];
            while (word != 0)
            {
                const unsigned tz = static_cast<unsigned>(std::countr_zero(word));
                fn((wordIdx << 6U) + tz);
                word &= (word - 1);
            }
        }
    }

  private:
    std::pmr::vector<uint64_t> m_words; // Backing storage; each word holds 64 consecutive bits.
};

#endif // EZCORE_DENSE_BIT_SET_H