#include "AstNodeParsers/Parsers/BreakParser.h"

AstNode *BreakParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Break))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected 'break'",
                       "BreakParser::BreakParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }
    
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::SemiColon))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ';' after 'break'",
                       "BreakParser::BreakParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    return ctx->getNodePool()->createNode<BreakAstNode>();
}
