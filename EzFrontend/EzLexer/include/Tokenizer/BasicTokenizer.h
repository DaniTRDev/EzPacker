/**
 * @file BasicTokenizer.h
 * @brief Synchronous tokenizer that splits raw source text into a stream of typed tokens.
 *
 * BasicTokenizer scans a character buffer and produces a sequence of
 * TokenInformation objects.  It recognises identifiers, reserved keywords
 * (`if`, `else`, `while`, `break`, `continue`), integer and floating-point
 * literals (decimal and hexadecimal), string literals with escape sequences,
 * single-character punctuation, and `#`-prefixed comments.  The resulting
 * token list is consumed by the parsing stage (BasicParsingContext).
 *
 * This file also defines the _TokenType enumeration (all possible token
 * kinds) and the TokenInformation struct that pairs a type with the
 * original text and source location.
 */
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
    Break,       // "break"
    Colon,       // ':'
    Comma,       //','
    Comment,     // # ...
    Continue,    // "continue"
    Dot,         // '.'
    Else,        // "else"
    Identifier,  // Something formed with [a-z] | [A-Z] | [0, 9] | [_]. It can't start with digits.
    If,          // "if"
    Include,     // "include"
    LeftBrace,   // '{'
    LeftParen,   // '('
    Minus,       // '-'
    NewLine,     // '\n'
    NumberInt,   // Something formed with [0-9]
    NumberFloat, // Something formed with [0-9].[0-9]
    Percentage,  // '%'
    Plus,        // '+'
    RightBrace,  // '}'
    RightParen,  // ')'
    SemiColon,   // ';'
    String,      // "..." Multiline strings are not supported. // TODO: Add support for multiline strings.
    Tab,         // '\t'
    While        //"while"
};

inline std::map<_TokenType, const char *> TokenType2StrMap = {
    { _TokenType::Invalid, "Invalid" },
    { _TokenType::Break, "Break" },
    { _TokenType::Colon, "Colon" },
    { _TokenType::Comma, "Comma" },
    { _TokenType::Comment, "Comment" },
    { _TokenType::Continue, "Continue" },
    { _TokenType::Dot, "Dot" },
    { _TokenType::Else, "Else" },
    { _TokenType::Identifier, "Identifier" },
    { _TokenType::If, "If" },
    { _TokenType::LeftBrace, "LeftBrace" },
    { _TokenType::LeftParen, "LeftParen" },
    { _TokenType::Minus, "Minus" },
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
    SourceReference m_sourceReference;
    std::string m_str;
};

/**
 * Basic SYNCHRONOUS tokenizer class. Given a character buffer and a starting address, it scans the input
 * and produces a sequence of TokenInformation objects. Whitespace and newlines are consumed but not emitted
 * as tokens (except for tracking line/column information). Comments (starting with '#') are preserved as
 * Comment tokens. Strings support common escape sequences (\\n, \\r, \\t, \\\\, \\', \\", \\0).
 *
 * After tokenization, the resulting tokens can be retrieved via getTokens().
 */
class BasicTokenizer
{
  public:
    /**
     * Creates the object with the given errorCollector and sourceManager.
     * @param errorCollector
     * @param sourceManager
     */
    BasicTokenizer(const std::shared_ptr<ErrorCollector> &errorCollector,
                   const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Tries to get the content of the given source file and tokenize it. Returns true if succeeded, false other ways.
     * The generated tokens can be retrieved via getTokens(). The address parameter indicates the starting position in
     * the buffer for tokenization.
     * @param address
     * @param sourceId
     * @return bool
     */
    bool tokenizeBuffer(size_t address, size_t sourceId);

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
     * Tokenizes an identifier at the current position of buffer at given address. An identifier starts with a letter
     * (a-z, A-Z) or underscore ('_'), followed by letters, digits, or underscores. Reserved keywords ("if", "else",
     * "while") are recognized and assigned their corresponding token types. Returns true if succeeded.
     * @param buffer
     * @param bufferSize
     * @param token
     * @return bool
     */
    bool tokenizeIdentifier(char *buffer, size_t bufferSize, _TokenType &token);

    /**
     * Tokenizes a number (integer or float) at the current position of buffer at given address.
     * Returns true if succeeded. Supports decimal and hexadecimal (0x...) integer formats, as well
     * as floating-point numbers containing a single '.'. Sign characters (+/-) are NOT handled here;
     * they are tokenized separately as Plus/Minus tokens.
     * @param buffer
     * @param bufferSize
     * @param token
     * @return bool
     */
    bool tokenizeNumber(char *buffer, size_t bufferSize, _TokenType &token);

  private:
    char *m_buffer;
    size_t m_address;
    size_t m_bufferSize;
    size_t m_col;
    size_t m_line;
    size_t m_sourceId;
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::vector<TokenInformation> m_tokens;
};

#endif // EZPACKER_BASICTOKENIZER_H
