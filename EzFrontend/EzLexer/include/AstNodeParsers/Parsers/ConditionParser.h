/**
 * @file ConditionParser.h
 * @brief Parser for comparison conditions: `(%lhs OP %rhs)`.
 *
 * Parses a parenthesised binary comparison expression used inside `if` and
 * `while` constructs.  The left and right sides are variables and the
 * operator is one of EQ, NE, GT, GE, LT, LE.  Produces a ConditionAstNode.
 */
#ifndef EZPACKER_CONDITIONPARSER_H
#define EZPACKER_CONDITIONPARSER_H

#include "EzLexerCommon.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/ParserBatch.h"
#include "AstNodeParsers/Parsers/VariableParser.h"
#include "AstNodes/WhileAstNode.h"

class ConditionParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a condition.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_CONDITIONPARSER_H
