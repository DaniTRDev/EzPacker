#ifndef EZPACKER_PARSING_H
#define EZPACKER_PARSING_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * Performs the parsing step of the compilation process. This involves taking the tokens produced by the tokenizer
 * and constructing an abstract syntax tree (AST) or similar intermediate representation. If parsing fails, it
 * returns false. Otherwise, it returns true.
 */
class ParsingPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * Performs the parsing of the tokens produced in tokenization.
     * @param unit
     * @return bool
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * Returns "ParsingPhase".
     * @return const char *
     */
    const char *getName() override;
};

#endif // EZPACKER_PARSING_H
