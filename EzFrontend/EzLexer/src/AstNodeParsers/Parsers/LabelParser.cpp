#include "AstNodeParsers/Parsers/LabelParser.h"

std::shared_ptr<AstNode> LabelParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation token;
    std::shared_ptr<Label> node;

    if (!ctx->consumeIf(ParsingCondition::TokenType, &token, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected identifier for label name",
                       "LabelParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Colon))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected ':' after label name",
                       "LabelParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    /**
     * If parser reached this place, this expression can only be a Label.
     */

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftBrace))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '{' after label ':'",
                       "LabelParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    /*
     * A label may or may not have any instructions.
     */
    node = std::make_shared<Label>(std::move(token.m_str));
    if (auto scope = CodeScopeParser().parse(ctx); !scope)
    {
        ctx->emitError(ErrorSeverity::Fatal, "Empty label scope", "LabelParser", ctx->getLastSourceReference());
        return nullptr;
    }

    return std::move(node);
}
