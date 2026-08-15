#include "Function/CallLoweringState.h"

CallLoweringState::CallLoweringState(CallingConvDesc *cc, MirBuilderContext *ctx, MirFunction *func) :
    m_callingConv(cc), m_func(func), m_allocatedRegs(ctx->getGlobalAllocator()), m_usableRegs(ctx->getGlobalAllocator())

{
    const auto &callerSavedRegs = cc->getAllCallerSavedRegs();
    for (const auto &reg : callerSavedRegs)
    {
        m_usableRegs[reg.getClass()].push_back(reg);
    }
}

bool CallLoweringState::allocate(MirRegisterClass *_class, RegisterRef &outReg)
{
    auto it = m_usableRegs.find(_class);
    if (it == m_usableRegs.end() || it->second.empty())
    {
        return false;
    }

    auto &pool = it->second;
    outReg = pool.front();

    // Shift allocated register out of usable pool into allocated pool
    pool.erase(pool.begin());
    m_allocatedRegs[_class].push_back(outReg);

    return true;
}

size_t CallLoweringState::getUsableRegCount(MirRegisterClass *_class) const
{
    auto it = m_usableRegs.find(_class);
    return (it != m_usableRegs.end()) ? it->second.size() : 0;
}

size_t CallLoweringState::getUsedRegCount(MirRegisterClass *_class) const
{
    auto it = m_allocatedRegs.find(_class);
    return (it != m_allocatedRegs.end()) ? it->second.size() : 0;
}

StackFrameObject *CallLoweringState::allocateStack(MirType *type) const
{
    return m_func->getStackFrame()->createStackParam(type);
}