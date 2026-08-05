#include "Function/CallLoweringState.h"

CallLoweringState::CallLoweringState(CallingConvDesc *cc, MirBuilderContext *ctx, MirFunction *func) :
    m_callingConv(cc), m_func(func), m_allocatedRegs(ctx->getGlobalAllocator()), m_usableRegs(ctx->getGlobalAllocator())

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

StackFrameObject *CallLoweringState::allocateStack(MirType *type) const
{
    return m_func->getStackFrame()->createStackParam(type);
}