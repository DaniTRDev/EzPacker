#ifndef EZPACKER_CALLLOWERINGSTATE_H
#define EZPACKER_CALLLOWERINGSTATE_H

#include "EzMirCommon.h"
#include "CallingConvDesc.h"
#include "Builder/MirBuilderContext.h"
#include "Operand/MirOperands.h"

class CallLoweringState
{
  public:
    /**
     * Creates the state with the given calling convention and context.
     */
    CallLoweringState(CallingConvDesc *cc, MirBuilderContext *ctx, MirFunction *func);

    /**
     * Attempts to allocate the next available register of the given class. Returns true if succeded and false if
     * no register is available.
     */
    bool allocate(MirRegisterClass *_class, RegisterRef &reg);

    /**
     * Returns the count of available registers of the given class type for the call.
     * @return
     */
    size_t getUsableRegCount(MirRegisterClass *_class) const;

    /**
     * Returns the count of used registers of the given class type for the call so far.
     * @return
     */
    size_t getUsedRegCount(MirRegisterClass *_class) const;

    /**
     * Allocates an abstract stack slot in the target function.
     */
    StackFrameObject *allocateStack(MirType *type) const;

  private:
    CallingConvDesc *m_callingConv;
    MirFunction *m_func;

    // Record of registers allocated during this lowering state
    std::pmr::unordered_map<MirRegisterClass *, std::pmr::vector<RegisterRef>> m_allocatedRegs;

    // Record of usable registers by this call lowering state machine.
    std::pmr::unordered_map<MirRegisterClass *, std::pmr::vector<RegisterRef>> m_usableRegs;
};

#endif // EZPACKER_CALLLOWERINGSTATE_H
