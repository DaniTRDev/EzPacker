#ifndef EZMIR_MIR_LEXER_H
#define EZMIR_MIR_LEXER_H

#include "EzMirCommon.h"
#include <deque>
#include <memory_resource>
#include <optional>
#include <string_view>

class SourceReference;
namespace EzMir
{
class MirParserContext;
}

namespace EzMir::Parser
{

/**
 * Lexical token categories produced by MirLexer.
 */
enum class MirTokenKind : uint8_t
{
    EndOfFile = 0, ///< Synthetic token emitted once the input is exhausted.

    // Identifiers & Names
    Identifier, ///< A bare name not prefixed with a sigil.
    GlobalName, ///< A name prefixed with '@' referring to a global symbol.
    LocalName,  ///< A name prefixed with '%' referring to a local (register/block).

    // Literals
    IntegerLiteral, ///< Integer numeric literal with optional radix prefix.
    FloatLiteral,   ///< Floating-point numeric literal.
    StringLiteral,  ///< Quoted string literal.

    // Keywords
    KwFn,       ///< "fn" keyword introducing a function.
    KwDeclare,  ///< "declare" keyword introducing a function prototype.
    KwConst,    ///< "const" keyword marking a read-only global.
    KwVar,      ///< "var" keyword introducing a global variable.
    KwExternal, ///< "external" linkage keyword.
    KwInternal, ///< "internal" linkage keyword.
    KwWeak,     ///< "weak" linkage keyword.
    KwLabel,    ///< "label" keyword introducing a basic block label.
    KwTarget,   ///< "target" keyword introducing a target directive.

    // Types
    TypeI1,    ///< "i1" boolean-width integer type.
    TypeI8,    ///< "i8" integer type.
    TypeI16,   ///< "i16" integer type.
    TypeI32,   ///< "i32" integer type.
    TypeI64,   ///< "i64" integer type.
    TypeI128,  ///< "i128" integer type.
    TypeI256,  ///< "i256" integer type.
    TypeF32,   ///< "f32" floating-point type.
    TypeF64,   ///< "f64" floating-point type.
    TypeF128,  ///< "f128" floating-point type.
    TypePtr,   ///< "ptr" pointer type.
    TypeVoid,  ///< "void" type.
    TypeToken, ///< "token" opaque token type.

    // Delimiters & Operators
    Equal,     // =
    Colon,     // :
    Semicolon, // ;
    Comma,     // ,
    Arrow,     // ->
    Ellipsis,  // ...
    LParen,    // (
    RParen,    // )
    LBracket,  // [
    RBracket,  // ]
    LBrace,    // {
    RBrace,    // }
    Plus,      // +
    Minus,     // -
    Star,      // *
    LAngle,    // <
    RAngle,    // >

    Unknown ///< Unrecognized character; retained so the parser can diagnose it.
};

/**
 * A single lexed token with its kind, spelling, decoded literal payload and source span.
 */
struct MirToken
{
    MirTokenKind m_kind{ MirTokenKind::EndOfFile }; // Token category.
    std::string_view m_text;                        // Raw token text as it appears in the source.
    int64_t m_intVal{ 0 };                          // Decoded value for IntegerLiteral.
    double m_floatVal{ 0.0 };                       // Decoded value for FloatLiteral.
    std::pmr::string m_strVal;                      // Decoded contents for StringLiteral.
    size_t m_startOffset{ 0 };                      // Byte offset where the token begins.
    size_t m_length{ 0 };                           // Length of the token text in bytes.
    SourceReference *m_ref{ nullptr };              // Source reference for the token span.

    /**
     * Allocates the decoded string payload from the given arena.
     */
    explicit MirToken(std::pmr::memory_resource *mr) : m_strVal(mr) {}
};

/**
 * Tokenizer for Textual MIR (.mir) source streams.
 */
class MirLexer
{
  public:
    /**
     * Creates a lexer over the given source buffer, using context to allocate strings and source refs.
     */
    MirLexer(std::string_view source, MirParserContext &context);

    /**
     * Consumes and returns the next token, advancing the cursor past it.
     */
    MirToken nextToken();

    /**
     * Returns the token ahead-th token without consuming it, buffering the lookahead.
     * ahead = 0 (default) is the next token; ahead = 1 enables two-token lookahead,
     * used to disambiguate a block label from an instruction without consuming input.
     */
    const MirToken &peekToken(size_t ahead = 0);

    /**
     * Pushes a previously consumed token back onto the input so it becomes the next token
     * returned by nextToken()/peekToken(). Used to re-examine a token that was speculatively
     * consumed while disambiguating the surrounding syntax.
     */
    void pushBack(MirToken tok);

    /**
     * Returns true once the cursor has consumed the entire source.
     */
    bool isAtEnd() const;

  private:
    /**
     * Lexes the next raw token directly from the source, bypassing the lookahead buffer.
     */
    MirToken lexToken();

    /**
     * Advances past whitespace, line comments and block comments.
     */
    void skipWhitespaceAndComments();
    /**
     * Scans an integer or floating-point literal starting at startPos.
     */
    MirToken lexNumber(size_t startPos);
    /**
     * Scans a quoted string literal starting at startPos.
     */
    MirToken lexString(size_t startPos);
    /**
     * Scans an identifier and classifies it as an identifier or keyword.
     */
    MirToken lexIdentifierOrKeyword(size_t startPos);

    /**
     * Returns the current character without consuming it, or '\0' at end of input.
     */
    char peekChar() const;
    /**
     * Returns the current character and advances the cursor, or '\0' at end of input.
     */
    char getChar();
    /**
     * Returns the character after the current one without consuming either.
     */
    char peekNextChar() const;

  private:
    std::string_view m_source;          // Source buffer being tokenized.
    size_t m_cursor{ 0 };               // Byte index of the next unconsumed character.
    MirParserContext &m_ctx;            // Context used for allocations and source references.
    std::deque<MirToken> m_lookahead;   // Buffered lookahead tokens; front is the next token to consume.
};

} // namespace EzMir::Parser

#endif // EZMIR_MIR_LEXER_H
