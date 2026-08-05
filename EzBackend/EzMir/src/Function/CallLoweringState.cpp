#include "Function/CallLoweringState.h"

CallLoweringState::CallLoweringState(CallingConvDesc *cc, MirBuilderContext *ctx) :
    m_callingConv(cc), m_currentStackOffset(0), m_allocatedRegs(ctx->getGlobalAllocator()),
    m_usableRegs(ctx->getGlobalAllocator())

{
    for (size_t i = static_cast<uint8_t>(RegisterRefClass::Invalid) + 1;
         i < static_cast<uint8_t>(RegisterRefClass::MAX_REF_TYPE);
         i++)
    {
        const auto &callerSavedRegs = cc->getCallerSavedRegs(static_cast<RegisterRefClass>(i));
        m_usableRegs[static_cast<RegisterRefClass>(i)] = callerSavedRegs;
    }
}

bool CallLoweringState::allocate(RegisterRefClass regClass, RegisterRef &outReg)
{
    auto it = m_usableRegs.find(regClass);
    if (it == m_usableRegs.end() || it->second.empty())
    {
        return false;
    }

    auto &pool = it->second;
    outReg = pool.front();

    // Shift allocated register out of usable pool into allocated pool
    pool.erase(pool.begin());
    m_allocatedRegs[regClass].push_back(outReg);

    return true;
}

size_t CallLoweringState::getUsableRegCount(RegisterRefClass regClass) const
{
    auto it = m_usableRegs.find(regClass);
    return (it != m_usableRegs.end()) ? it->second.size() : 0;
}

size_t CallLoweringState::getUsedRegCount(RegisterRefClass regClass) const
{
    auto it = m_allocatedRegs.find(regClass);
    return (it != m_allocatedRegs.end()) ? it->second.size() : 0;
}

int64_t CallLoweringState::getStackOffset() const { return m_currentStackOffset; }

int64_t CallLoweringState::allocateStackSlot(size_t sizeBytes, size_t alignmentBytes)
{
    if (sizeBytes == 0)
        return m_currentStackOffset;

    // Align stack offset to requested byte alignment boundary
    if (alignmentBytes > 1)
    {
        m_currentStackOffset = (m_currentStackOffset + static_cast<int64_t>(alignmentBytes) - 1) &
                ~(static_cast<int64_t>(alignmentBytes) - 1);
    }

    int64_t assignedOffset = m_currentStackOffset;
    m_currentStackOffset += static_cast<int64_t>(sizeBytes);

    return assignedOffset;
}