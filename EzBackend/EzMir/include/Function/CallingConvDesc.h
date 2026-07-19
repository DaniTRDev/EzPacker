#ifndef EZPACKER_CALLINGCONVDESC_H
#define EZPACKER_CALLINGCONVDESC_H

#include "EzMirCommon.h"
#include "ArgumentLocationDesc.h"
#include "CallLoweringState.h"
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
    virtual ArgumentLocationDesc getArgLoc(MirType *type, CallLoweringState *callState) = 0;

    /**
     * Returns the location of where the result of a function should be placed. Depends on the call state.
     * @param type
     * @param callState
     * @return
     */
    virtual ArgumentLocationDesc getReturnLoc(MirType *type, CallLoweringState *callState) = 0;

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
     * Returns the name of the calling convention.
     */
    virtual const char *getName() const = 0;

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
     * Returns the list of GPR registers that must be preserved by the callee.
     * @return
     */
    virtual const std::vector<PhysicalRegId> &getCalleeSavedGPRegs() const = 0;

    /**
     * Returns the list of FPR registers that must be preserved by the callee.
     * @return
     */
    virtual const std::vector<PhysicalRegId> &getCalleeSavedFPRegs() const = 0;

    /**
     * Returns the list of GPR registers that needs to be preserved by the caller.
     * @return
     */
    virtual const std::vector<PhysicalRegId> &getCallerSavedGPRegs() const = 0;

    /**
     * Returns the list of FPR registers that needs to be preserved by the caller.
     * @return
     */
    virtual const std::vector<PhysicalRegId> &getCallerSavedFPRegs() const = 0;
};

#endif // EZPACKER_CALLINGCONVDESC_H
