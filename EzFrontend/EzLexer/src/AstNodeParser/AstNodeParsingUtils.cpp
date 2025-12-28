#include "AstNodeParser/AstNodeParsingUtils.h"

bool AstNodeParsingUtils::expectContent(std::string_view expectedContent,
                                        const std::shared_ptr<IParsingContext> &ctx,
                                        LogMessage errorMsg)
{
    if (!ctx || !ctx->canPeek() || ctx->peek().m_str != expectedContent)
    {
        if (!errorMsg.isEmpty())
        {
            std::shared_ptr<SourceReference> ref = ctx->canPeek() ? ctx->peek().m_sourceReference : nullptr;
            ctx->getErrorCollector()->error(errorMsg, std::move(ref));

            throw std::runtime_error("PARSING FAILED");
        }
        return false;
    }

    return true;
}

bool AstNodeParsingUtils::expectType(_TokenType expectedType,
                                     const std::shared_ptr<IParsingContext> &ctx,
                                     LogMessage errorMsg)
{
    if (!ctx || !ctx->canPeek() || ctx->peek().m_type != expectedType)
    {
        if (!errorMsg.isEmpty())
        {
            std::shared_ptr<SourceReference> ref = ctx->canPeek() ? ctx->peek().m_sourceReference : nullptr;
            ctx->getErrorCollector()->error(errorMsg, std::move(ref));

            throw std::runtime_error("PARSING FAILED");
        }
        return false;
    }

    return true;
}

bool AstNodeParsingUtils::expectTypeAndConsume(_TokenType expectedType,
                                               const std::shared_ptr<IParsingContext> &ctx,
                                               LogMessage errorMsg)
{
    std::string out;
    return expectTypeDumpAndConsume(expectedType, out, ctx, std::move(errorMsg));
}

bool AstNodeParsingUtils::expectTypeDumpAndConsume(_TokenType expectedType,
                                                   std::string &out,
                                                   const std::shared_ptr<IParsingContext> &ctx,
                                                   LogMessage errorMsg)
{
    bool isPosValid = ctx->canPeek();
    size_t currentPos = ctx->getCurrentPosition();

    if (!expectType(expectedType, ctx, std::move(errorMsg)))
    {
        if (isPosValid)
            ctx->setPosition(currentPos);

        return false;
    }

    out = ctx->peek().m_str;
    ctx->consume();

    return true;
}

size_t
AstNodeParsingUtils::ignoreType(_TokenType type, const std::shared_ptr<IParsingContext> &ctx, LogMessage errorMsg)
{
    if (!ctx)
    {
        if (!errorMsg.isEmpty())
        {
            std::shared_ptr<SourceReference> ref = ctx->canPeek() ? ctx->peek().m_sourceReference : nullptr;
            ctx->getErrorCollector()->error(errorMsg, std::move(ref));

            throw std::runtime_error("PARSING FAILED");
        }
        return 0;
    }

    size_t count = 0;
    while (ctx->canPeek())
    {
        if (ctx->peek().m_type != type)
            break;

        count++;
        ctx->consume();
    }

    return count;
}
