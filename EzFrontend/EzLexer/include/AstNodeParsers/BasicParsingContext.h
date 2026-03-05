 /**
 * @file BasicParsingContext.h
 * @brief Shared state for all parsers: the token stream, position cursor,
 *        node pool, string pool, and conditional-consume helpers.
 *
 * Every parser receives a BasicParsingContext and reads tokens through
 * peek() / consume() / consumeIf().  Successfully parsed nodes are
 * allocated from the internal AstNodeTypedPool so they share a single
 * cache-friendly arena.  ParsingCondition provides pre-built lambda
 * predicates (match-by-type, match-by-content) used with consumeIf().
 */
#ifndef EZPACKER_SINGLETHREADPARSER_H
#define EZPACKER_SINGLETHREADPARSER_H

#include "AstNode/AstNodeTypedPool.h"
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
     * Returns the pool of nodes.
     * @return TypedPool *
     */
    AstNodeTypedPool *getNodePool();

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
     * Returns the string pool.
     * @return StringPool*
     */
    StringPool *getStringPool();

    /**
     * Returns the last valid source reference.
     * @return const SourceReference &
     */
    const SourceReference &getLastSourceReference() const;

    /**
     * Peeks the current context without consuming the token. canPeek must have returned true.
     * @return const TokenInformation &
     */
    const TokenInformation &peek() const;

    /**
     * Advances the stream position by 1 if canPeek() returns true. Updates the last source reference to the
     * consumed token's reference. If the next token after advancing is a Comment, it is automatically skipped
     * (consumed recursively).
     */
    void consume();

    /**
     * Sets the position of the stream to the one given, if pos is invalid an exception is thrown.
     */
    void setPosition(size_t pos);

  private:
    AstNodeTypedPool m_nodePool; // Used to store AstNode objects in memory. This is cache-friendly.
    size_t m_currentPos;
    StringPool m_stringPool; // Used to store strings in memory. This is cache-friendly.
    SourceReference m_lastSourceRef;
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

        while (ctx->canPeek() && ctx->peek().m_type == _TokenType::Comment)
            ctx->consume();

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
