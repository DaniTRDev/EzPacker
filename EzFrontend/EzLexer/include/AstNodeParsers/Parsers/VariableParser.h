#ifndef EZPACKER_VARIABLEPARSER_H
#define EZPACKER_VARIABLEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/Variable.h"
#include "AstNodeParsers/IAstNodeParser.h"
#include "ImmediateParser.h"

class VariableParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a variable node out of the given context.
     * @return std::shared_ptr<AstNode>
     */
    std::shared_ptr<AstNode> parse(const std::shared_ptr<BasicParsingContext> &ctx);
};

#endif // EZPACKER_VARIABLEPARSER_H
