#ifndef EZPACKER_SEMANTICANALYSIS_H
#define EZPACKER_SEMANTICANALYSIS_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * Performs the semantic analysis step of the compilation process. This involves checking the AST for semantic
 * errors, such as type errors, undefined variables, and other issues that cannot be detected during parsing. If
 * semantic analysis fails, it returns false. Otherwise, it returns true.
 */
class SemanticAnalysisPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * Performs the semantic analysis of the generated AST in the parsing.
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

#endif // EZPACKER_SEMANTICANALYSIS_H
