#include "AstNodeParsers/Parsers/MemoryOperandParser.h"

namespace MemoryOperandParser
{
std::shared_ptr<AstNode> BaseDisplacement::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    std::shared_ptr<BaseDisplacementMemory> node;
    std::shared_ptr<AstNode> base, displacement;

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

    node = std::make_shared<BaseDisplacementMemory>(
            std::move(std::dynamic_pointer_cast<IntegerImmediate>(displacement)),
            std::move(std::dynamic_pointer_cast<Variable>(base)),
            "");
    return node;
}

std::shared_ptr<AstNode> BaseIndexScaleDisplacement::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    std::shared_ptr<BaseIndexScaleDisplacementMemory> node;
    std::shared_ptr<AstNode> base, index;
    std::shared_ptr<AstNode> scale, displacement;

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

    node = std::make_shared<BaseIndexScaleDisplacementMemory>(
            std::move(std::dynamic_pointer_cast<IntegerImmediate>(displacement)),
            std::move(std::dynamic_pointer_cast<IntegerImmediate>(scale)),
            std::move(std::dynamic_pointer_cast<Variable>(base)),
            std::move(std::dynamic_pointer_cast<Variable>(index)),
            "");
    return node;
}

std::shared_ptr<AstNode> IndexScale::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    std::shared_ptr<IndexScaleMemory> node;
    std::shared_ptr<AstNode> index, scale;

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

    node = std::make_shared<IndexScaleMemory>(std::move(std::dynamic_pointer_cast<IntegerImmediate>(scale)),
                                              std::move(std::dynamic_pointer_cast<Variable>(index)),
                                              "");
    return node;
}

std::shared_ptr<AstNode> Direct::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    std::shared_ptr<DirectMemory> node;
    std::shared_ptr<AstNode> address;

    if (address = ImmediateParser::Integer().parse(ctx); !address)
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected 'address' for Direct",
                       "MemoryOperandParser::Direct",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    node = std::make_shared<DirectMemory>(std::move(std::dynamic_pointer_cast<IntegerImmediate>(address)), "");
    return node;
}

std::shared_ptr<AstNode> MemoryOperandParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation token;
    std::shared_ptr<AstNode> node;

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

    std::dynamic_pointer_cast<MemoryOperandAstNode>(node)->setReferencedDataType(std::move(token.m_str));
    return node;
}
}; // namespace MemoryOperandParser
