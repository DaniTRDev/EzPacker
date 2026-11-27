#include "HelperClasses/DenseBitSet.h"
#include <algorithm>

/**
 * Rounds numBits up to the next 64-bit word and allocates that many zero-initialized words.
 */
DenseBitSet::DenseBitSet(size_t numBits, allocator_type alloc) : m_words((numBits + 63) / 64, 0, alloc) {}

/**
 * Sets the addressed bit. Requests beyond the allocated word capacity are silently ignored.
 */
void DenseBitSet::set(size_t bit)
{
    if (bit / 64 < m_words.size())
        m_words[bit / 64] |= (1ULL << (bit % 64));
}

/**
 * Clears the addressed bit. Requests beyond the allocated word capacity are silently ignored.
 */
void DenseBitSet::clearBit(size_t bit)
{
    if (bit / 64 < m_words.size())
        m_words[bit / 64] &= ~(1ULL << (bit % 64));
}

/**
 * Tests the addressed bit, returning false for indices past the allocated capacity.
 */
bool DenseBitSet::test(size_t bit) const
{
    if (bit / 64 >= m_words.size())
        return false;
    return (m_words[bit / 64] & (1ULL << (bit % 64))) != 0;
}

/**
 * Clears all bits to 0 while retaining allocated word capacity.
 */
void DenseBitSet::reset() { std::fill(m_words.begin(), m_words.end(), 0ULL); }

/**
 * Resizes the bitset to hold at least numBits, zero-initializing any newly added words.
 */
void DenseBitSet::resize(size_t numBits)
{
    const size_t newWords = (numBits + 63) / 64;
    m_words.resize(newWords, 0ULL);
}

/**
 * Returns the total number of set bits across all words.
 */
size_t DenseBitSet::count() const
{
    size_t total = 0;
    for (uint64_t word : m_words)
    {
        total += static_cast<size_t>(std::popcount(word));
    }
    return total;
}

/**
 * ORs other into this bitset word by word, stopping at the shorter length, and reports whether any
 * word changed.
 */
bool DenseBitSet::unionWith(const DenseBitSet &other)
{
    bool changed = false;
    size_t wordCount = std::min(m_words.size(), other.m_words.size());
    for (size_t i = 0; i < wordCount; ++i)
    {
        uint64_t oldVal = m_words[i];
        m_words[i] |= other.m_words[i];
        if (m_words[i] != oldVal)
            changed = true;
    }
    return changed;
}

/**
 * Computes (this = a | b) across the maximum word bounds of a and b, resizing if needed.
 */
void DenseBitSet::copyOr(const DenseBitSet &a, const DenseBitSet &b)
{
    const size_t aWords = a.m_words.size();
    const size_t bWords = b.m_words.size();
    const size_t maxWords = std::max(aWords, bWords);
    if (m_words.size() < maxWords)
        m_words.resize(maxWords, 0ULL);

    const size_t minWords = std::min(aWords, bWords);
    for (size_t i = 0; i < minWords; ++i)
        m_words[i] = a.m_words[i] | b.m_words[i];
    for (size_t i = minWords; i < aWords; ++i)
        m_words[i] = a.m_words[i];
    for (size_t i = minWords; i < bWords; ++i)
        m_words[i] = b.m_words[i];
    for (size_t i = maxWords; i < m_words.size(); ++i)
        m_words[i] = 0ULL;
}

/**
 * Writes the liveness transfer result LiveIn = Use | (LiveOut & ~Def) into this bitset and reports
 * whether any word changed, which callers use as the fixed-point convergence test.
 */
bool DenseBitSet::computeLiveIn(const DenseBitSet &use, const DenseBitSet &liveOut, const DenseBitSet &def)
{
    bool changed = false;
    // Bound by the shortest operand so the transfer stays within every bitset's word storage.
    size_t count = std::min({ m_words.size(), use.m_words.size(), liveOut.m_words.size(), def.m_words.size() });
    // Compute Use | (LiveOut & ~Def) across the shared word bounds
    for (size_t i = 0; i < count; ++i)
    {
        uint64_t oldVal = m_words[i];
        uint64_t newVal = use.m_words[i] | (liveOut.m_words[i] & ~def.m_words[i]);
        m_words[i] = newVal;
        if (newVal != oldVal)
            changed = true;
    }
    return changed;
}