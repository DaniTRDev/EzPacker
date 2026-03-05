/**
 * @file VariableParser.h
 * @brief Parser for variable references: `%name` or typed declarations `type %name`.
 *
 * Produces a Variable node.  When a type prefix is present it is stored on
 * the node; otherwise the type is left unset and will be resolved later by
 * the semantic analysis pass.
 */
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
     * @return AstNode*
     */
    AstNode* parse(const std::shared_ptr<BasicParsingContext> &ctx);
};

#endif // EZPACKER_VARIABLEPARSER_H
