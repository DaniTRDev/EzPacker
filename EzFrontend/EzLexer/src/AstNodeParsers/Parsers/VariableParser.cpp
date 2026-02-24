#include "AstNodeParsers/Parsers/VariableParser.h"

std::shared_ptr<AstNode> VariableParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    bool isArray = false;
    TokenInformation typeToken, nameToken;
    std::shared_ptr<Variable> node;
    std::string type, name;
    std::vector<std::shared_ptr<AstNode>> initializers;

    // A variable might or might not have a type. This will be guarded in the semantic checker.
    if (ctx->consumeIf(ParsingCondition::TokenType, &typeToken, _TokenType::Identifier))
    {
        type = std::move(typeToken.m_str);
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Percentage))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected '%' at variable start",
                       "VariableParser::VariableParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    /**
     * At this point the expression looks like:
     * identifier(optional) %
     * Which can only be a Variable.
     */

    if (!ctx->consumeIf(ParsingCondition::TokenType, &nameToken, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected string for variable name",
                       "VariableParser::VariableParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    name = std::move(nameToken.m_str);

    if (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Colon))
    {
        // Variable has initializers.
        if (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftBrace))
        {
            // Variable is an array.
            if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightBrace))
            {
                do
                {
                    auto initializer = ImmediateParser::ImmediateParser().parse(ctx);
                    if (!initializer)
                    {
                        ctx->emitError(ErrorSeverity::Fatal,
                                       "Expected at least 1 element in array",
                                       "VariableParser::VariableParser",
                                       ctx->getLastSourceReference());
                        return nullptr;
                    }

                    initializers.push_back(std::move(initializer));

                } while (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma));

                if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightBrace))
                {
                    ctx->emitError(ErrorSeverity::Fatal,
                                   "Expected '}' at the end of variable initializer list",
                                   "VariableParser::VariableParser",
                                   ctx->getLastSourceReference());
                    return nullptr;
                }
            }

            isArray = true;
        }
        else
        {
            // Single-initializer variable.
            std::shared_ptr<AstNode> initializer;
            if (initializer = ImmediateParser::ImmediateParser().parse(ctx); !initializer)
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Expected single initializer for variable",
                               "VariableParser::VariableParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }

            initializers.push_back(std::move(initializer));
        }
    }

    node = std::make_shared<Variable>(isArray, type, name, std::move(initializers));
    return std::move(node);
}