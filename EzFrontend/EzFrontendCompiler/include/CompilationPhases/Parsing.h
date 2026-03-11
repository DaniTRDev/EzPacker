/**
 * @file Parsing.h
 * @brief Compilation phase for parsing source code.
 *
 * The ParsingPhase is responsible for transforming the stream of tokens produced
 * by the TokenizationPhase into an Abstract Syntax Tree (AST). It utilizes the
 * parsers provided by EzLexer to build the AST structure.
 */
#ifndef EZPACKER_PARSING_H
#define EZPACKER_PARSING_H

#include "EzFrontendCompilerCommon.h"
#include "FrontendCompilationUnitPhase.h"

/**
 * @class ParsingPhase
 * @brief Parses tokens into an Abstract Syntax Tree (AST).
 *
 * This phase consumes the token stream from the `FrontendCompilationUnit` and
 * invokes the parsing logic (via `BasicParsingContext` and `ParserBatch`) to
 * construct the AST.
 *
 * If parsing fails (e.g., syntax errors), this phase reports errors and returns `false`.
 */
class ParsingPhase : public FrontendCompilationUnitPhase
{
  public:
    /**
     * @brief Executes the parsing phase.
     *
     * Initializes the parsing context and runs the parser batch on the unit's token stream.
     * Populates the unit's AST upon success.
     *
     * @param unit Pointer to the compilation unit to parse.
     * @return `true` if parsing succeeded; `false` otherwise.
     */
    bool execute(class FrontendCompilationUnit *unit) override;

    /**
     * @brief Gets the phase name.
     *
     * @return "ParsingPhase"
     */
    const char *getName() override;
};

#endif // EZPACKER_PARSING_H
