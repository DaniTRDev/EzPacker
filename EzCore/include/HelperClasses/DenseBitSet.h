#ifndef EZCORE_DENSE_BIT_SET_H
#define EZCORE_DENSE_BIT_SET_H

#include "EzCoreCommon.h"

/**
 * Helper utility class that makes |=, &= operators work for high-dense bit fields.
 */
class DenseBitSet
{
  public:
    DenseBitSet() = default;
    explicit DenseBitSet(size_t numBits);

    /**
     * Sets the given dense bit index.
     */
    void set(size_t bit);

    /**
     * Returns 1 if the given bit index != 0.
     */
    bool test(size_t bit) const;

    /**
     * this |= other; returns true if this changed
     */
    bool unionWith(const DenseBitSet &other);

    /**
     * this = use | (liveOut & ~def); returns true if this changed
     */
    bool computeLiveIn(const DenseBitSet &use, const DenseBitSet &liveOut, const DenseBitSet &def);

  private:
    std::vector<uint64_t> m_words;
};

#endif // EZCORE_DENSE_BIT_SET_H