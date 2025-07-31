#ifndef EZPACKER_IPARSINGCONTEXT_H
#define EZPACKER_IPARSINGCONTEXT_H

#include "ErrorCollector/ErrorCollector.h"
#include "EzLexerCommon.h"
#include "Tokenizer/BasicTokenizer.h"

/**
 * This interface the basic functionality a parsing context should provide. This allows creating async parsers or
 * other types of parsers without affecting upper logic.
 */
class IParsingContext
{
  public:
    virtual ~IParsingContext() = default;

    /**
     * Advances the token vector to the next element, while it's within vector's bounds. Returns true if current token
     * was consumed.
     * @return bool.
     */
    virtual bool consume() = 0;

    /**
     * Advances the token vector to the next element only if current token's type matches the given type.Returns true if
     * current token was consumed. If an error succeeded, a log error will be pushed into the error collector.
     * @return bool.
     */
    virtual bool consumeIfToken(_TokenType token) = 0;

    /**
     * Tries to restore vector's position to pos if it's within token vector's bounds. Returns true if succeeded.
     * @param pos
     * @return bool
     */
    virtual bool restore(size_t pos) = 0;

    /**
     * Returns the current token in the buffer. If current element id is outside vector's bounds, it returns a token
     * with a '_TokenType::Invalid' type.
     * @return const TokenInformation &
     */
    virtual const TokenInformation &peek() const = 0;

    /**
     * Returns the current position of the buffer.
     * @return size_t
     */
    virtual size_t getPosition() const = 0;

    /**
     * Consumes tokens that should not be taken in account for generating the AST. Depends on each context.
     */
    virtual void skipUselessTokens() = 0;

    /**
     * Returns the error collector linked to this context.
     * @return std::shared_ptr<ErrorCollector> &
     */
    virtual const std::shared_ptr<ErrorCollector> &getErrorCollector() const = 0;
};

#endif // EZPACKER_IPARSINGCONTEXT_H
