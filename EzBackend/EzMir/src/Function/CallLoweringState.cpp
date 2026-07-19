#include "Function/CallLoweringState.h"

CallLoweringState::CallLoweringState(const std::vector<PhysicalRegId> &usableGprs,
                                     const std::vector<PhysicalRegId> &usableFprs) :
    m_currentStackOffset(0), m_usableFprs(usableFprs), m_usableGprs(usableGprs)
{
}

bool CallLoweringState::allocateGpr(PhysicalRegId &outReg)
{
    if (m_usableGprs.empty())
        return false;

    outReg = m_usableGprs.front();
    m_usableGprs.erase(m_usableGprs.begin());
    m_allocatedGprs.push_back(outReg); // Tracks it as used

    return true;
}

bool CallLoweringState::allocateFpr(PhysicalRegId &outReg)
{
    if (m_usableFprs.empty())
        return false;

    outReg = m_usableFprs.front();
    m_usableFprs.erase(m_usableFprs.begin());
    m_allocatedFprs.push_back(outReg); // Tracks it as used

    return true;
}

size_t CallLoweringState::getUsableFprCount() const { return m_usableFprs.size(); }

size_t CallLoweringState::getUsableGprCount() const { return m_usableGprs.size(); }

size_t CallLoweringState::getUsedFprCount() const { return m_allocatedFprs.size(); }

size_t CallLoweringState::getUsedGprCount() const { return m_allocatedGprs.size(); }

int64_t CallLoweringState::getStackOffset() const { return m_currentStackOffset; }

int64_t CallLoweringState::allocateStackSlot(size_t sizeBytes, size_t alignmentBytes)
{
    // Ensure non-sized parameters don't affect the allocation (void types).
    if (sizeBytes == 0)
        return m_currentStackOffset;

    // Align the stack offset up to the requested alignment.
    // Example: offset = 4, alignmentBytes = 8 -> (4 + 7) & ~7 -> 11 & 0xFFFFFFF8 = 8.
    m_currentStackOffset = (m_currentStackOffset + int64_t(alignmentBytes) - 1) & ~(alignmentBytes - 1);
    int64_t assignedOffset = m_currentStackOffset;

    // Advance the frame cursor state by the allocated type size.
    m_currentStackOffset += static_cast<int64_t>(sizeBytes);
    return assignedOffset;
}