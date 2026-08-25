#include "Function/CallLoweringState.h"
#include "Builder/MirBuilderContext.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirRegisterClass.h"

CallLoweringState::CallLoweringState(CallingConvDesc *cc, MirBuilderContext *ctx, MirFunction *func) :
    m_callingConv(cc), m_func(func), m_allocatedRegs(ctx->getGlobalAllocator()), m_usableRegs(ctx->getGlobalAllocator())
{
    const auto &callerSavedRegs = cc->getAllCallerSavedRegs();
    for (const auto &reg : callerSavedRegs)
    {
        m_usableRegs[reg.getClass()].push_back(reg);
    }
}

bool CallLoweringState::allocate(MirRegisterClass *_class, MirRegisterRef &outReg)
{
    auto it = m_usableRegs.find(_class);
    if (it == m_usableRegs.end())
    {
        return false;
    }

    const auto &pool = it->second;
    auto &allocated = m_allocatedRegs[_class];
    size_t cursor = allocated.size();

    // Check if we exhausted all available registers for this class
    if (cursor >= pool.size())
    {
        return false;
    }

    // O(1) index lookup
    outReg = pool[cursor];
    allocated.push_back(outReg);

    return true;
}

size_t CallLoweringState::getUsableRegCount(MirRegisterClass *_class) const
{
    auto itUsable = m_usableRegs.find(_class);
    if (itUsable == m_usableRegs.end())
    {
        return 0;
    }

    size_t total = itUsable->second.size();
    size_t used = getUsedRegCount(_class);

    return (total > used) ? (total - used) : 0;
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