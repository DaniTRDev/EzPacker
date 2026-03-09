/**
 * @file BasicTokenizer.h
 * @brief Synchronous tokenizer that converts source text into parser-ready tokens.
 *
 * BasicTokenizer reads a source buffer owned by SourceManager and emits a flat
 * sequence of TokenInformation entries. The tokenizer is intentionally simple:
 * it does not build partial syntax, it only classifies source substrings and
 * preserves enough SourceReference data for later diagnostics.
 *
 * Observable behavior relevant to callers:
 * - Spaces, tabs, and line breaks are consumed and are not exposed by
 *   getTokens().
 * - `#` starts a comment that runs until the end of the current line; comments
 *   are skipped and are not exposed by getTokens().
 * - Negative numbers are tokenized as a separate `Minus` token followed by the
 *   numeric token.
 * - Strings are delimited by double quotes and support `\\n`, `\\r`, `\\t`,
 *   `\\\\`, `\\'`, `\\\"`, and `\\0`.
 * - On the first fatal tokenization error, tokenizeBuffer() returns false and
 *   reports the diagnostic through the shared ErrorCollector.
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
 *
 * Note: some entries (such as NewLine, Tab, and Comment) exist because the
 * tokenizer handles them internally, but the current implementation skips them
 * instead of returning them to callers through getTokens().
 */
enum class _TokenType : uint8_t
{
    Invalid = 0,
    Break,       // "break"
    Case,        // "case"
    Colon,       // ':'
    Comma,       //','
    Comment,     // # ...
    Continue,    // "continue"
    Default,     // "default"
    Dot,         // '.'
    Else,        // "else"
    GreaterThan, // '>'
    Identifier,  // Something formed with [a-z] | [A-Z] | [0, 9] | [_]. It can't start with digits.
    If,          // "if"
    Include,     // "include"
    For,         // "for"
    LeftBrace,   // '{'
    LeftParen,   // '('
    LowerThan,   // '<'
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
    Switch,      // "switch"
    Tab,         // '\t'
    While        //"while"
};

inline std::map<_TokenType, const char *> TokenType2StrMap = {
    { _TokenType::Invalid, "Invalid" },
    { _TokenType::Break, "Break" },
    { _TokenType::Case, "Case" },
    { _TokenType::Colon, "Colon" },
    { _TokenType::Comma, "Comma" },
    { _TokenType::Comment, "Comment" },
    { _TokenType::Continue, "Continue" },
    { _TokenType::Default, "Default" },
    { _TokenType::Dot, "Dot" },
    { _TokenType::Else, "Else" },
    { _TokenType::GreaterThan, "GreaterThan" },
    { _TokenType::Identifier, "Identifier" },
    { _TokenType::If, "If" },
    { _TokenType::LeftBrace, "LeftBrace" },
    { _TokenType::LeftParen, "LeftParen" },
    { _TokenType::LowerThan, "LowerThan" },
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
    { _TokenType::Switch, "Switch" },
    { _TokenType::Tab, "\\n" },
    { _TokenType::While, "While" },
};

/**
 * Structure contains information about a token emitted by the tokenizer.
 *
 * m_str stores the normalized token payload:
 * - identifiers/keywords keep their textual spelling,
 * - strings store the unescaped content without surrounding quotes,
 * - punctuation stores the matched single-character lexeme.
 */
struct TokenInformation
{
    _TokenType m_type; // Type of the token.
    SourceReference m_sourceReference;
    std::string m_str;
};

/**
 * Basic synchronous tokenizer.
 *
 * Typical usage:
 *   1. Construct the tokenizer with the shared ErrorCollector and SourceManager.
 *   2. Call tokenizeBuffer(startOffset, sourceId).
 *   3. If the call returns true, pass getTokens() to BasicParsingContext.
 *
 * Lifetime notes:
 * - The token list belongs to the tokenizer instance and stays valid until the
 *   tokenizer is reused or destroyed.
 * - Source text is read from SourceManager; callers do not pass raw buffers.
 */
class BasicTokenizer
{
  public:
    /**
     * Creates a tokenizer bound to the given diagnostics and source registry.
     *
     * The tokenizer does not take ownership of these shared services, but it
     * expects them to outlive tokenization calls.
     */
    BasicTokenizer(const std::shared_ptr<ErrorCollector> &errorCollector,
                   const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Tokenizes the source identified by sourceId, starting at the given byte
     * offset within that source.
     *
     * @param address Start offset inside the source buffer.
     * @param sourceId SourceManager identifier for the input source.
     * @return true when the whole remaining buffer was tokenized successfully;
     *         false when input is invalid or a fatal lexical error was emitted.
     *
     * On success, getTokens() exposes the generated token stream. On failure,
     * the token stream should be considered incomplete.
     */
    bool tokenizeBuffer(size_t address, size_t sourceId);

    /**
     * Returns the generated tokens in source order.
     *
     * The returned reference becomes stale only if the tokenizer is reused or
     * destroyed.
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
