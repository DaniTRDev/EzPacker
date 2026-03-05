#ifndef EZPACKER_TOKENIZATION_H
#define EZPACKER_TOKENIZATION_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * Performs the tokenization step of the compilation process. This involves taking the source content and breaking
 * it down into a sequence of tokens that can be used for parsing. If tokenization fails, it returns false.
 * Otherwise, it returns true.
 */
class TokenizationPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * Performs the tokenization of the given source code.
     * @param unit
     * @return bool
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * Returns "TokenizationPhase"
     * @return const char *
     */
    const char *getName() override;
};

#endif // EZPACKER_TOKENIZATION_H
