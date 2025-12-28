#include "AstNodeParsers/VariableParser.h"

std::shared_ptr<Variable> VariableParser::parse(const std::shared_ptr<IParsingContext> &ctx)
{
    bool isArray = false;
    std::shared_ptr<Variable> node;
    std::string type, name;
    std::vector<std::shared_ptr<AstNode>> initializers;

    // A variable might or might not have a type. This will be guarded in the semantic checker.
    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier, type, ctx);
    AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Percentage, ctx, LogMessage("Invalid variable start"));
    AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType::Identifier,
                                                  name,
                                                  ctx,
                                                  LogMessage("Invalid variable name"));

    if (AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Colon, ctx))
    {
        // Variable is an array.
        if (AstNodeParsingUtils::expectTypeAndConsume(_TokenType::LeftBrace, ctx))
        {
            if (initializers = std::move(parseInitializerArray(ctx)); initializers.empty())
            {
                return nullptr;
            }

            AstNodeParsingUtils::expectTypeAndConsume(_TokenType::RightBrace,
                                                      ctx,
                                                      LogMessage("Expected '}}' at the end of initializers"));

            isArray = true;
        }
        else
        {
            // Single-initializer variable.
            std::shared_ptr<ImmediateOperand> initializer;
            if (initializer = ImmediateOperandParser().parse(ctx); !initializer)
            {
                return nullptr;
            }

            initializers.push_back(std::move(initializer));
        }
    }

    node = std::make_shared<Variable>(isArray, type, name, std::move(initializers));
    return std::move(node);
}

std::vector<std::shared_ptr<AstNode>> VariableParser::parseInitializerArray(const std::shared_ptr<IParsingContext> &ctx)
{
    std::vector<std::shared_ptr<AstNode>> initializers;

    /**
     * This will parse multiple initializers for array variable.
     */

    while (AstNodeParsingUtils::expectType(_TokenType::NumberInt, ctx) ||
           AstNodeParsingUtils::expectType(_TokenType::NumberFloat, ctx) ||
           AstNodeParsingUtils::expectType(_TokenType::String, ctx))
    {
        std::shared_ptr<ImmediateOperand> initializer;
        if (initializer = ImmediateOperandParser().parse(ctx); !initializer)
        {
            return {};
        }

        initializers.push_back(std::move(initializer));

        if (!AstNodeParsingUtils::expectTypeAndConsume(_TokenType::Comma, ctx))
            break;
    }

    return std::move(initializers);
}
