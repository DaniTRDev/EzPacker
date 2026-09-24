#ifndef EZMIR_CALL_LOWERING_STATE_H
#define EZMIR_CALL_LOWERING_STATE_H

#include "EzMirCommon.h"
#include "Operand/MirRegisterReference.h"
#include <string_view>

/**
 * Transparent hash/equality for bank-name keys, letting cursors be queried with a string_view
 * without building a temporary std::string.
 */
struct BankCursorHash
{
    using is_transparent = void;

    size_t operator()(std::string_view value) const noexcept { return std::hash<std::string_view>{}(value); }
    size_t operator()(const std::string &value) const noexcept { return operator()(std::string_view(value)); }
};

struct BankCursorEqual
{
    using is_transparent = void;

    bool operator()(std::string_view lhs, std::string_view rhs) const noexcept { return lhs == rhs; }
};

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
     * Allocates a parameter slot in the target function's stack frame.
     */
    class StackFrameObject *allocateStack(class MirType *type) const;

    /**
     * Returns the CallingConvDesc associated with this lowering state.
     */
    class CallingConvDesc *getCallingConv() const { return m_callingConv; }

    /**
     * Returns the target MirFunction being lowered.
     */
    class MirFunction *getFunction() const { return m_func; }

    /**
     * Returns the builder context.
     */
    class MirBuilderContext *getContext() const { return m_ctx; }

    /**
     * Current parameter slot index (for slot-based calling conventions, e.g. Win64).
     */
    size_t getSlotIndex() const { return m_slotIndex; }
    /**
     * Moves to the next parameter slot index.
     */
    void advanceSlot() { ++m_slotIndex; }

    /**
     * Moves to the next logical argument index.
     */
    void advanceArg() { ++m_argIndex; }

    /**
     * Returns the current cursor index within a named register bank.
     */
    size_t getBankCursor(std::string_view bank) const;

    /**
     * Advances the cursor for a named register bank by one.
     */
    void advanceBankCursor(std::string_view bank);

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
     * Builder context owning allocator and symbols.
     */
    class MirBuilderContext *m_ctx;

    /**
     * Current parameter slot index.
     */
    size_t m_slotIndex{ 0 };

    /**
     * Current logical argument index.
     */
    size_t m_argIndex{ 0 };

    /**
     * Cursors for named register banks (e.g. "integer", "float").
     */
    std::pmr::unordered_map<std::string, size_t, BankCursorHash, BankCursorEqual> m_bankCursors;

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
