#include "AstNodeParsers/MemoryOperandParser.h"

std::shared_ptr<MemoryOperandAstNode> MemoryOperandParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<MemoryOperandAstNode> node;
    std::string referencedDataType;

    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier,
                                                  referencedDataType,
                                                  ctx,
                                                  LogMessage("Expected data-type for memory operand"));

    node = std::dynamic_pointer_cast<MemoryOperandAstNode>(
            AstNodeParsingUtils::tryParsers<BaseDisplacement, BaseIndexScaleDisplacement, IndexScale, Direct>(
                    ctx,
                    LogMessage("Invalid memory operand")));

    if (!node)
    {
        return nullptr;
    }

    node->setReferencedDataType(std::move(referencedDataType));
    return std::dynamic_pointer_cast<MemoryOperandAstNode>(node);
}

std::shared_ptr<BaseDisplacementMemory>
MemoryOperandParser::BaseDisplacement::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<BaseDisplacementMemory> node;
    std::shared_ptr<IntegerImmediate> displacement;
    std::shared_ptr<Variable> base;

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftParen,
                                              ctx,
                                              LogMessage("Expected '(' at the start of memory reference"));

    if (base = VariableParser().parse(ctx); !base)
    {
        return nullptr;
    }

    if (AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Comma, ctx))
    {
        // Displacement is expected
        if (displacement = std::dynamic_pointer_cast<IntegerImmediate>(ImmediateOperandParser().parse(ctx));
            !displacement)
        {
            return nullptr;
        }
    }

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightParen,
                                              ctx,
                                              LogMessage("Expected ')' at the end of memory reference"));

    node = std::make_shared<BaseDisplacementMemory>(std::move(displacement), std::move(base), "");
    return std::move(node);
}

std::shared_ptr<BaseIndexScaleDisplacementMemory>
MemoryOperandParser::BaseIndexScaleDisplacement::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<BaseIndexScaleDisplacementMemory> node;
    std::shared_ptr<Variable> base, index;
    std::shared_ptr<IntegerImmediate> scale, displacement;

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftParen,
                                              ctx,
                                              LogMessage("Expected '(' at the start of memory reference"));

    if (base = VariableParser().parse(ctx); !base)
    {
        return nullptr;
    }

    AstNodeParsingUtils::expectTypeAndConsume(
            _TokenType::Comma,
            ctx,
            LogMessage("Expected comma after 'base' register for base + index*scale + displacement reference"));

    if (index = VariableParser().parse(ctx); !index)
    {
        return nullptr;
    }

    AstNodeParsingUtils::expectTypeAndConsume(
            _TokenType::Comma,
            ctx,
            LogMessage("Expected comma after 'index' register for base + index*scale + displacement reference"));

    if (scale = ImmediateOperandParser::Integer().parse(ctx); !scale)
    {
        return nullptr;
    }

    if (AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Comma, ctx))
    {
        // Displacement is expected
        if (displacement = ImmediateOperandParser::Integer().parse(ctx); !displacement)
        {
            return nullptr;
        }
    }

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightParen,
                                              ctx,
                                              LogMessage("Expected ')' at the end of memory reference"));

    node = std::make_shared<BaseIndexScaleDisplacementMemory>(std::move(displacement),
                                                              std::move(scale),
                                                              std::move(base),
                                                              std::move(index),
                                                              "");
    return std::move(node);
}

std::shared_ptr<IndexScaleMemory> MemoryOperandParser::IndexScale::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<IndexScaleMemory> node;
    std::shared_ptr<Variable> index = nullptr;
    std::shared_ptr<IntegerImmediate> scale = nullptr;

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftParen,
                                              ctx,
                                              LogMessage("Expected '(' at the start of memory reference"));

    AstNodeParsingUtils::expectTypeAndConsume(
            _TokenType::Comma,
            ctx,
            LogMessage("Expected comma at the beginning of scale*index memory reference"));

    if (index = VariableParser().parse(ctx); !index)
    {
        return nullptr;
    }

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Comma,
                                              ctx,
                                              LogMessage("Expected ',' after 'index' register"));

    if (scale = ImmediateOperandParser::Integer().parse(ctx); !scale)
    {
        return nullptr;
    }

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightParen,
                                              ctx,
                                              LogMessage("Expected ')' at the end of memory reference"));

    node = std::make_shared<IndexScaleMemory>(std::move(scale), std::move(index), "");
    return std::move(node);
}

std::shared_ptr<DirectMemory> MemoryOperandParser::Direct::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    std::shared_ptr<DirectMemory> node;
    std::shared_ptr<IntegerImmediate> address;

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftParen,
                                              ctx,
                                              LogMessage("Expected '(' at the start of memory reference"));

    if (address = ImmediateOperandParser::Integer().parse(ctx); !address)
    {
        return nullptr;
    }

    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightParen,
                                              ctx,
                                              LogMessage("Expected ')' at the end of memory reference"));

    node = std::make_shared<DirectMemory>(std::move(address), "");
    return std::move(node);
}
