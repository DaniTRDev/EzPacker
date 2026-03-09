#include "AstNodeParsers/Parsers/ForParser.h"

AstNode *ForParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::For))
    {
        return nullptr; // Silent error not to overlap.
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        ctx->emitError(ErrorSeverity::Fatal, "Expected '(' after 'for'", "ForParser", ctx->getLastSourceReference());
        return nullptr;
    }

    CodeScope *initialization = dynamic_cast<CodeScope *>(CodeScopeParser().parse(ctx));
    if (!initialization)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected initialization clause in for loop",
                       "ForParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '(' before 'for' condition",
                       "ForParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    ConditionAstNode *condition = dynamic_cast<ConditionAstNode *>(ConditionParser().parse(ctx));
    if (!condition)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected condition clause in for loop",
                       "ForParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ')' after 'for' condition",
                       "ForParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    CodeScope *nextIt = dynamic_cast<CodeScope *>(CodeScopeParser().parse(ctx));
    if (!nextIt)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected 'next iteration' clause in for loop",
                       "ForParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ')' after for header",
                       "ForParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    CodeScope *body = dynamic_cast<CodeScope *>(CodeScopeParser().parse(ctx));
    if (!body)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected body after 'for' header",
                       "ForParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    return ctx->getNodePool()->createNode<ForAstNode>(body, initialization, nextIt, condition);
}
