#include "CallLoweringState.h"

CallLoweringState::CallLoweringState(ABIDesc *abi) :
    m_abi(abi), m_allocatedFprs(0), m_allocatedGprs(0), m_currentStackOffset(0)
{
}

ABIDesc *CallLoweringState::getABI() const { return m_abi; }

size_t CallLoweringState::getFprCount() const { return m_allocatedFprs; }

size_t CallLoweringState::getGprCount() const { return m_allocatedGprs; }

int64_t CallLoweringState::getStackOffset() const { return m_currentStackOffset; }

void CallLoweringState::consumeFprs(size_t count) { m_allocatedFprs += count; }

void CallLoweringState::consumeGprs(size_t count) { m_allocatedGprs += count; }

int64_t CallLoweringState::allocateStackSlot(size_t sizeBytes, size_t alignmentBytes)
{
    // Ensure non-sized parameters doesn't affect the allocation (void types).
    if (sizeBytes == 0)
        return m_currentStackOffset;

    // Align the running stack offset up to the requested boundary mask requirement.
    // Standard MirCat_Bitwise Formula: (offset + align - 1) & ~(align - 1)
    // Example: offset = 4, alignmentBytes = 8 -> (4 + 7) & ~7 -> 11 & 0xFFFFFFF8 = 8.
    m_currentStackOffset = (m_currentStackOffset + alignmentBytes - 1) & ~(alignmentBytes - 1);
    int64_t assignedOffset = m_currentStackOffset;

    // Advance the frame cursor state by the allocated type footprint size
    m_currentStackOffset += static_cast<int64_t>(sizeBytes);
    return assignedOffset;
}
