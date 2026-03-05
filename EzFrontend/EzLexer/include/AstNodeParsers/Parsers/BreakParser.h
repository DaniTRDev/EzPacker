/**
 * @file BreakParser.h
 * @brief Parser for the `break;` statement.
 *
 * Consumes the `break` keyword followed by a semicolon and produces a
 * BreakAstNode.  Returns nullptr (soft error) if the current token is not
 * `break`, or emits a fatal error if the semicolon is missing.
 */
#ifndef EZPACKER_BREAKPARSER_H
#define EZPACKER_BREAKPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/BreakAstNode.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"

class BreakParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse the given break statement. If the current token is not a break statement, it returns nullptr.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_BREAKPARSER_H
