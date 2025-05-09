#ifndef EZPACKER_BASICTOKENIZER_H
#define EZPACKER_BASICTOKENIZER_H

#include "EzIRBuilderCommon.h"
#include "ITokenizer.h"
#include "InstructionTokenTable.h"
#include "KeywordTokenMap.h"
#include "TypeTokenMap.h"

struct TokenInformation
{
    IRTokenType m_type; // Type of the token.
    size_t m_col;       // Col inside m_line this token was parsed at.
    size_t m_line;      // Line of the input buffer this token was parsed at.
    std::string m_str;
};

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
     * Adds a set of valid instructions to tokenize them. If a buffer is tokenized, TokenType of keywords of this type
     * will be set to IRTokenType::Instruction
     * @param instructions
     */
    void addInstructionTable(const std::set<std::string> &instructions);
    
    /**
     * Adds a set of valid keywords to tokenize them. If a buffer is tokenized, TokenType of keywords of this type
     * will be set depending on the keyword.
     * @param keywords
     */
    void addKeywordMap(const std::map<std::string, IRTokenType> &keywords);
    
    /**
     * Adds a set of valid types to tokenize them. If a buffer is tokenized, TokenType of keywords of this type
     * will be set depending on the type.
     * @param types
     */
    void addTypeMap(const std::map<std::string, IRTokenType> &types);
    
    /**
     * Returns the list of tokens.
     * @return std::vector<TokenInformation>.
     */
    const std::vector<TokenInformation> &getTokens() const;
    
  private:
    /**
     * Tries to read the input buffer at given address to match input in a SINGLE-FIRST-OCURRENCE TokenType and returns
     * the number of bytes used to identify the token (should be used in the loop calling this function to increment
     * buffer starting address). Returns 0 if there was any error.
     * @param buffer
     * @param address
     * @param bufferSize
     * @param tokenType
     * @return size_t
     */
    size_t tokenize(char *buffer, size_t address, size_t bufferSize, IRTokenType &tokenType) override;

    /**
     * Tokenizes as a comment current position of buffer at given address. Returns of length of tokenized content.
     * @param buffer
     * @param address
     * @param bufferSize
     * @param token
     * @return size_t
     */
    size_t tokenizeComment(char *buffer, size_t address, size_t bufferSize, IRTokenType &token);
    
    /**
     * Tokenizes as an identifier current position of buffer at given address. Returns of length of tokenized content.
     * @param buffer
     * @param address
     * @param bufferSize
     * @param token
     * @return size_t
     */
    size_t tokenizeIdentifier(char *buffer, size_t address, size_t bufferSize, IRTokenType &token);
    
    /**
     * Tokenizes as an indentifier current position of buffer at given address. Returns of length of tokenized content.
     * @param buffer
     * @param address
     * @param bufferSize
     * @param token
     * @return size_t
     */
    size_t tokenizeNumber(char *buffer, size_t address, size_t bufferSize, IRTokenType &token);
    
    /**
     * Tokenizes as a string current position of buffer at given address. Returns of length of tokenized content.
     * If multi line strings are required, \ must be used after close-quotation mark. '\n' and '\r' are supported.
     * @param buffer
     * @param address
     * @param bufferSize
     * @param token
     * @return size_t
     */
    size_t tokenizeString(char *buffer, size_t address, size_t bufferSize, IRTokenType &token);
    
    /**
     * Logs an error when tokenizing and prints relevant information.
     */
    void logTokenizerError(const LogMessage &logMessage);
    
    /**
     * Sets a new col in m_col.
     */
    void newCol();
    
    /**
     * Sets a new line in m_line and resets m_col to 0.
     */
    void newLine();
    
  private:
    size_t m_col;
    size_t m_line;
    std::map<std::string, IRTokenType> m_keywords;
    std::set<std::string> m_instructions;
    std::map<std::string, IRTokenType> m_types;
    std::vector<TokenInformation> m_tokens;
};

#endif // EZPACKER_BASICTOKENIZER_H
