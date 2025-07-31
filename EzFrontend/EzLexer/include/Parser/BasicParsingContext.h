#ifndef EZPACKER_BASICPARSINGCONTEXT_H
#define EZPACKER_BASICPARSINGCONTEXT_H

#include "EzLexerCommon.h"
#include "IParsingContext.h"
#include "Rule.h"

/**
 * This class models a basic SYNCHRONOUS context that implements the methods of IParsingContext.
 */
class BasicParsingContext : public IParsingContext
{
  public:
    /**
     * Creates the object.
     * @param logger
     * @param sourceManager
     * @param tokens
     */
    BasicParsingContext(const std::shared_ptr<SourceLoggingSink> &logger,
                        const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Advances the token vector to the next element, while it's within vector's bounds. Returns true if current token
     * was consumed.
     * @return bool.
     */
    bool consume() override;

    /**
     * Advances the token vector to the next element only if current token's type matches the given type.Returns true if
     * current token was consumed. If an error succeeded, a log error will be pushed into the error collector.
     * @return bool.
     */
    bool consumeIfToken(_TokenType token) override;

    /**
     * Tries to restore vector's position to pos if it's within token vector's bounds. Returns true if succeeded.
     * @param pos
     * @return bool
     */
    bool restore(size_t pos) override;

    /**
     * Returns the current token in the buffer. If current element id is outside vector's bounds, it returns a token
     * with a '_TokenType::Invalid' type.
     * @return const TokenInformation &
     */
    const TokenInformation &peek() const override;

    /**
     * Returns the current position of the buffer.
     * @return size_t
     */
    size_t getPosition() const override;

    /**
     * Sets tokens for this context. Make sure this context is not being used before setting tokens!
     * @param tokens
     */
    void setTokens(const std::vector<TokenInformation> &tokens);

    /**
     * Consumes tokens until the current token is not a white space, a new line or a comment.
     */
    void skipUselessTokens() override;

    /**
     * Returns the error collector linked to this context.
     * @return std::shared_ptr<ErrorCollector> &
     */
    const std::shared_ptr<ErrorCollector> &getErrorCollector() const override;

  private:
    size_t m_position;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<SourceLoggingSink> m_logger;
    std::vector<TokenInformation> m_tokens;
};

#endif // EZPACKER_BASICPARSINGCONTEXT_H
