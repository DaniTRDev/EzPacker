#ifndef EZCORE_DENSE_BIT_SET_H
#define EZCORE_DENSE_BIT_SET_H

#include "EzCoreCommon.h"

/**
 * Fixed-capacity 64-bit word-aligned dense bitset used for compiler dataflow analysis and register liveness.
 * Stores contiguous bitfields inside a standard vector of uint64_t words, providing fast bitwise union
 * and transfer function evaluations.
 */
class DenseBitSet
{
  public:
    /**
     * Default constructor creating an empty bitset with zero words allocated.
     */
    DenseBitSet() = default;

    /**
     * Constructs a bitset capable of holding at least numBits, sizing internal 64-bit storage words accordingly.
     */
    explicit DenseBitSet(size_t numBits);

    /**
     * Sets the bit at the specified 0-based index to 1.
     * Computes word index (bit / 64) and bit offset (bit % 64) within word.
     */
    void set(size_t bit);

    /**
     * Tests whether the bit at the specified index is set to 1.
     * Returns true if the bit is 1, or false if the bit is 0 or out of bounds.
     */
    bool test(size_t bit) const;

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

  private:
    std::vector<uint64_t> m_words;
};

#endif // EZCORE_DENSE_BIT_SET_H