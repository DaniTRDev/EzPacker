#ifndef EZPACKER_IMMEDIATEPARSER_H
#define EZPACKER_IMMEDIATEPARSER_H

#include "AstNodeParsers/IAstNodeParser.h"
#include "AstNodes/ImmediateOperand.h"
#include "AstNodeParsers/ParserBatch.h"

namespace ImmediateParser
{
class Integer : public IAstNodeParser
{
  public:
    /**
     * Tries to parse an integer immediate out of the token list within context.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class Float : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a floating-point immediate out of the token list within context.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class String : public IAstNodeParser
{
  public:
    /**
     * Tries to parse a string immediate out of the token list within context.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

class ImmediateParser : public IAstNodeParser
{
  public:
    /**
     * Tries to parse an immediate (Integer, Float or String) out of the token list within context.
     * @param ctx
     * @return AstNode*
     */
    AstNode *parse(const std::shared_ptr<BasicParsingContext> &ctx) override;
};

}; // namespace ImmediateParser

#endif // EZPACKER_IMMEDIATEPARSER_H
