/**
 * @file WhileParser.h
 * @brief Parser for `while` loops: `while (condition) { body }`.
 *
 * Consumes the `while` keyword, a parenthesised condition, and a
 * brace-delimited body scope.  Produces a WhileAstNode.
 */
#ifndef EZPACKER_WHILEPARSER_H
#define EZPACKER_WHILEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodeParsers/ParserBatch.h"
#include "AstNodeParsers/Parsers/CodeScopeParser.h"
#include "AstNodeParsers/Parsers/ConditionParser.h"

class WhileParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a while block.
     * @param ctx
     * @return WhileAstNode *
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_WHILEPARSER_H
