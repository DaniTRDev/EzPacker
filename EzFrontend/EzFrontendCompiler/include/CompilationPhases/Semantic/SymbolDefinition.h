#ifndef EZPACKER_SYMBOLDEFINITION_H
#define EZPACKER_SYMBOLDEFINITION_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * Performs the definition of the symbols in the generated AST in the parsing.
 * This phase is responsible for defining the symbols in the symbol table, which will be used in the subsequent phases
 * of the compilation.
 */
class SymbolDefinitionPhase : public FrontendCompilationUnitPhase
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

#endif // EZPACKER_SYMBOLDEFINITION_H
