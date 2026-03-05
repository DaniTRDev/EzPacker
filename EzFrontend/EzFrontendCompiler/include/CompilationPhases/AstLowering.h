#ifndef EZPACKER_ASTLOWERING_H
#define EZPACKER_ASTLOWERING_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * Performs the MIR emission step of the compilation process. This involves taking the semantically analyzed AST and
 * generating an intermediate representation (MIR) that can be used for further optimization and code generation. If
 * MIR emission fails, it returns false. Otherwise, it returns true.
 */
class AstLoweringPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * Performs the lowering of the AST.
     * @param unit
     * @return bool
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * Returns "AstLoweringPhase"
     * @return const char *
     */
    const char *getName() override;
};

#endif // EZPACKER_ASTLOWERING_H
