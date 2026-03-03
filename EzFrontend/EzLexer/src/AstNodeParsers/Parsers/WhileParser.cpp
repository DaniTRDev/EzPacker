#include "AstNodeParsers/Parsers/WhileParser.h"

AstNode *WhileParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::While))
    {
        // This is not a while loop.
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        // Expected '(' after 'while'.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '(' after 'while'",
                       "WhileParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    AstNode *condition = nullptr, *codeScope = nullptr;

    if (condition = ConditionParser().parse(ctx); !condition)
    {
        // Failed to parse condition.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Failed to parse condition in 'while' statement",
                       "WhileParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {
        // Expected ')' after 'while' condition.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ')' after 'while' condition",
                       "WhileParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (codeScope = CodeScopeParser().parse(ctx); !codeScope)
    {
        // Failed to parse while loop body.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Failed to parse code block for 'while' statement",
                       "WhileParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    WhileAstNode *node = ctx->getNodePool()->createNode<WhileAstNode>();
    node->setCondition((ConditionAstNode *)condition);
    node->setCodeScope((CodeScope *)codeScope);

    return node;
}
