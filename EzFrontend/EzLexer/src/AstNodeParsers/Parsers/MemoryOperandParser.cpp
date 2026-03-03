#include "AstNodeParsers/Parsers/MemoryOperandParser.h"

namespace MemoryOperandParser
{
AstNode *BaseDisplacement::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    BaseDisplacementMemory *node = nullptr;
    AstNode *base = nullptr, *displacement = nullptr;

    if (base = VariableParser().parse(ctx); !base)
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected base for BaseDisplacement operand",
                       "MemoryOperandParser::BaseDisplacement",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    /**
     * If parser reached this place, expression looks like:
     * (identifier(base)+
     * Which, might or might not be a displacement.
     */

    displacement = ImmediateParser::ImmediateParser().parse(ctx);
    node = ctx->getNodePool()->create<BaseDisplacementMemory>((IntegerImmediate *)displacement, (Variable *)base, "");

    return node;
}

AstNode *BaseIndexScaleDisplacement::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    BaseIndexScaleDisplacementMemory *node = nullptr;
    AstNode *base = nullptr, *index = nullptr, *scale = nullptr, *displacement = nullptr;

    if (base = VariableParser().parse(ctx); !base)
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected 'base' for BaseIndexScaleDisplacement operand",
                       "MemoryOperandParser::BaseIndexScaleDisplacement",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected comma after 'base' for BaseIndexScaleDisplacement",
                       "MemoryOperandParser::BaseIndexScaleDisplacement",
                       ctx->getLastSourceReference());
    }

    if (index = VariableParser().parse(ctx); !index)
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected 'index' for BaseIndexScaleDisplacement",
                       "MemoryOperandParser::BaseIndexScaleDisplacement",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected comma after 'index' for BaseIndexScaleDisplacement",
                       "MemoryOperandParser::BaseIndexScaleDisplacement",
                       ctx->getLastSourceReference());
    }

    /**
     * If parser reached this place, expression looks like:
     * (variable(base), variable(base),
     * Which can only be a BaseIndexScaleDisplacement operand.
     */

    if (scale = ImmediateParser::Integer().parse(ctx); !scale)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected 'scale' for BaseIndexScaleDisplacement",
                       "MemoryOperandParser::BaseIndexScaleDisplacement",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected comma after 'scale' for BaseIndexScaleDisplacement",
                       "MemoryOperandParser::BaseIndexScaleDisplacement",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    // Displacement is expected
    if (displacement = ImmediateParser::Integer().parse(ctx); !displacement)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected 'displacement' for BaseIndexScaleDisplacement",
                       "MemoryOperandParser::BaseIndexScaleDisplacement",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    node = ctx->getNodePool()->create<BaseIndexScaleDisplacementMemory>((IntegerImmediate *)displacement,
                                                                        (IntegerImmediate *)scale,
                                                                        (Variable *)base,
                                                                        (Variable *)index,
                                                                        "");
    return node;
}

AstNode *IndexScale::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    IndexScaleMemory *node = nullptr;
    AstNode *index = nullptr, *scale = nullptr;

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected comma for IndexScale",
                       "MemoryOperandParser::IndexScale",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    /**
     * If parser reached this place, expression looks like:
     * (,  which can only be an IndexScale.
     */

    if (index = VariableParser().parse(ctx); !index)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected 'index' for IndexScale",
                       "MemoryOperandParser::IndexScale",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected comma after 'index' for IndexScale",
                       "MemoryOperandParser::IndexScale",
                       ctx->getLastSourceReference());
    }

    if (scale = ImmediateParser::Integer().parse(ctx); !scale)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected 'scale' for IndexScale",
                       "MemoryOperandParser::IndexScale",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    node = ctx->getNodePool()->create<IndexScaleMemory>((IntegerImmediate *)scale, (Variable *)index, "");
    return node;
}

AstNode *Direct::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    DirectMemory *node;
    AstNode *address = nullptr;

    if (address = ImmediateParser::Integer().parse(ctx); !address)
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected 'address' for Direct",
                       "MemoryOperandParser::Direct",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    node = ctx->getNodePool()->create<DirectMemory>((IntegerImmediate *)address, "");
    return node;
}

AstNode *MemoryOperandParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation token;
    AstNode *node;

    ctx->consumeIf(ParsingCondition::TokenType, &token, _TokenType::Identifier);
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected '(' at the start of memory reference",
                       "MemoryOperandParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    /**
     * If parser reached this place, expression looks like:
     * identifier(datatype) (
     * Which can only be a MemoryOperand (any of operand types).
     */

    ParserBatch batch;
    batch.addParsersFromTypeList<BaseIndexScaleDisplacement, IndexScale, BaseDisplacement, Direct>();

    node = batch.parse(ctx).m_node;
    if (!node)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Invalid memory operand",
                       "MemoryOperandParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ')' at the end of memory reference",
                       "MemoryOperandParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    MemoryOperandAstNode *memoryOperand = (MemoryOperandAstNode *)node;
    memoryOperand->setReferencedDataType(ctx->getStringPool()->createConstantString(token.m_str));

    return memoryOperand;
}
}; // namespace MemoryOperandParser
