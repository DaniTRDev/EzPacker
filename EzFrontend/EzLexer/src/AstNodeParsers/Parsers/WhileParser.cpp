#include "AstNodeParsers/Parsers/WhileParser.h"

std::shared_ptr<AstNode> WhileParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::While))
    {
        // This is not an if-clause.
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        // Expected '(' after 'if'.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '(' after 'while'",
                       "WhileParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::shared_ptr<AstNode> condition;
    std::shared_ptr<AstNode> codeScope;

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
        // Expected '(' after 'if'.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ')' after 'while' condition",
                       "WhileParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (codeScope = CodeScopeParser().parse(ctx); !codeScope)
    {
        // Failed to parse true scope.
        ctx->emitError(ErrorSeverity::Fatal,
                       "Failed to parse code block for 'while' statement",
                       "WhileParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::shared_ptr<WhileAstNode> node = std::make_shared<WhileAstNode>();
    node->setCondition(std::dynamic_pointer_cast<ConditionAstNode>(condition));
    node->setCodeScope(std::dynamic_pointer_cast<CodeScope>(codeScope));

    return node;
}
