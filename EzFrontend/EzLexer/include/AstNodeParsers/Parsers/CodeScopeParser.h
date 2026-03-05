/**
 * @file CodeScopeParser.h
 * @brief Parser for brace-delimited code scopes: `{ … }`.
 *
 * Expects an opening `{`, then repeatedly tries to parse child statements
 * (labels, if/else, while, instructions, break, continue) until the closing
 * `}` is reached.  Produces a CodeScope node containing the ordered list of
 * parsed children.
 */
#ifndef EZPACKER_CODESCOPEPARSER_H
#define EZPACKER_CODESCOPEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/CodeScope.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodeParsers/ParserBatch.h"

// TODO: Fix circular dependency between CodeScopeParser and: InstructionParser, LabelParser, IfParser, WhileParser.
#include "AstNodeParsers/Parsers/BreakParser.h"
#include "AstNodeParsers/Parsers/ContinueParser.h"
#include "AstNodeParsers/Parsers/IfParser.h"
#include "AstNodeParsers/Parsers/InstructionParser.h"
#include "AstNodeParsers/Parsers/LabelParser.h"
#include "AstNodeParsers/Parsers/WhileParser.h"

class CodeScopeParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse the code of a scope.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_CODESCOPEPARSER_H
