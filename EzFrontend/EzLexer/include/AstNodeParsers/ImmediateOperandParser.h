#ifndef EZPACKER_IMMEDIATEOPERANDPARSER_H
#define EZPACKER_IMMEDIATEOPERANDPARSER_H

#include "AstNodeParser/IAstNodeParser.h"
#include "AstNodes/ImmediateOperand.h"
#include "AstNodeParser/AstNodeParsingUtils.h"

class ImmediateOperandParser : public IAstNodeParser<ImmediateOperand>
{
  public:
    class Integer : public IAstNodeParser<IntegerImmediate>
    {
      public:
        /**
         * Tries to parse an integer immediate out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<AstNode>
         */
        std::shared_ptr<IntegerImmediate> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };
    class Float : public IAstNodeParser<FloatImmediate>
    {
      public:
        /**
         * Tries to parse a floating-point immediate out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<AstNode>
         */
        std::shared_ptr<FloatImmediate> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };
    class String : public IAstNodeParser<StringImmediate>
    {
      public:
        /**
         * Tries to parse a string immediate out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<AstNode>
         */
        std::shared_ptr<StringImmediate> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };

    /**
     * Tries to parse an immediate out of the token list within context.
     * @param ctx
     * @return std::shared_ptr<AstNode>
     */
    std::shared_ptr<ImmediateOperand> parse(const std::shared_ptr<IParsingContext> &ctx) override;
};

#endif // EZPACKER_IMMEDIATEOPERANDPARSER_H
