#ifndef EZPACKER_ITOKENIZER_H
#define EZPACKER_ITOKENIZER_H

#include "EzLexerCommon.h"
#include "SourceManager/SourceManager.h"

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
    Identifier,  // Something formed with [a-z] | [A-Z] | [0, 9] | [_]. It can't start with digits.
    LeftBrace,   // '{'
    LeftParen,   // ')'
    NewLine,     // '\n'
    NumberInt,   // Something formed with [0-9]
    NumberFloat, // Something formed with [0-9].[0-9]
    Percentage,  // '%'
    RightBrace,  // '}'
    RightParen,  // '('
    SemiColon,   // ';'
    String,      // "..." Multiline strings are not supported. // TODO: Add support for multiline strings.
    WhiteSpace   // ' '
};

inline std::map<_TokenType, const char *> TokenType2StrMap = {
    { _TokenType::Invalid, "Invalid" },
    { _TokenType::Dot, "Dot" },
    { _TokenType::Colon, "Colon" },
    { _TokenType::Comma, "Comma" },
    { _TokenType::Comment, "Comment" },
    { _TokenType::Identifier, "Identifier" },
    { _TokenType::LeftBrace, "LeftBrace" },
    { _TokenType::LeftParen, "LeftParen" },
    { _TokenType::NewLine, "NewLine" },
    { _TokenType::NumberInt, "NumberInt" },
    { _TokenType::NumberFloat, "NumberFloat" },
    { _TokenType::Percentage, "Percentage" },
    { _TokenType::RightBrace, "RightBrace" },
    { _TokenType::RightParen, "RightParen" },
    { _TokenType::SemiColon, "SemiColon" },
    { _TokenType::String, "String" },
    { _TokenType::WhiteSpace, "WhiteSpace" },
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
 * Interface that defines the very basic skeleton of a tokenizer. Made an interface so there can be different types of
 * tokenizers.
 */
class ITokenizer
{
  public:
    virtual ~ITokenizer() = default;

    /**
     * Tries to read the input buffer, starting at pos 0, to match a SINGLE-FIRST-OCCURRENCE _TokenType and
     * returns true if a token was identified.
     * @param buffer
     * @param bufferSize
     * @param token
     * @return bool
     */
    virtual bool tokenize(char *buffer, size_t bufferSize, _TokenType &token) = 0;

  private:
};

#endif // EZPACKER_ITOKENIZER_H
