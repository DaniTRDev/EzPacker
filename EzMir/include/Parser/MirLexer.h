#ifndef EZMIR_MIR_LEXER_H
#define EZMIR_MIR_LEXER_H

#include "EzMirCommon.h"
#include <memory_resource>
#include <optional>
#include <string_view>

class SourceReference;
namespace EzMir { class MirParserContext; }

namespace EzMir::Parser
{

enum class MirTokenKind : uint8_t
{
    EndOfFile = 0,

    // Identifiers & Names
    Identifier,
    GlobalName,
    LocalName,

    // Literals
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,

    // Keywords
    KwFn,
    KwDeclare,
    KwConst,
    KwVar,
    KwExternal,
    KwInternal,
    KwWeak,
    KwLabel,
    KwTarget,

    // Types
    TypeI1,
    TypeI8,
    TypeI16,
    TypeI32,
    TypeI64,
    TypeI128,
    TypeI256,
    TypeF32,
    TypeF64,
    TypeF128,
    TypePtr,
    TypeVoid,
    TypeToken,

    // Delimiters & Operators
    Equal,        // =
    Colon,        // :
    Semicolon,    // ;
    Comma,        // ,
    Arrow,        // ->
    Ellipsis,     // ...
    LParen,       // (
    RParen,       // )
    LBracket,     // [
    RBracket,     // ]
    LBrace,       // {
    RBrace,       // }
    Plus,         // +
    Minus,        // -
    Star,         // *
    LAngle,       // <
    RAngle,       // >

    Unknown
};

struct MirToken
{
    MirTokenKind m_kind{ MirTokenKind::EndOfFile };
    std::string_view m_text;
    int64_t m_intVal{ 0 };
    double m_floatVal{ 0.0 };
    std::pmr::string m_strVal;
    size_t m_startOffset{ 0 };
    size_t m_length{ 0 };
    SourceReference *m_ref{ nullptr };

    explicit MirToken(std::pmr::memory_resource *mr) : m_strVal(mr) {}
};

/**
 * Tokenizer for Textual MIR (.mir) source streams.
 */
class MirLexer
{
  public:
    MirLexer(std::string_view source, MirParserContext &context);

    MirToken nextToken();
    const MirToken &peekToken();

    bool isAtEnd() const;

  private:
    void skipWhitespaceAndComments();
    MirToken lexNumber(size_t startPos);
    MirToken lexString(size_t startPos);
    MirToken lexIdentifierOrKeyword(size_t startPos);

    char peekChar() const;
    char getChar();
    char peekNextChar() const;

  private:
    std::string_view m_source;
    size_t m_cursor{ 0 };
    MirParserContext &m_ctx;
    std::optional<MirToken> m_peeked;
};

} // namespace EzMir::Parser

#endif // EZMIR_MIR_LEXER_H
