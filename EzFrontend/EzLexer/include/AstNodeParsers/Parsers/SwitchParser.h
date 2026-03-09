/**
 * @file SwitchParser.h
 * @brief Parser for `switch` statements.
 *
 * Accepted grammar:
 *
 * `switch (%var) {`
 * `    case immediate: { ... }`
 * `    case immediate: { ... }`
 * `    default: { ... }`
 * `}`
 *
 * Behavior notes:
 * - The switch selector must be parsed as a Variable node.
 * - Each `case` value must be an ImmediateOperand.
 * - Case bodies and the optional default body are full CodeScope nodes.
 * - At most one `default` case is accepted.
 */
#ifndef EZPACKER_SWITCHPARSER_H
#define EZPACKER_SWITCHPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/SwitchAstNode.h"
#include "AstNodes/SwitchCaseAstNode.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodeParsers/ParserBatch.h"
#include "AstNodeParsers/Parsers/CodeScopeParser.h"
#include "AstNodeParsers/Parsers/ImmediateParser.h"

class SwitchParser : public IAstNodeParser
{
  public:
    /**
     * Parses a switch statement from the current token position.
     *
     * If the current token is not `switch`, the parser returns nullptr without
     * emitting a hard failure so it can safely participate in ParserBatch.
     * Once `switch` has been consumed, the rest of the grammar is treated as
     * mandatory and malformed input becomes a fatal parse error.
     *
     * On success, returns a SwitchAstNode that contains:
     * - the selector variable,
     * - zero or more SwitchCaseAstNode entries,
     * - an optional default CodeScope.
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_SWITCHPARSER_H
