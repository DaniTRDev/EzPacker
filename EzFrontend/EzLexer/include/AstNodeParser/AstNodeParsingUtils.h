#ifndef EZPACKER_ASTNODEPARSINGUTILITIES_H
#define EZPACKER_ASTNODEPARSINGUTILITIES_H

#include "EzLexerCommon.h"
#include "IParsingContext.h"
#include "AstNode/AstNode.h"
#include "AstNodeParser/IAstNodeParser.h"

/**
 * Basic instrumentation to deal with common, basic and repetitive operations for token: checking type, content, ...
 */
class AstNodeParsingUtils
{
  public:
    /**
     * Returns true if current context is valid, there are tokens available in it and the first one is of the given
     * type. If there was an error an error with "errorMsg" is pushed to current error collector scope an and exception
     * WITHOUT information is thrown.
     * @param expectedContent
     * @param ctx
     * @param errorMsg
     * @return bool
     */
    static bool expectContent(std::string_view expectedContent,
                              const std::shared_ptr<IParsingContext> &ctx,
                              LogMessage errorMsg = LogMessage());

    /**
     * Returns true if current context is valid, there are tokens available in it and the first one is of the given
     * type. If there was an error an error with "errorMsg" is pushed to current error collector scope an and exception
     * WITHOUT information is thrown.
     * @param expectedType
     * @param ctx
     * @param errorMsg
     * @return bool
     */
    static bool expectType(_TokenType expectedType,
                           const std::shared_ptr<IParsingContext> &ctx,
                           LogMessage errorMsg = LogMessage());

    /**
     * Does the same as expectType but if all conditions are met, the token is also consumed. If there was an error an
     * error with "errorMsg" is pushed to current error collector scope an and exception WITHOUT information is thrown.
     * If there was an error parser position will be restored to the first token prior this call.
     * @param expectedType
     * @param ctx
     * @param errorMsg
     * @return bool
     */
    static bool expectTypeAndConsume(_TokenType expectedType,
                                     const std::shared_ptr<IParsingContext> &ctx,
                                     LogMessage errorMsg = LogMessage());

    /**
     * Does the same as expectTypeAndConsume but sets out to the token's str before consuming. If there was an error an
     * error with "errorMsg" is pushed to current error collector scope an and exception WITHOUT information is thrown.
     * . If there was an error parser position will be restored to the first token prior
     * this call.
     * @param expectedType
     * @param out
     * @param ctx
     * @param errorMsg
     */
    static bool expectTypeDumpAndConsume(_TokenType expectedType,
                                         std::string &out,
                                         const std::shared_ptr<IParsingContext> &ctx,
                                         LogMessage errorMsg = LogMessage());

    /**
     * Returns the number of ignored tokens. If context is valid, has tokens and tokens are of the given type they
     * will be CONSUMED. Stops when there aren't more tokens or when current token is not of expected type. If there was
     * an error, "errorMsg" is pushed to current error collector scope an and exception WITHOUT information is thrown.
     * @param type
     * @param ctx
     * @param errorMsg
     * @return size_t
     */
    static size_t
    ignoreType(_TokenType type, const std::shared_ptr<IParsingContext> &ctx, LogMessage errorMsg = LogMessage());

    /**
     * This function tries a list of parsers and returns the first one that returned != nullptr. It enters a new scope
     * within ErrorCollector and if the current parser returned nullptr logs are PROPAGATED to the upper scope of. In
     * this function errorMsg is passed as a const referenced to avoid copying data.
     *
     * Everything is executed inside a try-catch block that will catch generated exceptions out of given parsers.
     *
     * If errorMsg is not empty, it will be pushed (as an error) in the LAST scope created by the function which
     * is the scope of the last parser type that was tried.
     *
     * Note: parsers will be automatically created.
     * @tparam FirstParser First of the parsers to try (used to be able to stop type-recursion from parameter pack).
     * @tparam OtherParsersTypes The rest of parsers to try.
     * @param errorMsg
     * @return std::shared_ptr<AstNode>
     */
    template <typename FirstParser, typename... OtherParsersTypes>
    static std::shared_ptr<AstNode> tryParsers(const std::shared_ptr<IParsingContext> &ctx,
                                               const LogMessage &errorMsg = LogMessage())
    {
        std::shared_ptr<AstNode> node;
        ctx->getErrorCollector()->enterScope();
        ctx->beginMultiSourceRef();
        {
            size_t currentPos = 0;
            bool wasPosValid = ctx->canPeek();
            try
            {
                currentPos = ctx->getCurrentPosition();
                if (node = FirstParser().parse(ctx); node)
                {
                    ctx->getErrorCollector()->exitScope(ErrorHandleType::Discard);

                    std::shared_ptr<SourceReference> merged =
                            ctx->getSourceManager()->mergeReferences(ctx->getCurrentMultiReference());
                    node->setSourceRef(std::move(merged));

                    ctx->endMultiSourceRef();
                    return std::move(node);
                }
            }
            catch (...)
            {
                ;
            }

            if (wasPosValid)
                ctx->setPosition(currentPos);

            if constexpr (sizeof...(OtherParsersTypes) > 0) // If parameter pack is not empty, keep recursing.
            {
                if (node = tryParsers<OtherParsersTypes...>(ctx, errorMsg); node)
                {
                    ctx->getErrorCollector()->exitScope(ErrorHandleType::Discard);

                    std::shared_ptr<SourceReference> merged =
                            ctx->getSourceManager()->mergeReferences(ctx->getCurrentMultiReference());
                    node->setSourceRef(std::move(merged));

                    ctx->endMultiSourceRef();
                    return std::move(node);
                }
            }

            // If the last parser fails, we must log the error we were given
            if constexpr (sizeof...(OtherParsersTypes) == 0)
            {
                if (!errorMsg.isEmpty())
                {
                    std::shared_ptr<SourceReference> ref = ctx->canPeek() ? ctx->peek().m_sourceReference : nullptr;
                    ctx->getErrorCollector()->error(errorMsg, std::move(ref));
                }
            }

            ctx->getErrorCollector()->exitScope(ErrorHandleType::Propagate);
            ctx->endMultiSourceRef();
        }
        return nullptr;
    }
};

#endif // EZPACKER_ASTNODEPARSINGUTILITIES_H
