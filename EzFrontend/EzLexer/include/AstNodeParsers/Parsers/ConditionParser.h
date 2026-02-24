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
     * @return std::shared_ptr<::ModuleHeaderParser>
     */
    std::shared_ptr<AstNode> parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

#endif // EZPACKER_CONDITIONPARSER_H
