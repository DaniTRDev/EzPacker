#ifndef EZPACKER_CALLINGCONVDESC_H
#define EZPACKER_CALLINGCONVDESC_H

#include "EzMirCommon.h"
#include "ArgumentLocationDesc.h"
#include "Type/MirType.h"

/**
 * Class used as a book to know where function arguments and returns should be placed. Since this information CAN'T be
 * set statically, its methods also need a "CallLoweringState" pointer.
 *
 * Example of why it is needed:
 * Imagine 1 integer arg: The CallingConvDesc would ask the CallLoweringState how many integer
 * registers are currently used, if less than available a register location will be return; if no integer register is
 * available, a stack location will be returned.
 */
class CallingConvDesc
{
  public:
    /**
     * Returns the location of an argument of the given type supposing the current call state. This ensure reusing the
     * maximum numbers of registers and to correctly keep track of call parameters.
     * @param type
     * @param callState
     * @return
     */
    virtual ArgumentLocationDesc getArgLoc(MirType *type, class CallLoweringState *callState) = 0;

    /**
     * Returns the location of where the result of a function should be placed. Depends on the call state.
     * @param type
     * @param callState
     * @return
     */
    virtual ArgumentLocationDesc getReturnLoc(MirType *type, class CallLoweringState *callState) = 0;

    /**
     * Returns true if the given type can be returned in register(s). This is useful because in the same target,
     * some ABIs allow returning upto 128 bits in 2 registers (System_V) and others just allow returning 64 bits
     * (Windows).
     *
     * If this returns false, a SRET should be used (struct return, meaning caller allocates space for the return, pass
     * it as a parameter to the callee and the callee writes into it during its execution).
     * @param type
     * @return
     */
    virtual bool canReturnInRegs(MirType *type) const = 0;

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
    virtual RegisterRef getFramePointerReg() const = 0;

    /**
     * Returns the stack pointer register used by this calling convention (e.g., RSP / SP).
     */
    virtual RegisterRef getStackPointerReg() const = 0;

    /**
     * Returns the stack alignment needed BEFORE a call.
     * @return
     */
    virtual size_t getStackAlignment() const = 0;

    /**
     * Returns the shadown space needed BEFORE a call.
     * @return
     */
    virtual size_t getShadowSpaceSize() const = 0;

    /**
     * Returns true if frame pointers (RBP/FP) are required for the given function.
     */
    virtual bool hasFramePointer(class MirFunction *func) const = 0;

    /**
     * Returns EVERY register that must be preserved by the callee.
     * @return
     */
    virtual const std::pmr::vector<RegisterRef> &getAllCalleeSavedRegs() = 0;

    /**
     * Returns the list of registers of the given class that must be preserved by the callee.
     * @return
     */
    virtual const std::pmr::vector<RegisterRef> &getCalleeSavedRegs(MirRegisterClass *_class) = 0;

    /**
     * Returns EVERY register that needs to be preserved by the caller.
     * @return
     */
    virtual const std::pmr::vector<RegisterRef> &getAllCallerSavedRegs() = 0;

    /**
     * Returns the list of registers of the given class that needs to be preserved by the caller.
     * @return
     */
    virtual const std::pmr::vector<RegisterRef> &getCallerSavedRegs(MirRegisterClass *_class) = 0;
};

#endif // EZPACKER_CALLINGCONVDESC_H
