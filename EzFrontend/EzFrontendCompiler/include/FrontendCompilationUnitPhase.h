#ifndef EZPACKER_FRONTENDCOMPILATIONUNITPHASE_H
#define EZPACKER_FRONTENDCOMPILATIONUNITPHASE_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnit.h"

class FrontendCompilationUnitPhase
{
  public:
    /**
     * Peforms a task in the compilation unit. Returns true if succeeded. Otherwise, returns false.
     * @param unit
     * @return bool
     */
    virtual bool execute(class FrontendCompilationUnit *unit) = 0;

    /**
     * Returns the name of the phase.
     * @return const char *
     */
    virtual const char *getName() = 0;

  protected:
    friend class FrontendCompilationUnit;
};

#endif // EZPACKER_FRONTENDCOMPILATIONUNITPHASE_H
