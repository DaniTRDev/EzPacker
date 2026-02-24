#include "AstNodeParsers/Parsers/CodeScopeParser.h"

std::shared_ptr<AstNode> CodeScopeParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    std::shared_ptr<CodeScope> node;

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftBrace))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '{' at the start of code scope",
                       "CodeScopeParser::CodeScopeParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    node = std::make_shared<CodeScope>();
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightBrace))
    {
        // At least 1 instruction is expected;
        ParserBatch batch;
        batch.addParsersFromTypeList<LabelParser, IfParser, WhileParser, InstructionParser::InstructionParser>();

        std::shared_ptr<AstNode> exprNode = batch.parse(ctx).m_node;
        while (exprNode)
        {
            node->addExpression(exprNode);
            exprNode = batch.parse(ctx).m_node;
        }

        if (!node->containsExpressions())
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected instruction or nested label in module body",
                           "CodeScopeParser::CodeScopeParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }

        if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightBrace))
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected '}' after module body",
                           "CodeScopeParser::CodeScopeParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }
    }

    return std::move(node);
}
