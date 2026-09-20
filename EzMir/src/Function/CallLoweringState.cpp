#include "Function/CallLoweringState.h"
#include "Builder/MirBuilderContext.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Operand/MirRegisterClass.h"

/**
 * Initializes the lowering state and seeds the usable register pool with the calling convention's
 * caller-saved registers, grouped by register class. Falls back to the default resource when no
 * builder context is supplied.
 */
CallLoweringState::CallLoweringState(CallingConvDesc *cc, MirBuilderContext *ctx, MirFunction *func) :
    m_callingConv(cc), m_func(func), m_ctx(ctx),
    m_bankCursors(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),
    m_allocatedRegs(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),
    m_usableRegs(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource())
{
    if (cc)
    {
        const auto &callerSavedRegs = cc->getAllCallerSavedRegs();
        for (const auto &reg : callerSavedRegs)
        {
            m_usableRegs[reg.getClass()].push_back(reg);
        }
    }
}

/**
 * Hands out the next unused register of the requested class by index, recording it as allocated.
 * Returns false when the class is unknown or all of its registers are exhausted.
 */
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

/**
 * Creates a stack-passed parameter slot of the given type in the target function's frame.
 */
StackFrameObject *CallLoweringState::allocateStack(MirType *type) const
{
    return m_func->getStackFrame()->createStackParam(type);
}

/**
 * Returns the current cursor for the named register bank, or 0 if the bank has never advanced.
 */
size_t CallLoweringState::getBankCursor(std::string_view bank) const
{
    auto it = m_bankCursors.find(bank);
    return (it != m_bankCursors.end()) ? it->second : 0;
}

/**
 * Advances the named register bank's cursor, creating it at zero first when absent.
 */
void CallLoweringState::advanceBankCursor(std::string_view bank) { m_bankCursors[std::string(bank)]++; }