#include "HelperClasses/DenseBitSet.h"

DenseBitSet::DenseBitSet(size_t numBits) : m_words((numBits + 63) / 64, 0) {}

void DenseBitSet::set(size_t bit)
{
    // Compute word index and set the corresponding bit flag if within allocated word capacity
    if (bit / 64 < m_words.size())
        m_words[bit / 64] |= (1ULL << (bit % 64));
}

bool DenseBitSet::test(size_t bit) const
{
    // Return false for any query beyond current bit capacity
    if (bit / 64 >= m_words.size())
        return false;
    return (m_words[bit / 64] & (1ULL << (bit % 64))) != 0;
}

bool DenseBitSet::unionWith(const DenseBitSet &other)
{
    bool changed = false;
    // Iterate over shared word bounds and perform bitwise OR
    size_t count = std::min(m_words.size(), other.m_words.size());
    for (size_t i = 0; i < count; ++i)
    {
        uint64_t oldVal = m_words[i];
        m_words[i] |= other.m_words[i];
        if (m_words[i] != oldVal)
            changed = true;
    }
    return changed;
}

bool DenseBitSet::computeLiveIn(const DenseBitSet &use, const DenseBitSet &liveOut, const DenseBitSet &def)
{
    bool changed = false;
    // Compute Use | (LiveOut & ~Def) across all words
    for (size_t i = 0; i < m_words.size(); ++i)
    {
        uint64_t oldVal = m_words[i];
        uint64_t newVal = use.m_words[i] | (liveOut.m_words[i] & ~def.m_words[i]);
        m_words[i] = newVal;
        if (newVal != oldVal)
            changed = true;
    }
    return changed;
}