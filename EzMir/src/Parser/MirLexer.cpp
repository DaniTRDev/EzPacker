#include "Parser/MirLexer.h"
#include "Parser/MirParserContext.h"
#include <charconv>
#include <cctype>

namespace EzMir::Parser
{

/**
 * Binds the lexer to the source buffer and parser context, starting at offset 0.
 */
MirLexer::MirLexer(std::string_view source, MirParserContext &context) : m_source(source), m_cursor(0), m_ctx(context)
{
}

/**
 * Returns the character at the cursor without consuming it, or '\0' at end of input.
 */
char MirLexer::peekChar() const
{
    if (m_cursor >= m_source.size())
    {
        return '\0';
    }
    return m_source[m_cursor];
}

/**
 * Consumes and returns the character at the cursor, or '\0' if the input is exhausted.
 */
char MirLexer::getChar()
{
    if (m_cursor >= m_source.size())
    {
        return '\0';
    }
    return m_source[m_cursor++];
}

/**
 * Returns the character after the cursor without consuming either, or '\0' at end of input.
 */
char MirLexer::peekNextChar() const
{
    if (m_cursor + 1 >= m_source.size())
    {
        return '\0';
    }
    return m_source[m_cursor + 1];
}

/**
 * Returns true once the cursor has reached the end of the source.
 */
bool MirLexer::isAtEnd() const { return m_cursor >= m_source.size(); }

/**
 * Advances past spaces/newlines, // line comments, and ';' line comments that appear at the start
 * of a line.
 */
void MirLexer::skipWhitespaceAndComments()
{
    bool atStartOfLine = (m_cursor == 0);

    while (m_cursor < m_source.size())
    {
        char c = m_source[m_cursor];

        if (c == '\n')
        {
            ++m_cursor;
            atStartOfLine = true;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            ++m_cursor;
            continue;
        }

        // C++ style single-line comment: // ...
        if (c == '/' && peekNextChar() == '/')
        {
            m_cursor += 2;
            while (m_cursor < m_source.size() && m_source[m_cursor] != '\n')
            {
                ++m_cursor;
            }
            continue;
        }

        // LLVM style single-line comment: ; ... (only when at start of line)
        if (c == ';' && atStartOfLine)
        {
            ++m_cursor;
            while (m_cursor < m_source.size() && m_source[m_cursor] != '\n')
            {
                ++m_cursor;
            }
            continue;
        }

        // Found non-whitespace, non-comment token start
        break;
    }
}

/**
 * Returns the token ahead-th ahead of the cursor without consuming it, lexing and buffering as many
 * tokens as needed. References remain valid until the token is consumed because the underlying
 * deque never invalidates references to existing elements on push/pop.
 */
const MirToken &MirLexer::peekToken(size_t ahead)
{
    while (m_lookahead.size() <= ahead)
    {
        m_lookahead.push_back(lexToken());
    }
    return m_lookahead[ahead];
}

/**
 * Returns the next token, consuming any buffered lookahead first and otherwise lexing from source.
 */
MirToken MirLexer::nextToken()
{
    if (!m_lookahead.empty())
    {
        MirToken tok = std::move(m_lookahead.front());
        m_lookahead.pop_front();
        return tok;
    }

    return lexToken();
}

/**
 * Pushes a previously consumed token back as the next token to be returned.
 */
void MirLexer::pushBack(MirToken tok) { m_lookahead.push_front(std::move(tok)); }

/**
 * Lexes the next raw token directly from source. Dispatches to the number/string/identifier
 * scanners and otherwise classifies punctuation, including a leading '-' on a digit as a negative
 * number.
 */
MirToken MirLexer::lexToken()
{
    skipWhitespaceAndComments();

    MirToken tok(m_ctx.getArena());
    tok.m_startOffset = m_cursor;

    if (m_cursor >= m_source.size())
    {
        tok.m_kind = MirTokenKind::EndOfFile;
        tok.m_text = "";
        tok.m_length = 0;
        tok.m_ref = m_ctx.createRef(m_cursor, 0);
        return tok;
    }

    char c = peekChar();

    // Check for negative number or minus
    if (c == '-' && std::isdigit(static_cast<unsigned char>(peekNextChar())))
    {
        return lexNumber(m_cursor);
    }

    // Number literals
    if (std::isdigit(static_cast<unsigned char>(c)))
    {
        return lexNumber(m_cursor);
    }

    // String literals
    if (c == '"')
    {
        return lexString(m_cursor);
    }

    // Global names: @...
    if (c == '@')
    {
        getChar(); // Consume '@'
        size_t idStart = m_cursor;
        while (m_cursor < m_source.size() &&
               (std::isalnum(static_cast<unsigned char>(m_source[m_cursor])) || m_source[m_cursor] == '_' ||
                m_source[m_cursor] == '.'))
        {
            ++m_cursor;
        }
        tok.m_kind = MirTokenKind::GlobalName;
        tok.m_length = m_cursor - tok.m_startOffset;
        tok.m_text = m_source.substr(tok.m_startOffset, tok.m_length);
        tok.m_strVal = std::pmr::string(m_source.substr(idStart, m_cursor - idStart), m_ctx.getArena());
        tok.m_ref = m_ctx.createRef(tok.m_startOffset, tok.m_length);
        return tok;
    }

    // Local names: %...
    if (c == '%')
    {
        getChar(); // Consume '%'
        size_t idStart = m_cursor;
        while (m_cursor < m_source.size() &&
               (std::isalnum(static_cast<unsigned char>(m_source[m_cursor])) || m_source[m_cursor] == '_' ||
                m_source[m_cursor] == '.'))
        {
            ++m_cursor;
        }
        tok.m_kind = MirTokenKind::LocalName;
        tok.m_length = m_cursor - tok.m_startOffset;
        tok.m_text = m_source.substr(tok.m_startOffset, tok.m_length);
        tok.m_strVal = std::pmr::string(m_source.substr(idStart, m_cursor - idStart), m_ctx.getArena());
        tok.m_ref = m_ctx.createRef(tok.m_startOffset, tok.m_length);
        return tok;
    }

    // Identifiers and Keywords
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
    {
        return lexIdentifierOrKeyword(m_cursor);
    }

    // Multi-character operators
    if (c == '-' && peekNextChar() == '>')
    {
        getChar();
        getChar();
        tok.m_kind = MirTokenKind::Arrow;
        tok.m_length = 2;
        tok.m_text = "->";
        tok.m_ref = m_ctx.createRef(tok.m_startOffset, tok.m_length);
        return tok;
    }

    if (c == '.' && peekNextChar() == '.' && m_cursor + 2 < m_source.size() && m_source[m_cursor + 2] == '.')
    {
        getChar();
        getChar();
        getChar();
        tok.m_kind = MirTokenKind::Ellipsis;
        tok.m_length = 3;
        tok.m_text = "...";
        tok.m_ref = m_ctx.createRef(tok.m_startOffset, tok.m_length);
        return tok;
    }

    // Single-character punctuation
    getChar(); // Consume character
    tok.m_length = 1;
    tok.m_text = m_source.substr(tok.m_startOffset, 1);
    tok.m_ref = m_ctx.createRef(tok.m_startOffset, 1);

    switch (c)
    {
        case '=':
            tok.m_kind = MirTokenKind::Equal;
            break;
        case ':':
            tok.m_kind = MirTokenKind::Colon;
            break;
        case ';':
            tok.m_kind = MirTokenKind::Semicolon;
            break;
        case ',':
            tok.m_kind = MirTokenKind::Comma;
            break;
        case '(':
            tok.m_kind = MirTokenKind::LParen;
            break;
        case ')':
            tok.m_kind = MirTokenKind::RParen;
            break;
        case '[':
            tok.m_kind = MirTokenKind::LBracket;
            break;
        case ']':
            tok.m_kind = MirTokenKind::RBracket;
            break;
        case '{':
            tok.m_kind = MirTokenKind::LBrace;
            break;
        case '}':
            tok.m_kind = MirTokenKind::RBrace;
            break;
        case '+':
            tok.m_kind = MirTokenKind::Plus;
            break;
        case '-':
            tok.m_kind = MirTokenKind::Minus;
            break;
        case '*':
            tok.m_kind = MirTokenKind::Star;
            break;
        case '<':
            tok.m_kind = MirTokenKind::LAngle;
            break;
        case '>':
            tok.m_kind = MirTokenKind::RAngle;
            break;
        default:
            tok.m_kind = MirTokenKind::Unknown;
            break;
    }

    return tok;
}

/**
 * Scans an integer (decimal, 0x/0X hex) or floating-point (fractional and/or exponent) literal
 * beginning at startPos and decodes its value.
 */
MirToken MirLexer::lexNumber(size_t startPos)
{
    MirToken tok(m_ctx.getArena());
    tok.m_startOffset = startPos;

    bool isNegative = false;
    if (peekChar() == '-')
    {
        isNegative = true;
        getChar();
    }
    else if (peekChar() == '+')
    {
        getChar();
    }

    // Check for hex, bin, oct
    if (peekChar() == '0' && (peekNextChar() == 'x' || peekNextChar() == 'X'))
    {
        getChar(); // '0'
        getChar(); // 'x'
        size_t digitsStart = m_cursor;
        while (m_cursor < m_source.size() && std::isxdigit(static_cast<unsigned char>(m_source[m_cursor])))
        {
            ++m_cursor;
        }
        std::string_view hexStr = m_source.substr(digitsStart, m_cursor - digitsStart);
        uint64_t val = 0;
        std::from_chars(hexStr.data(), hexStr.data() + hexStr.size(), val, 16);

        tok.m_kind = MirTokenKind::IntegerLiteral;
        tok.m_intVal = isNegative ? -static_cast<int64_t>(val) : static_cast<int64_t>(val);
        tok.m_length = m_cursor - startPos;
        tok.m_text = m_source.substr(startPos, tok.m_length);
        tok.m_ref = m_ctx.createRef(startPos, tok.m_length);
        return tok;
    }

    // Decimal or Float
    bool isFloat = false;
    while (m_cursor < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_cursor])))
    {
        ++m_cursor;
    }

    if (m_cursor < m_source.size() && m_source[m_cursor] == '.' && m_cursor + 1 < m_source.size() &&
        std::isdigit(static_cast<unsigned char>(m_source[m_cursor + 1])))
    {
        isFloat = true;
        ++m_cursor; // Consume '.'
        while (m_cursor < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_cursor])))
        {
            ++m_cursor;
        }
    }

    if (m_cursor < m_source.size() && (m_source[m_cursor] == 'e' || m_source[m_cursor] == 'E'))
    {
        isFloat = true;
        ++m_cursor;
        if (m_cursor < m_source.size() && (m_source[m_cursor] == '+' || m_source[m_cursor] == '-'))
        {
            ++m_cursor;
        }
        while (m_cursor < m_source.size() && std::isdigit(static_cast<unsigned char>(m_source[m_cursor])))
        {
            ++m_cursor;
        }
    }

    tok.m_length = m_cursor - startPos;
    tok.m_text = m_source.substr(startPos, tok.m_length);
    tok.m_ref = m_ctx.createRef(startPos, tok.m_length);

    if (isFloat)
    {
        tok.m_kind = MirTokenKind::FloatLiteral;
        double fVal = 0.0;
        std::from_chars(tok.m_text.data(), tok.m_text.data() + tok.m_text.size(), fVal);
        tok.m_floatVal = fVal;
    }
    else
    {
        tok.m_kind = MirTokenKind::IntegerLiteral;
        int64_t iVal = 0;
        std::from_chars(tok.m_text.data(), tok.m_text.data() + tok.m_text.size(), iVal);
        tok.m_intVal = iVal;
    }

    return tok;
}

/**
 * Scans a double-quoted string literal starting at startPos, decoding the standard escape
 * sequences (\n, \t, \r, \\, \", \0xx and \xXX).
 */
MirToken MirLexer::lexString(size_t startPos)
{
    MirToken tok(m_ctx.getArena());
    tok.m_startOffset = startPos;
    tok.m_kind = MirTokenKind::StringLiteral;

    getChar(); // Consume opening '"'

    while (m_cursor < m_source.size())
    {
        char c = getChar();
        if (c == '"')
        {
            break;
        }
        if (c == '\\' && m_cursor < m_source.size())
        {
            char esc = getChar();
            if (esc == 'n')
                tok.m_strVal += '\n';
            else if (esc == 't')
                tok.m_strVal += '\t';
            else if (esc == 'r')
                tok.m_strVal += '\r';
            else if (esc == '\\')
                tok.m_strVal += '\\';
            else if (esc == '"')
                tok.m_strVal += '"';
            else if (esc == '0' && m_cursor < m_source.size() && std::isxdigit(static_cast<unsigned char>(peekChar())))
            {
                // Hex escape like \0A or \00
                char hex[3] = { '0', peekChar(), '\0' };
                getChar();
                uint8_t byteVal = 0;
                std::from_chars(hex, hex + 2, byteVal, 16);
                tok.m_strVal += static_cast<char>(byteVal);
            }
            else if ((esc == 'x' || esc == 'X') && m_cursor + 1 < m_source.size() &&
                     std::isxdigit(static_cast<unsigned char>(m_source[m_cursor])) &&
                     std::isxdigit(static_cast<unsigned char>(m_source[m_cursor + 1])))
            {
                char hex[3] = { m_source[m_cursor], m_source[m_cursor + 1], '\0' };
                m_cursor += 2;
                uint8_t byteVal = 0;
                std::from_chars(hex, hex + 2, byteVal, 16);
                tok.m_strVal += static_cast<char>(byteVal);
            }
            else
            {
                tok.m_strVal += esc;
            }
        }
        else
        {
            tok.m_strVal += c;
        }
    }

    tok.m_length = m_cursor - startPos;
    tok.m_text = m_source.substr(startPos, tok.m_length);
    tok.m_ref = m_ctx.createRef(startPos, tok.m_length);
    return tok;
}

/**
 * Scans an identifier starting at startPos and classifies it as a keyword, a primitive type
 * keyword, or a plain Identifier.
 */
MirToken MirLexer::lexIdentifierOrKeyword(size_t startPos)
{
    MirToken tok(m_ctx.getArena());
    tok.m_startOffset = startPos;

    while (m_cursor < m_source.size())
    {
        char c = m_source[m_cursor];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.')
        {
            ++m_cursor;
        }
        else
        {
            break;
        }
    }

    tok.m_length = m_cursor - startPos;
    tok.m_text = m_source.substr(startPos, tok.m_length);
    tok.m_strVal = std::pmr::string(tok.m_text, m_ctx.getArena());
    tok.m_ref = m_ctx.createRef(startPos, tok.m_length);

    std::string_view t = tok.m_text;

    // Check keywords
    if (t == "fn")
    {
        tok.m_kind = MirTokenKind::KwFn;
        return tok;
    }
    if (t == "declare")
    {
        tok.m_kind = MirTokenKind::KwDeclare;
        return tok;
    }
    if (t == "const")
    {
        tok.m_kind = MirTokenKind::KwConst;
        return tok;
    }
    if (t == "var")
    {
        tok.m_kind = MirTokenKind::KwVar;
        return tok;
    }
    if (t == "external")
    {
        tok.m_kind = MirTokenKind::KwExternal;
        return tok;
    }
    if (t == "internal")
    {
        tok.m_kind = MirTokenKind::KwInternal;
        return tok;
    }
    if (t == "weak")
    {
        tok.m_kind = MirTokenKind::KwWeak;
        return tok;
    }
    if (t == "label")
    {
        tok.m_kind = MirTokenKind::KwLabel;
        return tok;
    }
    if (t == "target")
    {
        tok.m_kind = MirTokenKind::KwTarget;
        return tok;
    }

    // Check primitive types
    if (t == "i1")
    {
        tok.m_kind = MirTokenKind::TypeI1;
        return tok;
    }
    if (t == "i8")
    {
        tok.m_kind = MirTokenKind::TypeI8;
        return tok;
    }
    if (t == "i16")
    {
        tok.m_kind = MirTokenKind::TypeI16;
        return tok;
    }
    if (t == "i32")
    {
        tok.m_kind = MirTokenKind::TypeI32;
        return tok;
    }
    if (t == "i64")
    {
        tok.m_kind = MirTokenKind::TypeI64;
        return tok;
    }
    if (t == "i128")
    {
        tok.m_kind = MirTokenKind::TypeI128;
        return tok;
    }
    if (t == "i256")
    {
        tok.m_kind = MirTokenKind::TypeI256;
        return tok;
    }
    if (t == "f32")
    {
        tok.m_kind = MirTokenKind::TypeF32;
        return tok;
    }
    if (t == "f64")
    {
        tok.m_kind = MirTokenKind::TypeF64;
        return tok;
    }
    if (t == "f128")
    {
        tok.m_kind = MirTokenKind::TypeF128;
        return tok;
    }
    if (t == "ptr")
    {
        tok.m_kind = MirTokenKind::TypePtr;
        return tok;
    }
    if (t == "void" || t == "_void")
    {
        tok.m_kind = MirTokenKind::TypeVoid;
        return tok;
    }
    if (t == "token" || t == "__bindToken")
    {
        tok.m_kind = MirTokenKind::TypeToken;
        return tok;
    }

    tok.m_kind = MirTokenKind::Identifier;
    return tok;
}

} // namespace EzMir::Parser
