#ifndef EZPACKER_INCLUDEPARSER_H
#define EZPACKER_INCLUDEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/IncludeAstNode.h"
#include "AstNodeParsers/BasicParsingContext.h"
#include "AstNodeParsers/IAstNodeParser.h"

class IncludeParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse the include directive. It will append the file to the discovered list of the given semantic
     * context.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_INCLUDEPARSER_H
