#ifndef EZPACKER_IFPARSER_H
#define EZPACKER_IFPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/IfAstNode.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodeParsers/ParserBatch.h"
#include "AstNodeParsers/Parsers/CodeScopeParser.h"
#include "AstNodeParsers/Parsers/ConditionParser.h"

class IfParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse an if-elseif-else block.
     * @param ctx
     * @return IfAstNode*
     */
    AstNode* parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_IFPARSER_H
