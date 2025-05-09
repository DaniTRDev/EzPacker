#ifndef EZPACKER_ITOKENIZER_H
#define EZPACKER_ITOKENIZER_H

#include "EzIRBuilderCommon.h"

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
const static std::function<bool(char ch)> isSpecial = [](char ch) -> bool { return (ch == '-') || (ch == '_'); };

/**
 * Returns true if ch is white space.
 * @param ch
 * @return bool
 */
const static std::function<bool(char ch)> isWhiteSpace = [](char ch) -> bool { return (ch == ' '); };
}; // namespace TokenizerHelpers

/**
 * Enum to indicate different types of tokens. "From XX" indicates that the token is generated depending on the
 * specified table given to the tokenizer.
 */
enum IRTokenType : uint8_t
{
    Invalid = 0,
    Arch,       // From keywords.
    Comma,
    Comment,
    DoubleDot,   // :
    End,         // From keywords.
    Identifier,  // A word that is not a keyword nor a register formed with [a-z, A-Z] |& [0-9] |& _,-
    Instruction, // From instruction map.
    Label,       // From keywords.
    LeftParen,   // Left parenthesis.
    Memory,
    Module,      // From keywords
    NumberInt,   // A word that is not a keyword, nor a register, nor an identifier formed with: [0-9]
    NumberFloat, // A word that is not a keyword, nor a register, nor an identifier formed with: {NumberInt}.NumberInt
    Register,    // Registers are identified with '%regName'.
    RightParen,  // Right parenthesis.
    String,      // Starts with " and ends with ". ANY character is allowed inside.
    TypeI8,      // From types.
    TypeI16,     // From types.
    TypeI32,     // From types.
    TypeI64,     // From types.
    TypePtr,     // From types.
    Variable,
    Vector      // From keywords.
};

class ITokenizer
{
  public:
    virtual ~ITokenizer() = default;

  protected:
    /**
     * Tries to read the input buffer at given address to match input in a SINGLE-FIRST-OCURRENCE TokenType and returns
     * the number of bytes used to identify the token (should be used in the loop calling this function to increment
     * buffer starting address). Returns 0 if there was any error.
     * @param buffer
     * @param address
     * @param bufferSize
     * @param token
     * @return size_t
     */
    virtual size_t tokenize(char *buffer, size_t address, size_t bufferSize, IRTokenType &token) = 0;

  private:
};

#endif // EZPACKER_ITOKENIZER_H
