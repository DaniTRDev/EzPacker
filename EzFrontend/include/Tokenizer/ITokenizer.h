#ifndef EZPACKER_ITOKENIZER_H
#define EZPACKER_ITOKENIZER_H

#include "EzFrontendCommon.h"
#include "SourceManager/SourceManager.h"

namespace TokenizerHelpers
{
/**
 * Returns true if ch is a '\' .
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isBackSlash = [](char ch) -> bool { return (ch == '\\'); };
/**
 * Returns true if ch is a '.' .
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isComma = [](char ch) -> bool { return (ch == ','); };

/**
 * Returns true if ch is a digit (0-9).
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isDigit = [](char ch) -> bool { return (ch >= '0' && ch <= '9'); };

/**
 * Returns true if ch is a '.' .
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isDot = [](char ch) -> bool { return (ch == '.'); };

/**
 * Returns true if ch is '%'.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isDoubleDot = [](char ch) -> bool { return ch == ':'; };

/**
 * Returns true if ch is end of line.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isEndOfLine = [](char ch) -> bool { return (ch == '\n') || (ch == '\r'); };

/**
 * Returns true if ch is a '#' .
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isHashtag = [](char ch) -> bool { return (ch == '#'); };

/**
 * Returns true if ch is '('.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isLeftParen = [](char ch) -> bool { return ch == '('; };

/**
 * Returns true if ch is a letter (a-z or A-Z).
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isLetter = [](char ch) -> bool {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
};

/**
 * Returns true if ch is a '"' .
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isQuote = [](char ch) -> bool { return (ch == '"'); };

/**
 * Returns true if ch is '%'.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isPercentage = [](char ch) -> bool { return ch == '%'; };

/**
 * Returns true if ch is ')'.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isRightParen = [](char ch) -> bool { return ch == ')'; };

/**
 * Returns true if ch is a special character: - or _.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isSpecial = [](char ch) -> bool { return (ch == '_'); };

/**
 * Returns true if ch is white space.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isWhiteSpace = [](char ch) -> bool { return (ch == ' '); };
}; // namespace TokenizerHelpers

/**
 * Enum to indicate different types of tokens.
 */
enum IRTokenType : uint8_t
{
    Invalid = 0,
    Dot,
    Colon,       // ':'
    Comma,       //',' Used to parse lists. And operands for instructions.
    Comment,     // # ...
    Identifier,  // Something formed with [a-z] | [A-Z] | [0, 9] | [-_]. It can't start with digits.
    LeftParen,   // ')'
    NewLine,     // '\n'
    NumberInt,   // Something formed with [0-9]
    NumberFloat, // Something formed with [0-9].[0-9]
    Percentage,  // '%'
    RightParen,  // '('
    String,      // "..." Multiline strings are not supported, yet.
    WhiteSpace   // ' '
};

inline std::map<IRTokenType, const char *> g_IRTokenTypeStr = {
    {IRTokenType::Invalid, "Invalid"},       {IRTokenType::Dot, "Dot"},
    {IRTokenType::Colon, "Colon"},           {IRTokenType::Comma, "Comma"},
    {IRTokenType::Comment, "Comment"},       {IRTokenType::Identifier, "Identifier"},
    {IRTokenType::LeftParen, "LeftParen"},   {IRTokenType::NewLine, "NewLine"},
    {IRTokenType::NumberInt, "NumberInt"},   {IRTokenType::NumberFloat, "NumberFloat"},
    {IRTokenType::Percentage, "Percentage"}, {IRTokenType::RightParen, "RightParen"},
    {IRTokenType::String, "String"},         {IRTokenType::WhiteSpace, "WhiteSpace"},
};

/**
 * Structure contains information about the token.
 */
struct TokenInformation
{
    IRTokenType m_type; // Type of the token.
    std::shared_ptr<SourceReference> m_sourceReference;
    std::string m_str;
};

class ITokenizer
{
  public:
    virtual ~ITokenizer() = default;

  protected:
    /**
     * Tries to read the input buffer at given address to match input in a SINGLE-FIRST-OCCURRENCE TokenType and returns
     * the number of bytes used to identify the token (should be used in the loop calling this function to increment
     * buffer starting address). Returns false if there was any error.
     * @param buffer
     * @param address
     * @param bufferSize
     * @param token
     * @return size_t
     */
    virtual bool tokenize(char *buffer, size_t bufferSize, IRTokenType &token) = 0;

  private:
};

#endif // EZPACKER_ITOKENIZER_H
