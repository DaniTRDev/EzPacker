#include "AstNodeParsers/Parsers/ModuleParser.h"

namespace ModuleParser
{
AstNode *ModuleHeaderParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    ModuleHeader *node = nullptr;
    StringPool *stringPool = ctx->getStringPool();
    TypedPool *nodePool = ctx->getNodePool();
    TokenInformation nameToken, returnTypeToken;

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

    std::string_view name = stringPool->createConstantString(nameToken.m_str),
                     returnType = stringPool->createConstantString(returnTypeToken.m_str);

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

    TypedPoolLinkedList<AstNode> *arguments = nodePool->createLinkedList<AstNode>();
    if (!ctx->consumeIf(ParsingCondition::TokenType, nullptr, _TokenType::RightParen))
    {

        // We expect call parameters.
        ParserBatch batch;
        batch.addParsersFromTypeList<VariableParser>();

        // At least 1 parameter is expected.
        do
        {
            AstNode *argument = nullptr;
            if (argument = batch.parse(ctx).m_node; !argument)
            {
                ctx->emitError(ErrorSeverity::Fatal,
                               "Expected module parameter",
                               "ModuleParser::ModuleHeaderParser",
                               ctx->getLastSourceReference());
                return nullptr;
            }
            nodePool->appendToListBack(arguments, argument);
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

    node = nodePool->create<ModuleHeader>(arguments, name, returnType);
    return node;
}

AstNode *ModuleParser::parse(const std::shared_ptr<BasicParsingContext> &ctx)
{
    // Order is important, we can't alphabetically order these variables.
    AstNode *header = ModuleHeaderParser().parse(ctx);
    if (!header)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Error in module header",
                       "ModuleParser::ModuleHeaderParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    AstNode *body = CodeScopeParser().parse(ctx);
    if (!body)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Error in module scope",
                       "ModuleParser::ModuleHeaderParser",
                       ctx->getLastSourceReference());
        return nullptr;
    }

    return ctx->getNodePool()->create<Module>((CodeScope *)body, (ModuleHeader *)header);
}
}; // namespace ModuleParser