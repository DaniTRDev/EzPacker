#ifndef EZMIR_CALLING_CONV_DESC_H
#define EZMIR_CALLING_CONV_DESC_H

#include "EzMirCommon.h"
#include "ArgumentLocationDesc.h"

/**
 * ABI classification categories for argument and return value coercion.
 */
enum class CallingConvTypeClass : uint8_t
{
    Integer = 0, // Passed or returned via general-purpose integer registers (e.g. System V INTEGER)
    Float,       // Passed or returned via floating-point/vector registers (e.g. System V SSE)
    Memory,      // Passed or returned on the stack memory
    Hfa,         // Homogeneous Floating-point Aggregate (passed in consecutive vector registers)
    ByRef        // Large composite passed indirectly by pointer to caller-allocated copy
};

/**
 * Abstract interface defining target Calling Convention and ABI rules.
 * Governs argument/return placement, caller vs callee register saving conventions,
 * stack alignment, shadow space, and stack frame pointer requirements.
 */
class CallingConvDesc
{
  public:
    virtual ~CallingConvDesc() = default;

    /**
     * Returns the size of the stack red zone in bytes (e.g. 128 bytes on SysV AMD64, 0 on Win64).
     */
    virtual size_t getRedZoneSize() const { return 0; }

    /**
     * Returns the hardware link register reference if this ABI uses one (e.g. LR/X30 on AArch64).
     */
    virtual std::optional<MirRegisterRef> getLinkRegister() const { return std::nullopt; }

    /**
     * Returns true if an implicit struct return (sret) pointer consumes a normal argument slot (e.g. Win64 RCX).
     */
    virtual bool consumesSretSlot() const { return false; }

    /**
     * Computes the concrete argument location (register, stack, split, or indirect) for the given type and call state.
     */
    virtual ArgumentLocationDesc getArgLoc(class MirType *type, class CallLoweringState *callState) = 0;

    /**
     * Computes the return value location for the given type and call state.
     */
    virtual ArgumentLocationDesc getReturnLoc(class MirType *type, class CallLoweringState *callState) = 0;

    /**
     * Returns true if the given type fits within return registers without requiring an implicit SRET pointer.
     */
    virtual bool canReturnInRegs(class MirType *type) const = 0;

    /**
     * Returns true if the callee cleans up stack arguments (stdcall/thiscall), or false for caller-cleanup (cdecl/SysV).
     */
    virtual bool isCalleeCleanup() const = 0;

    /**
     * Returns true if the architecture stack grows downwards toward lower memory addresses.
     */
    virtual bool doesStackGrowsDownwards() const = 0;

    /**
     * Returns the name of the calling convention (e.g. "SystemV_AMD64", "Win64").
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the frame pointer hardware register reference (e.g. RBP/EBP/FP).
     */
    virtual MirRegisterRef getFramePointerReg() const = 0;

    /**
     * Returns the stack pointer hardware register reference (e.g. RSP/ESP/SP).
     */
    virtual MirRegisterRef getStackPointerReg() const = 0;

    /**
     * Returns the required stack alignment boundary in bytes before a call instruction (e.g. 16 bytes).
     */
    virtual size_t getStackAlignment() const = 0;

    /**
     * Returns the mandatory shadow / home space in bytes allocated before a call (e.g. 32 bytes on Win64).
     */
    virtual size_t getShadowSpaceSize() const = 0;

    /**
     * Returns true if the function requires a dedicated frame pointer (e.g. dynamic alloca, variable stack).
     */
    virtual bool hasFramePointer(class MirFunction *func) const = 0;

    /**
     * Classifies a type into its constituent ABI type classes for argument/return passing.
     */
    virtual void classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const = 0;

    /**
     * Returns all callee-saved (non-volatile) registers that must be preserved across function calls.
     */
    virtual const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() = 0;

    /**
     * Returns callee-saved registers filtered by the specified register class.
     */
    virtual const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(class MirRegisterClass *_class) = 0;

    /**
     * Returns all caller-saved (volatile / scratch) registers that can be used for parameters or destroyed by calls.
     */
    virtual const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() = 0;

    /**
     * Returns caller-saved registers filtered by the specified register class.
     */
    virtual const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(class MirRegisterClass *_class) = 0;
};

#endif // EZMIR_CALLING_CONV_DESC_H
