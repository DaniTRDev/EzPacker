#ifndef EZPACKER_BASICTOKENIZER_H
#define EZPACKER_BASICTOKENIZER_H

#include "EzLexerCommon.h"
#include "Logger/SourceLoggingSink.h"

namespace TokenizerHelpers
{
/**
 * Returns true if ch is a digit (0-9).
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isDigit = [](char ch) -> bool { return (ch >= '0' && ch <= '9'); };

/**
 * Returns true if ch is end of line.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isEndOfLine = [](char ch) -> bool { return (ch == '\n') || (ch == '\r'); };

/**
 * Returns true if ch is a letter (a-z or A-Z).
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isLetter = [](char ch) -> bool
{ return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); };

/**
 * Returns true if ch is a special character: At the moment only '_'.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isSpecial = [](char ch) -> bool { return (ch == '_'); };
}; // namespace TokenizerHelpers

/**
 * Enum to indicate different types of tokens.
 */
enum class _TokenType : uint8_t
{
    Invalid = 0,
    Dot,         // '.'
    Colon,       // ':'
    Comma,       //','
    Comment,     // # ...
    Else,        // "else"
    Identifier,  // Something formed with [a-z] | [A-Z] | [0, 9] | [_]. It can't start with digits.
    If,          // "if"
    LeftBrace,   // '{'
    LeftParen,   // ')'
    Minus,       // '-'
    NewLine,     // '\n'
    NumberInt,   // Something formed with [0-9]
    NumberFloat, // Something formed with [0-9].[0-9]
    Percentage,  // '%'
    Plus,        // '+'
    RightBrace,  // '}'
    RightParen,  // '('
    SemiColon,   // ';'
    String,      // "..." Multiline strings are not supported. // TODO: Add support for multiline strings.
    While        //"while"
};

inline std::map<_TokenType, const char *> TokenType2StrMap = {
    { _TokenType::Invalid, "Invalid" },
    { _TokenType::Dot, "Dot" },
    { _TokenType::Colon, "Colon" },
    { _TokenType::Comma, "Comma" },
    { _TokenType::Comment, "Comment" },
    { _TokenType::Else, "Else" },
    { _TokenType::Identifier, "Identifier" },
    { _TokenType::If, "If" },
    { _TokenType::LeftBrace, "LeftBrace" },
    { _TokenType::LeftParen, "LeftParen" },
    { _TokenType::NewLine, "NewLine" },
    { _TokenType::NumberInt, "NumberInt" },
    { _TokenType::NumberFloat, "NumberFloat" },
    { _TokenType::Percentage, "Percentage" },
    { _TokenType::Plus, "Plus" },
    { _TokenType::RightBrace, "RightBrace" },
    { _TokenType::RightParen, "RightParen" },
    { _TokenType::SemiColon, "SemiColon" },
    { _TokenType::String, "String" },
    { _TokenType::While, "While" },
};

/**
 * Structure contains information about the token.
 */
struct TokenInformation
{
    _TokenType m_type; // Type of the token.
    std::shared_ptr<SourceReference> m_sourceReference;
    std::string m_str;
};

/**
 * Basic SYNCHRONOUS tokenizer class that implements tokenizeBuffer method. It also implements a higher
 * level tokenizeBuffer method that tokenizers the entire input.
 *
 */
class BasicTokenizer
{
  public:
    /**
     * Creates the object with the given errorCollector, sourceManager and links it to a source file.
     * @param errorCollector
     * @param sourceManager
     * @param source
     */
    BasicTokenizer(const std::shared_ptr<ErrorCollector> &errorCollector,
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
    bool tokenizeBuffer(char *buffer, size_t address, size_t bufferSize);

    /**
     * Returns the list of generated tokens.
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
    bool tokenizeNumber(char *buffer, size_t bufferSize, _TokenType &token);

  private:
    char *m_buffer;
    size_t m_address;
    size_t m_bufferSize;
    size_t m_col;
    size_t m_line;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::string m_source;
    std::vector<TokenInformation> m_tokens;
};

#endif // EZPACKER_BASICTOKENIZER_H
