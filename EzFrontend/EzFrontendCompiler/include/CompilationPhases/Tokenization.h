/**
 * @file Tokenization.h
 * @brief Compilation phase for lexical analysis.
 *
 * The TokenizationPhase is the first step in the compilation pipeline. It reads
 * the raw source code and converts it into a stream of tokens (keywords,
 * identifiers, operators, etc.) using the `BasicTokenizer` from EzLexer.
 */
#ifndef EZPACKER_TOKENIZATION_H
#define EZPACKER_TOKENIZATION_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * @class TokenizationPhase
 * @brief Converts source code into a token stream.
 *
 * This phase initializes the tokenizer for the `FrontendCompilationUnit` and
 * processes the source content. The resulting tokens are stored in the unit
 * for subsequent phases (like parsing).
 *
 * If lexical errors occur (e.g., invalid characters), this phase reports them
 * and returns `false`.
 */
class TokenizationPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * @brief Executes the tokenization phase.
     *
     * Reads the source content from the unit and runs the tokenizer.
     *
     * @param unit Pointer to the compilation unit to tokenize.
     * @return `true` if tokenization succeeded; `false` otherwise.
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * @brief Gets the phase name.
     *
     * @return "TokenizationPhase"
     */
    const char *getName() override;
};

#endif // EZPACKER_TOKENIZATION_H
