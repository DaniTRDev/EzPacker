#include "AstNodeParsers/Parsers/VariableParser.h"

AstNode *VariableParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation typeToken, nameToken;
    TypedPoolSlice<AstNode> *initializers = nullptr;
    std::string_view type, name;

    // A variable might or might not have a type. This will be guarded in the semantic checker.
    if (ctx->consumeIf(ParsingCondition::TokenType, &typeToken, _TokenType::Identifier))
    {
        type = std::move(ctx->getStringPool()->createConstantString(typeToken.m_str));
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

    name = ctx->getStringPool()->createConstantString(nameToken.m_str);

    if (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Colon))
    {
        // Variable has initializers.
        initializers = ctx->getNodePool()->createSlice<AstNode>();

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

                    ctx->getNodePool()->appendToSlice(initializers, initializer);
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
        }
        else
        {
            // Single-initializer variable.
            AstNode *initializer;
            if (initializer = ImmediateParser::ImmediateParser().parse(ctx); !initializer)
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Expected single initializer for variable",
                               "VariableParser::VariableParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }

            ctx->getNodePool()->appendToSlice(initializers, initializer);
        }
    }

    Variable *node = ctx->getNodePool()->create<Variable>(initializers, type, name);
    return std::move(node);
}