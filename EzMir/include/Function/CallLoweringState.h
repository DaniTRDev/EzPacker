#ifndef EZMIR_CALL_LOWERING_STATE_H
#define EZMIR_CALL_LOWERING_STATE_H

#include "EzMirCommon.h"
#include "Operand/MirRegisterReference.h"

class CallLoweringState
{
  public:
    /**
     * Creates the state with the given calling convention and context.
     */
    CallLoweringState(class CallingConvDesc *cc, class MirBuilderContext *ctx, class MirFunction *func);

    /**
     * Attempts to allocate the next available register of the given class. Returns true if succeded and false if
     * no register is available.
     */
    bool allocate(class MirRegisterClass *_class, MirRegisterRef &reg);

    /**
     * Returns the count of available registers of the given class type for the call.
     * @return
     */
    size_t getUsableRegCount(class MirRegisterClass *_class) const;

    /**
     * Returns the count of used registers of the given class type for the call so far.
     * @return
     */
    size_t getUsedRegCount(class MirRegisterClass *_class) const;

    /**
     * Allocates an abstract stack slot in the target function.
     */
    class StackFrameObject *allocateStack(class MirType *type) const;

  private:
    class CallingConvDesc *m_callingConv;
    class MirFunction *m_func;

    // Record of registers allocated during this lowering state
    std::pmr::unordered_map<class MirRegisterClass *, std::pmr::vector<MirRegisterRef>> m_allocatedRegs;

    // Record of usable registers by this call lowering state machine.
    std::pmr::unordered_map<class MirRegisterClass *, std::pmr::vector<MirRegisterRef>> m_usableRegs;
};

#endif // EZMIR_CALL_LOWERING_STATE_H
