#ifndef EZPACKER_BASICTOKENIZER_H
#define EZPACKER_BASICTOKENIZER_H

#include "EzFrontendCommon.h"
#include "ITokenizer.h"

class BasicTokenizer : public ITokenizer, public LogSink
{
  public:
    /**
     * Creates the object.
     */
    BasicTokenizer();

    /**
     * Destroys the object and released resources.
     */
    ~BasicTokenizer() override;

    /**
     * Tries to read the input buffer and generate a set of tokens. Returns true if there wasn't any error with the
     * input while tokenizing. If buffer is invalid or if address >= bufferSize, false is returned.
     * @param buffer
     * @param address
     * @param bufferSize
     * @return
     */
    bool tokenize(char *buffer, size_t address, size_t bufferSize);

    /**
     * Returns the list of tokens.
     * @return std::vector<TokenInformation>.
     */
    const std::vector<TokenInformation> &getTokens() const;

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
    char peek();

    /**
     * Tries to read the input buffer to match input in a SINGLE-FIRST-OCURRENCE TokenType and returns
     * the number of bytes used to identify the token (should be used in the loop calling this function to increment
     * buffer starting address). Returns false if there was any error.
     * @param buffer
     * @param bufferSize
     * @param tokenType
     * @return size_t
     */
    bool tokenize(char *buffer, size_t bufferSize, IRTokenType &tokenType) override;

    /**
     * Tokenizes as an identifier current position of buffer at given address. Returns of true if succeeded.
     * @param buffer
     * @param bufferSize
     * @param token
     * @return size_t
     */
    bool tokenizeIdentifier(char *buffer, size_t bufferSize, IRTokenType &token);

    /**
     * Tokenizes as an identifier current position of buffer at given address. Returns of true if succeeded.
     * @param buffer
     * @param bufferSize
     * @param token
     * @return size_t
     */
    bool tokenizeNumber(char *buffer, size_t bufferSize, IRTokenType &token);
    
    /**
     * Logs an error when tokenizing and prints relevant information.
     */
    void logTokenizerError(const LogMessage &logMessage);

    /**
     * Sets m_lastTokenLine & m_lastTokenCol to current m_col and m_line. Used to log properly.
     */
    void tokenLogCheckPoint();
    
  private:
    char *m_buffer;
    size_t m_address;
    size_t m_bufferSize;
    size_t m_col;
    size_t m_lastTokenLine; // Used not to have log showing col & line after the end of the token.
    size_t m_lastTokenCol;  // Used not to have log showing col & line after the end of the token.
    size_t m_line;
    std::vector<TokenInformation> m_tokens;
};

#endif // EZPACKER_BASICTOKENIZER_H
