/**
 * @file BasicParsingContext.h
 * @brief Shared parsing state: token stream, cursor, node pool, string pool,
 *        and token-matching helpers.
 *
 * Every parser in EzLexer receives the same BasicParsingContext instance. This
 * keeps all parser attempts consistent: they observe the same token sequence,
 * allocate nodes from the same arena, and report diagnostics through the same
 * ErrorCollector/SourceManager pair.
 *
 * Important behavioral rules for parser authors and integrators:
 * - `peek()` never consumes input.
 * - `consume()` advances by one logical token and automatically skips trailing
 *   newlines and tabs.
 * - `consumeIf(...)` is the preferred way to match expected tokens because it
 *   keeps the common "match + optional capture + consume" pattern compact.
 * - Rollback is not automatic when calling a parser directly. That behavior is
 *   provided by ParserBatch.
 * - AST nodes and pooled strings returned by this context remain valid for as
 *   long as the context itself stays alive.
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
     * Creates a parsing context over an already-tokenized input stream.
     *
     * The token vector is moved into the context. All AST nodes created by
     * parsers working on this context will be allocated from the internal node
     * pool and therefore share the context's lifetime.
     */
    BasicParsingContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                        const std::shared_ptr<SourceManager> &sourceManager,
                        std::vector<TokenInformation> tokens);

    /**
     * Returns the arena used to allocate AST nodes for this parse session.
     *
     * Callers usually do not need to manage this directly unless they are
     * implementing a parser.
     */
    AstNodeTypedPool *getNodePool();

    /**
     * Returns true when the cursor still points to a valid token.
     */
    [[nodiscard]] bool canPeek() const;

    /**
     * Consumes the current token only when the given predicate succeeds.
     *
     * @param condition Matching function, usually one of ParsingCondition's
     *                  predefined helpers.
     * @param outToken Optional output copy of the consumed token.
     * @return true if the condition matched and the token was consumed.
     */
    template <typename... Args>
    bool consumeIf(ParsingConditionType<Args...> &condition, TokenInformation *outToken, Args &&...args)
    {
        return condition(this, outToken, std::forward<Args>(args)...);
    }

    /**
     * Returns the current zero-based token index.
     */
    [[nodiscard]] size_t getCurrentPosition() const;

    /**
     * Returns how many tokens are left from the current position to the end.
     */
    [[nodiscard]] size_t getRemainingTokenCount() const;

    /**
     * Returns the pool that owns canonical string storage for parsed names and
     * type spellings.
     */
    StringPool *getStringPool();

    /**
     * Returns the source reference of the last token consumed successfully.
     *
     * This is commonly used when a parser needs to report an error after it has
     * already advanced past the most relevant source token.
     */
    [[nodiscard]] const SourceReference &getLastSourceReference() const;

    /**
     * Returns the current token without consuming it.
     *
     * Precondition: canPeek() must be true.
     */
    [[nodiscard]] const TokenInformation &peek() const;

    /**
     * Advances to the next logical token.
     *
     * After consuming one token, the implementation also skips any immediately
     * following newline or tab tokens. This means most parsers can treat line
     * breaks as ordinary whitespace.
     */
    void consume();

    /**
     * Restores the cursor to a previously saved token position.
     *
     * @throws std::runtime_error if pos is outside the token array.
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
     * Matches the current token by exact token type.
     *
     * If the current token matches, it is copied into outToken (when provided)
     * and consumed before returning true.
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
     * Matches the current token by exact token text.
     *
     * This helper is useful for grammar fragments that are represented as
     * generic identifiers in the tokenizer but have keyword-like meaning at the
     * parser level, such as instruction mnemonics or condition operators.
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
