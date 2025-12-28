#ifndef EZPACKER_VARIABLEPARSER_H
#define EZPACKER_VARIABLEPARSER_H

#include "EzLexerCommon.h"
#include "AstNodes/Variable.h"
#include "AstNodeParser/IAstNodeParser.h"
#include "AstNodeParser/AstNodeParsingUtils.h"
#include "AstNodeParsers/ImmediateOperandParser.h"

class VariableParser : public IAstNodeParser<Variable>
{
  public:
    /**
     * Tries to parse a variable node out of the given context.
     * @return std::shared_ptr<AstNode>
     */
    std::shared_ptr<Variable> parse(const std::shared_ptr<IParsingContext> &ctx);

  private:
    /**
     * Parses MANY initializers, separated by commas. Supposes the variable is an array.
     * @param ctx
     * @return
     */
    std::vector<std::shared_ptr<AstNode>> parseInitializerArray(const std::shared_ptr<IParsingContext> &ctx);
};

#endif // EZPACKER_VARIABLEPARSER_H
