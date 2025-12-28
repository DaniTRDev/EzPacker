#ifndef EZPACKER_MEMORYOPERANDPARSER_H
#define EZPACKER_MEMORYOPERANDPARSER_H

#include "EzLexerCommon.h"
#include "AstNodeParser/IAstNodeParser.h"
#include "AstNodes/MemoryOperand.h"
#include "AstNodeParser/AstNodeParsingUtils.h"
#include "VariableParser.h"
#include "ImmediateOperandParser.h"

class MemoryOperandParser : public IAstNodeParser<MemoryOperandAstNode>
{
  public:
    /**
     * Tries to parse a memory operand out of the token list within context.
     * @param ctx
     * @return std::shared_ptr<AstNode>
     */
    std::shared_ptr<MemoryOperandAstNode> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    
    class BaseDisplacement : public IAstNodeParser<BaseDisplacementMemory>
    {
      public:
        /**
         * Tries to parse a BaseDisplacement memory operand out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<BaseDisplacementMemory>
         */
        std::shared_ptr<BaseDisplacementMemory> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };

    class BaseIndexScaleDisplacement : public IAstNodeParser<BaseIndexScaleDisplacementMemory>
    {
      public:
        /**
         * Tries to parse a BaseIndexScaleDisplacement memory operand out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<BaseIndexScaleDisplacement>
         */
        std::shared_ptr<BaseIndexScaleDisplacementMemory> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };

    class IndexScale : public IAstNodeParser<IndexScaleMemory>
    {
      public:
        /**
         * Tries to parse a IndexScale memory operand out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<IndexScale>
         */
        std::shared_ptr<IndexScaleMemory> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };

    class Direct : public IAstNodeParser<DirectMemory>
    {
      public:
        /**
         * Tries to parse a DirectMemory operand out of the token list within context.
         * @param ctx
         * @return std::shared_ptr<DirectMemory>
         */
        std::shared_ptr<DirectMemory> parse(const std::shared_ptr<IParsingContext> &ctx) override;
    };
};

#endif // EZPACKER_MEMORYOPERANDPARSER_H
