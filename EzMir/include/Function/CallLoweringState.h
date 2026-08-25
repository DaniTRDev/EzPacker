#ifndef EZMIR_CALL_LOWERING_STATE_H
#define EZMIR_CALL_LOWERING_STATE_H

#include "EzMirCommon.h"
#include "Operand/MirRegisterReference.h"

/**
 * State machine tracking register and stack allocations during ABI call/argument lowering.
 *
 * Maintains the sequence of caller-saved registers consumed so far across parameter slots,
 * delegating spill parameters to the function's stack frame.
 */
class CallLoweringState
{
  public:
    /**
     * Constructs a lowering state machine initialized with caller-saved registers from the calling convention.
     */
    CallLoweringState(class CallingConvDesc *cc, class MirBuilderContext *ctx, class MirFunction *func);

    /**
     * Attempts to allocate the next free physical register belonging to the requested register class.
     * Returns true on success and writes the register reference to outReg; returns false if exhausted.
     */
    bool allocate(class MirRegisterClass *_class, MirRegisterRef &reg);

    /**
     * Returns the number of unallocated usable registers remaining in the specified register class.
     */
    size_t getUsableRegCount(class MirRegisterClass *_class) const;

    /**
     * Returns the number of registers already allocated in the specified register class during this lowering.
     */
    size_t getUsedRegCount(class MirRegisterClass *_class) const;

    /**
     * Allocates a parameter slot in the target function's stack frame.
     */
    class StackFrameObject *allocateStack(class MirType *type) const;

  private:
    /**
     * Calling convention providing ABI classification rules.
     */
    class CallingConvDesc *m_callingConv;

    /**
     * Target function undergoing call lowering.
     */
    class MirFunction *m_func;

    /**
     * Registers allocated so far, partitioned by register class.
     */
    std::pmr::unordered_map<class MirRegisterClass *, std::pmr::vector<MirRegisterRef>> m_allocatedRegs;

    /**
     * Usable register pool available for arguments, partitioned by register class.
     */
    std::pmr::unordered_map<class MirRegisterClass *, std::pmr::vector<MirRegisterRef>> m_usableRegs;
};

#endif // EZMIR_CALL_LOWERING_STATE_H
