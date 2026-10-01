#ifndef EZCORE_DENSE_BIT_SET_H
#define EZCORE_DENSE_BIT_SET_H

#include "EzCoreCommon.h"
#include <bit>
#include <memory_resource>

/**
 * Fixed-capacity 64-bit word-aligned dense bitset used for compiler dataflow analysis and register liveness.
 * Stores contiguous bitfields inside an arena-aware std::pmr::vector of uint64_t words, providing fast bitwise union
 * and transfer function evaluations.
 */
class DenseBitSet
{
  public:
    /**
     * Default constructor creating an empty bitset with zero words allocated using the provided allocator.
     */
    DenseBitSet(std::pmr::memory_resource *alloc = std::pmr::get_default_resource()) : m_words(alloc) {}

    /**
     * Constructs a bitset capable of holding at least numBits, sizing internal 64-bit storage words accordingly.
     */
    explicit DenseBitSet(size_t numBits, std::pmr::memory_resource *alloc = std::pmr::get_default_resource());

    /**
     * Sets the bit at the specified 0-based index to 1.
     * Computes word index (bit / 64) and bit offset (bit % 64) within word.
     */
    void set(size_t bit);

    /**
     * Clears the bit at the specified 0-based index to 0.
     */
    void reset(size_t bit);

    /**
     * Tests whether the bit at the specified index is set to 1.
     * Returns true if the bit is 1, or false if the bit is 0 or out of bounds.
     */
    bool test(size_t bit) const;

    /**
     * Resets all bits in the bitset to 0 without releasing allocated word storage.
     */
    void clear();

    /**
     * Returns true if the bitset has zero allocated words.
     */
    bool empty() const noexcept { return m_words.empty(); }

    /**
     * Returns the bit capacity of the bitset (number of 64-bit words * 64).
     */
    size_t size() const noexcept { return m_words.size() * 64; }

    /**
     * Returns the number of backing 64-bit storage words.
     */
    size_t wordCount() const noexcept { return m_words.size(); }

    /**
     * Counts the total number of set bits (population count) across all words.
     */
    size_t count() const noexcept;

    /**
     * Returns true if at least one bit is set to 1.
     */
    bool any() const noexcept;

    /**
     * Returns true if no bits are set to 1.
     */
    bool none() const noexcept { return !any(); }

    /**
     * Performs an in-place bitwise OR operation (this |= other) across common word bounds.
     * Returns true if any word in this bitset was modified (i.e. new bits were added), false otherwise.
     */
    bool unionWith(const DenseBitSet &other);

    /**
     * Evaluates standard compiler liveness transfer equation: LiveIn = Use | (LiveOut & ~Def).
     * Updates internal words in-place and returns true if any word changed value.
     */
    bool computeLiveIn(const DenseBitSet &use, const DenseBitSet &liveOut, const DenseBitSet &def);

    /**
     * Returns the memory resource backing this bitset.
     */
    std::pmr::memory_resource *getResource() const noexcept { return m_words.get_allocator().resource(); }

  private:
    std::pmr::vector<uint64_t> m_words; // Backing storage; each word holds 64 consecutive bits.
};

#endif // EZCORE_DENSE_BIT_SET_H