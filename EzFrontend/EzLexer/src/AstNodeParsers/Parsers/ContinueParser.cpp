#include "AstNodeParsers/Parsers/ContinueParser.h"

AstNode *ContinueParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Continue))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected 'continue'",
                       "ContinueParser::ContinueParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::SemiColon))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected ';' after 'continue'",
                       "BreakParser::ContinueParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }
    
    return ctx->getNodePool()->createNode<ContinueAstNode>();
}
