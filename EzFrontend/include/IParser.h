#ifndef EZPACKER_IPARSER_H
#define EZPACKER_IPARSER_H

#include "EzFrontendCommon.h"
#include "tokenizer/BasicTokenizer.h"

/**
 * Interface that represents a generic parser. It should receive an array of tokens and it should convert them to
 * architecture-specific terms (instructions, operands, ...), build the AST and check the grammar with ParseRules.
 */
class IParser
{
  public:
    virtual ~IParser() = default;

    /**
     * Advances the token vector to the next element, while it's within vector's bounds. Returns true if current token
     * was consumed.
     * @return bool.
     */
    virtual bool consume() = 0;

    /**
     * Advances the token vector to the next element only if current token is of given token, while it's within vector's
     * bounds. Returns true if current token was consumed. If an error succeeded, a log error will be shown.
     * @return bool.
     */
    virtual bool consumeIfToken(IRTokenType token) = 0;
    
    /**
     * Tries to restore vector's position to pos. Returns true if succeeded.
     * @param pos
     * @return bool
     */
    virtual bool restore(size_t pos) = 0;

    /**
     * Returns the current token in the buffer. If current element id is outside vector's bounds, it returns a token
     * with an TokenType::Invalid type.
     * @return const TokenInformation &
     */
    virtual const TokenInformation &peek() const = 0;

    /**
     * Returns the current position of the buffer.
     * @return size_t
     */
    virtual size_t getPosition() const = 0;
    
    /**
     * Begins a new log block.
     */
    virtual void beginLogBlock() = 0;

    /**
     * Finishes the log block, and logs it if commit is set to true.
     */
    virtual void endLogBlock(bool commit) = 0;

    /**
     * Logs an error related to parsing with detailed information.
     * @param msg
     * @param token
     */
    virtual void logParserError(const LogMessage &msg, const TokenInformation &token) = 0;

    /**
     * Skips every white space or new line and consumes tokens.
     */
    virtual void skipWhiteSpacesAndNewLines() = 0;
};

#endif // EZPACKER_IPARSER_H
