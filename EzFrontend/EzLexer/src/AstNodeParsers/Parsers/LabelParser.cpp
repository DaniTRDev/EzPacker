#include "AstNodeParsers/Parsers/LabelParser.h"

AstNode *LabelParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation token;
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

    /*
     * If parser reached this place, this expression can only be a Label. A label may or may not have any instructions.
     */
    CodeScope *codeScope = (CodeScope *)CodeScopeParser().parse(ctx);
    if (!codeScope)
    {
        ctx->emitError(ErrorSeverity::Fatal, "Empty label scope", "LabelParser", ctx->getLastSourceReference());
        return nullptr;
    }

    Label *node = ctx->getNodePool()->create<Label>(ctx->getStringPool()->createConstantString(token.m_str));
    node->setCodeScope(codeScope);

    return node;
}
