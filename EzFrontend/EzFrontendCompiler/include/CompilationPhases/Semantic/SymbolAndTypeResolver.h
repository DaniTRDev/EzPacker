#ifndef EZPACKER_SYMBOLANDTYPERESOLVER_H
#define EZPACKER_SYMBOLANDTYPERESOLVER_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * Performs the resolution of used symbols and types in the AST. This phase should be executed after the symbol
 * definition phase, as it relies on the symbols being defined in order to resolve them.
 */
class SymbolAndTypeResolverPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * Performs the the definition of the symbols of the AST.
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

#endif // EZPACKER_SYMBOLANDTYPERESOLVER_H
