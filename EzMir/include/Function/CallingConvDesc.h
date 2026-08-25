#ifndef EZMIR_CALLING_CONV_DESC_H
#define EZMIR_CALLING_CONV_DESC_H

#include "EzMirCommon.h"
#include "ArgumentLocationDesc.h"

/**
 * This enum is used to know which class a type has. It makes easier coercing types used for aggregate
 * returning-passing: SysV -> MEMORY > INTEGER > FLOAT. struct { int a (INTEGER); float b; (FLOAT) } -> the resulting
 * type class IS INTEGER with a total size on 8 bytes, making it be returned/passed a single GPR64.
 */
enum class CallingConvTypeClass : uint8_t
{
    Integer = 0,
    Float,
    Memory,
    Hfa,  // For aggregates that contains elements of the same type: struct { float a; float b; float c; float d;} ->
          // return in a vec register.
    ByRef // For types bigger thank X bytes in stack, copy the original content there and pass the new pointer to the
          // callee.
};

/**
 * Class used as a book to know where function arguments and returns should be placed. Since this information CAN'T be
 * known statically, its methods also need a "CallLoweringState" pointer.
 *
 * Example of why it is needed:
 * Imagine 1 integer arg: The CallingConvDesc would ask the CallLoweringState how many integer
 * registers are currently used, if less than available a register location will be return; if no integer register is
 * available, a stack location will be returned.
 *
 * This is the most clear reason, but there's a LOT of obfuscure rules (mostly in x86) that need an object to keep track
 * of the calling state.
 */
class CallingConvDesc
{
  public:
    /**
     * Returns the location of an argument of the given type supposing the current call state. This ensure reusing the
     * maximum numbers of registers and to correctly keep track of call parameters.
     */
    virtual ArgumentLocationDesc getArgLoc(class MirType *type, class CallLoweringState *callState) = 0;

    /**
     * Returns the location of where the result of a function should be placed. Depends on the call state.
     */
    virtual ArgumentLocationDesc getReturnLoc(class MirType *type, class CallLoweringState *callState) = 0;

    /**
     * Returns true if the given type can be returned in register(s). This is useful because in the same target,
     * some ABIs allow returning upto 128 bits in 2 registers (System_V) and others just allow returning 64 bits
     * (Windows).
     *
     * If this returns false, a SRET should be used (struct return, meaning caller allocates space for the return, pass
     * it as a parameter to the callee and the callee writes into it during its execution).
     */
    virtual bool canReturnInRegs(class MirType *type) const = 0;

    /**
     * Returns true if the callee is responsible for cleaning up stack arguments (e.g., stdcall).
     * Returns false if the caller cleans up the stack (e.g., cdecl, SysV).
     */
    virtual bool isCalleeCleanup() const = 0;

    /**
     * Returns true if stack grows downwards.
     */
    virtual bool doesStackGrowsDownwards() const = 0;

    /**
     * Returns the name of the calling convention.
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the frame pointer register used by this calling convention (e.g., RBP / FP).
     */
    virtual MirRegisterRef getFramePointerReg() const = 0;

    /**
     * Returns the stack pointer register used by this calling convention (e.g., RSP / SP).
     */
    virtual MirRegisterRef getStackPointerReg() const = 0;

    /**
     * Returns the stack alignment needed BEFORE a call.
     */
    virtual size_t getStackAlignment() const = 0;

    /**
     * Returns the shadown space needed BEFORE a call.
     */
    virtual size_t getShadowSpaceSize() const = 0;

    /**
     * Returns true if frame pointers (RBP/FP) are required for the given function.
     */
    virtual bool hasFramePointer(class MirFunction *func) const = 0;

    /**
     * Classifies the given type in any of the calling convention classes. If an aggregate is given, more than 1 type is
     * returned.
     */
    virtual void classify(MirType *type, std::pmr::vector<CallingConvTypeClass> &out) const = 0;

    /**
     * Returns EVERY register that must be preserved by the callee.
     */
    virtual const std::pmr::vector<MirRegisterRef> &getAllCalleeSavedRegs() = 0;

    /**
     * Returns the list of registers of the given class that must be preserved by the callee.
     */
    virtual const std::pmr::vector<MirRegisterRef> &getCalleeSavedRegs(class MirRegisterClass *_class) = 0;

    /**
     * Returns EVERY register that needs to be preserved by the caller.
     */
    virtual const std::pmr::vector<MirRegisterRef> &getAllCallerSavedRegs() = 0;

    /**
     * Returns the list of registers of the given class that needs to be preserved by the caller.
     */
    virtual const std::pmr::vector<MirRegisterRef> &getCallerSavedRegs(class MirRegisterClass *_class) = 0;
};

#endif // EZMIR_CALLING_CONV_DESC_H
