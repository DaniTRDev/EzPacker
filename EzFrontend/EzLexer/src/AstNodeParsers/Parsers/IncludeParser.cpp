#include "AstNodeParsers/Parsers/IncludeParser.h"

AstNode *IncludeParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Include))
    {
        // Return silent error not to overlap with other parser.
        return nullptr;
    }

    TokenInformation relFilePath;
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LowerThan))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '<' after 'include' directive",
                       "IncludeParser::parse",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, &relFilePath, _TokenType::String))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected relative file path (string) in 'include' directive",
                       "IncludeParser::parse",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::GreaterThan))
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected '>' after 'include' path",
                       "IncludeParser::parse",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::string_view filePath = ctx->getStringPool()->createConstantString(relFilePath.m_str);
    return ctx->getNodePool()->create<IncludeAstNode>(filePath);
}
