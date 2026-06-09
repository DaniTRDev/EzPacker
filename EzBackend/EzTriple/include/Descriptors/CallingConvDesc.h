#ifndef EZPACKER_CALLINGCONVDESC_H
#define EZPACKER_CALLINGCONVDESC_H

#include "EzTripleCommon.h"
#include "ArgumentLocationDesc.h"
#include "CallLoweringState.h"

/**
 * Class used as a book to know where function arguments are mapped. Since this information CAN'T be set statically,
 * its methods also need a "CallLoweringState" pointer.
 *
 * Example of way it is needed:
 * Imagine 1 integer arg: The CallingConvDesc would ask the MirFunctionCallingConvInfo how many integer
 * registers are currently used, if less than available a register location will be return; if no integer register is
 * available, it a stack location will be returned.
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

  private:
};

#endif // EZPACKER_CALLINGCONVDESC_H
