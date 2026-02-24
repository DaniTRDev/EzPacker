#include "AstNodeParsers/Parsers/ModuleParser.h"

namespace ModuleParser
{
std::shared_ptr<AstNode> ModuleHeaderParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    TokenInformation nameToken, returnTypeToken;
    std::shared_ptr<::ModuleHeader> node;
    std::vector<std::shared_ptr<AstNode>> arguments;

    if (!ctx->consumeIf(ParsingCondition::TokenType, &returnTypeToken, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected module return type",
                       "ModuleParser::ModuleHeaderParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    if (!ctx->consumeIf(ParsingCondition::TokenType, &nameToken, _TokenType::Identifier))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected module name",
                       "ModuleParser::ModuleHeaderParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    std::string &name = nameToken.m_str, &returnType = returnTypeToken.m_str;

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::LeftParen))
    {
        ctx->emitError(ErrorSeverity::Soft,
                       "Expected '(' after module name",
                       "ModuleParser::ModuleHeaderParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    /**
     * If parser reached this place, expression looks like:
     * identifier(returnType) identifier(name) (
     * Which can only be a ModuleHeader.
     */

    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {

        // We expect call parameters.
        ParserBatch batch;
        batch.addParsersFromTypeList<VariableParser>();

        // At least 1 parameter is expected.
        do
        {
            std::shared_ptr<AstNode> argument;
            if (argument = batch.parse(ctx).m_node; !argument)
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Expected module parameter",
                               "ModuleParser::ModuleHeaderParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }

            arguments.push_back(std::move(argument));
        } while (ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::Comma));

        if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
        {
            ctx->emitError(ErrorSeverity::Fatal,
                           "Expected ')' at the end of module definition",
                           "ModuleParser::ModuleHeaderParser",
                           ctx->getLastSourceReference());
            return nullptr;
        }
    }

    node = std::make_shared<::ModuleHeader>(std::move(name), std::move(returnType), std::move(arguments));
    return std::move(node);
}

std::shared_ptr<AstNode> ModuleParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    // Order is important, we can't alphabetically order these variables.
    std::shared_ptr<AstNode> header = ModuleHeaderParser().parse(ctx);
    if (!header)
    {
        return nullptr;
    }

    std::shared_ptr<AstNode> body = CodeScopeParser().parse(ctx);
    if (!body)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Empty module scope",
                       "ModuleParser::ModuleHeaderParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    return std::make_shared<Module>(std::move(std::dynamic_pointer_cast<CodeScope>(body)),
                                    std::move(std::dynamic_pointer_cast<ModuleHeader>(header)));
}
}; // namespace ModuleParser