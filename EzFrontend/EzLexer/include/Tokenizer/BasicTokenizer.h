#ifndef EZPACKER_BASICTOKENIZER_H
#define EZPACKER_BASICTOKENIZER_H

#include "EzLexerCommon.h"
#include "ITokenizer.h"
#include "Logger/SourceLoggingSink.h"

/**
 * Basic SYNCHRONOUS tokenizer class that implements tokenizeBuffer method from ITokenizer. It also implements a higher
 * level tokenizeBuffer method that tokenizers the entire input.
 *
 * IMPROVE: If in a future it's wanted to make the input reader ASYNC, a new "ITokenizerContext" class should be
 * created that would be inherited by a sync tokenizer and an async tokenizer. ITokenizerContext should wrap methods
 * used in the tokenizing process and that are related to the input itself: peek, consume, ...
 */
class BasicTokenizer : public ITokenizer
{
  public:
    /**
     * Creates the object with the given sourceManager, logger and links it to a source file.
     * @param logger
     * @param sourceManager
     * @param source
     */
    BasicTokenizer(const std::shared_ptr<SourceLoggingSink> &logger,
                   const std::shared_ptr<SourceManager> &sourceManager,
                   const std::string &source);

    /**
     * Tries to read the input buffer, starting at pos = address, and generate a set of tokens. Returns true if there
     * wasn't any error with the input while tokenizing. If buffer is invalid or if address >= bufferSize, false is
     * returned.
     * @param buffer
     * @param address
     * @param bufferSize
     * @return bool
     */
    bool tokenizeBuffer(char *buffer, size_t address, size_t bufferSize) override;

    /**
     * Returns the list of generated tokens.
     * @return std::vector<TokenInformation>.
     */
    const std::vector<TokenInformation> &getTokens() const override;
    
  private:
    /**
     * Consumes current character of the input by incrementing address. Returns true if there's more input remaining,
     * false other ways.
     * @return bool
     */
    bool consume();

    /**
     * Returns the current character at buffer+address. If address >= bufferSize, \0 is returned.
     * @return char
     */
    char peek() const;

    /**
     * Tries to read the input buffer, starting at pos 0, to match a SINGLE-FIRST-OCCURRENCE _TokenType and
     * returns true if a token was identified.
     * @param buffer
     * @param bufferSize
     * @param token
     * @return bool
     */
    bool tokenizeSingle(char *buffer, size_t bufferSize, _TokenType &tokenType);

    /**
     * Tokenizes as an identifier the current position of buffer at given address. Returns of true if succeeded.
     * @param buffer
     * @param bufferSize
     * @param token
     * @return size_t
     */
    bool tokenizeIdentifier(char *buffer, size_t bufferSize, _TokenType &token);

    /**
     * Tokenizes as a number at the current position of buffer at given address. Returns of true if succeeded and, if
     * signed is set, it will need the FIRST character of current buffer position to be a '-'.
     * @param _signed
     * @param buffer
     * @param bufferSize
     * @param token
     * @return size_t
     */
    bool tokenizeNumber(bool _signed, char *buffer, size_t bufferSize, _TokenType &token);

  private:
    char *m_buffer;
    size_t m_address;
    size_t m_bufferSize;
    size_t m_col;
    size_t m_line;
    std::shared_ptr<SourceLoggingSink> m_logger;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::string m_source;
    std::vector<TokenInformation> m_tokens;
};

#endif // EZPACKER_BASICTOKENIZER_H
