#include "AstNodeParsers/Parsers/IfParser.h"

AstNode *IfParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::If))
    {
        // This is not an if-clause.
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        // Expected '(' after 'if'.
        ctx->emitError(ErrorSeverity::Fatal, "Expected '(' after 'if'", "IfParser", ctx->getLastSourceReference());
        return nullptr;
    }

    AstNode *condition = nullptr, *trueScope = nullptr, *falseScope = nullptr;

    if (condition = ConditionParser().parse(ctx); !condition)
    {
        // Failed to parse condition.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Failed to parse condition in 'if' statement",
                       "IfParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {
        // Expected ')' after 'if'.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ')' after 'if' condition",
                       "IfParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (trueScope = CodeScopeParser().parse(ctx); !trueScope)
    {
        // Failed to parse true scope.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Failed to parse code block for 'if' statement",
                       "IfParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Else))
    {
        if (ctx->canPeek() && ctx->peek().m_type == _TokenType::If)
        {
            // This if is an if-elif.
            falseScope = parse(ctx);
            if (!falseScope)
            {
                // Failed to parse else scope.
                ctx->emitError(ErrorSeverity::Fatal,
                               "Failed to parse 'else if' clause",
                               "IfParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }
        }
        else
        {
            // This is an if-else.
            falseScope = CodeScopeParser().parse(ctx);
            if (!falseScope)
            {
                // Failed to parse else scope.
                ctx->emitError(ErrorSeverity::Fatal,
                               "Failed to parse 'else' clause",
                               "IfParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }
        }
    }

    if (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Else))
    {
        // This if is an if-else or if-elseif-else.
        falseScope = CodeScopeParser().parse(ctx);
        if (!falseScope)
        {
            // Failed to parse else scope.
            ctx->emitError(ErrorSeverity::Fatal,
                           "Failed to parse code block for 'else' clause",
                           "IfParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }
    }

    IfAstNode *node = ctx->getNodePool()->create<IfAstNode>();
    node->setCondition((ConditionAstNode *)(condition));
    node->setTrueScope((CodeScope *)(trueScope));
    node->setFalseScope(falseScope);

    return node;
}
