/**
 * @file VariableParser.h
 * @brief Parser for `%name` references and variable declarations.
 *
 * Accepted source forms:
 * - `%name`
 * - `type %name`
 * - `type %name: immediate`
 * - `type %name: { immediate[, immediate...] }`
 *
 * Notes:
 * - The parser requires the leading `%` before the variable name.
 * - Initializers are limited to immediate expressions.
 * - Semantic meaning (parameter, local, global, symbol use) is not decided
 *   here; only syntax is recorded.
 */
#ifndef EZPACKER_VARIABLEPARSER_H
#define EZPACKER_VARIABLEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/Variable.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "ImmediateParser.h"

class VariableParser : public IAstNodeParser
{
  public:
    /**
     * Parses a Variable node from the current token position.
     *
     * The parser accepts an optional type identifier before `%name`. If a colon
     * follows the name, it expects either a single immediate initializer or a
     * brace-delimited list of immediates.
     */
    AstNode* parse(const std::shared_ptr<BasicParsingContext> &ctx);
};

#endif // EZPACKER_VARIABLEPARSER_H
