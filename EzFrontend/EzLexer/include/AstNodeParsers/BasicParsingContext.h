#ifndef EZPACKER_SINGLETHREADPARSER_H
#define EZPACKER_SINGLETHREADPARSER_H

#include "ErrorCollector/ErrorCollector.h"
#include "Tokenizer/BasicTokenizer.h"

template <typename... Args>
using ParsingConditionType =
        std::function<bool(class BasicParsingContext *ctx, TokenInformation *outToken, Args &&...args)>;

class BasicParsingContext : public ErrorEmitter
{
  public:
    /**
     * Creates the parsing context with the given error collector, source manager and token array.
     * @param errorCollector
     * @param sourceManager
     * @param tokens
     */
    BasicParsingContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                        const std::shared_ptr<SourceManager> &sourceManager,
                        std::vector<TokenInformation> tokens);

    /**
     * Returns true if the current token stream can be peeked.
     * @return bool
     */
    bool canPeek() const;

    /**
     * If condition is met, true is returned, consume is called and if out token != nullptr, it will be set to the
     * current token (before consume call).
     * @param condition
     * @param outToken
     * @tparam Args
     * @return bool
     */
    template <typename... Args>
    bool consumeIf(ParsingConditionType<Args...> &condition, TokenInformation *outToken, Args &&...args)
    {
        return condition(this, outToken, std::forward<Args>(args)...);
    }

    /**
     * Returns the current position in the token stream.
     * @return size_t
     */
    size_t getCurrentPosition() const;

    /**
     * Returns the number of tokens left to parse.
     * @return size_t
     */
    size_t getRemainingTokenCount() const;

    /**
     * Peeks the current context without consuming the token. canPeek must habe returned true.
     * @return const std::shared_ptr<TokenInformation> &
     */
    const TokenInformation &peek() const;

    /**
     * Begins a new multiple source reference that will be filled with "consumed nodes".
     */
    void beginMultiSourceRef();

    /**
     * Advances the stream position by 1 if canPeek didn't return false. Will push the consumed node into the
     * current "merged" source reference for the caller AstNode. If no multi source ref was pushed, an access violation
     * is thrown.
     */
    void consume();

    /**
     * Ends the current multiple source reference.
     */
    void endMultiSourceRef();

    /**
     * Sets the position of the stream to the one given, if pos is invalid an exception is thrown.
     */
    void setPosition(size_t pos);

    /**
     * Returns the last valid source reference.
     * @return const std::shared_ptr<SourceReference> &
     */
    const std::shared_ptr<SourceReference> &getLastSourceReference() const;

    /**
     * Returns the current multiple reference.
     * @return std::vector<std::shared_ptr<SourceReference>>
     */
    std::vector<std::shared_ptr<SourceReference>> getCurrentMultiReference() const;

  private:
    size_t m_currentPos;
    std::shared_ptr<SourceReference> m_lastSourceRef;
    std::stack<std::vector<std::shared_ptr<SourceReference>>> m_multiSourceRefs;
    std::vector<TokenInformation> m_tokens;
};

struct ParsingCondition
{
    friend class BasicParsingContext;
    /**
     * This condition checks if the current token (if any) is of the given type. If so returns true and sets outToken
     * to the current token before consuming it.
     */
    inline static ParsingConditionType<_TokenType> TokenType =
            [](class BasicParsingContext *ctx, TokenInformation *outToken, _TokenType type) -> bool
    {
        if (!ctx || !ctx->canPeek())
        {
            return false;
        }

        if (ctx->peek().m_type == type)
        {
            if (outToken)
            {
                *outToken = ctx->peek();
            }

            ctx->consume();
            return true;
        }

        return false;
    };

    /**
     * This condition checks if the current token content (if any) matches the string given. If so returns true and sets
     * outToken to the current token before consuming it.
     */
    inline static ParsingConditionType<const std::string &> TokenContent =
            [](class BasicParsingContext *ctx, TokenInformation *outToken, const std::string &content) -> bool
    {
        if (!ctx || !ctx->canPeek())
        {
            return false;
        }

        if (ctx->peek().m_str == content)
        {
            if (outToken)
            {
                *outToken = ctx->peek();
            }

            ctx->consume();
            return true;
        }

        return false;
    };
};

#endif // EZPACKER_SINGLETHREADPARSER_H
