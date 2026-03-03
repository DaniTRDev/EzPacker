#ifndef EZPACKER_CONTINUEPARSER_H
#define EZPACKER_CONTINUEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/ContinueAstNode.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"

class ContinueParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse the given continue statement. If the current token is not a continue statement, it returns
     * nullptr.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_CONTINUEPARSER_H
