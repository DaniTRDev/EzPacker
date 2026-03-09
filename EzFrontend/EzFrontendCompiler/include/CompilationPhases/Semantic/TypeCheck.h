#ifndef EZPACKER_TYPECHECK_H
#define EZPACKER_TYPECHECK_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * Checks that the used types matches the expected types in the generated AST.
 * For example, if a function expects an integer parameter, it checks that the provided argument is indeed an integer.
 * It also checks for type compatibility in expressions, ensuring that operations are performed on compatible types and
 * realizing a cast if necessary.
 */
class TypeCheckPhase : public FrontendCompilationUnitPhase
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

#endif // EZPACKER_TYPECHECK_H
