#ifndef EZPACKER_BASICPARSER_H
#define EZPACKER_BASICPARSER_H

#include "EzFrontendCommon.h"
#include "IParser.h"
#include "ParseRule.h"

/**
 * Simple parser that can consume, peek and restore the vector of tokens.
 */
class BasicParser : public IParser, public LogSink
{
  public:
    friend class ParseRule;
    /**
     * Creates the object.
     * @param registerMap
     * @param tokens
     */
    BasicParser(const std::vector<TokenInformation> &tokens);

    /**
     * Destroys the object and releases resources.
     */
    ~BasicParser() override;

    /**
     * Advances the token vector to the next element, while it's within vector's bounds. Returns true if current token
     * was consumed.
     * @return bool.
     */
    bool consume() override;

    /**
     * Advances the token vector to the next element only if current token is of given token, while it's within vector's
     * bounds. Returns true if current token was consumed.
     * @return bool.
     */
    bool consumeIfToken(IRTokenType token) override;
    
    /**
     * Tries to restore vector's position to pos. Returns true if succeeded.
     * @param pos
     * @return bool
     */
    bool restore(size_t pos) override;

    /**
     * Returns the current token in the buffer. If current element id is outside vector's bounds, it returns a token
     * with an TokenType::Invalid type.
     * @return const TokenInformation &
     */
    const TokenInformation &peek() const override;

    /**
     * Returns the current position of the buffer.
     * @return size_t
     */
    size_t getPosition() const override;
    
    /**
     * Begins a new log block.
     */
    void beginLogBlock() override;
    
    /**
     * Finishes the log block, and logs it if commit is set to true.
     */
    void endLogBlock(bool commit) override;
    
    /**
     * Logs an error related to parsing with detailed information.
     * @param msg
     * @param token
     */
    void logParserError(const LogMessage &msg, const TokenInformation &token) override;

    /**
     * Skips every white space or new line and consumes tokens.
     */
    void skipWhiteSpacesAndNewLines() override;
    
  private:
    size_t m_position;
    std::stack<std::queue<LogMessage>> m_logStack;
    std::vector<TokenInformation> m_tokens;
};

#endif // EZPACKER_BASICPARSER_H
