#ifndef EZPACKER_FORPARSER_H
#define EZPACKER_FORPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/ForAstNode.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodeParsers/ParserBatch.h"
#include "AstNodeParsers/Parsers/CodeScopeParser.h"

class ForParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse an for loop from the current position in the token stream. If successful, returns the
     * corresponding for AST node.
     * @param ctx
     * @return IfAstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_FORPARSER_H
