#include "CodeGenerators/CppSourceEmitter.h"

namespace CodeGenerators
{

// ============================================================================
// CppSourceEmitter::Scope Implementation
// ============================================================================

// Captures the owning emitter, the closing footer text and whether the body is indented.
CppSourceEmitter::Scope::Scope(CppSourceEmitter &emitter, std::string footer, bool indentBody) :
    m_emitter(&emitter), m_footer(std::move(footer)), m_indentBody(indentBody), m_active(true)
{
}

// Closes the scope on destruction if it was not closed explicitly.
CppSourceEmitter::Scope::~Scope() { close(); }

// Transfers ownership of the open scope, disarming the moved-from object.
CppSourceEmitter::Scope::Scope(Scope &&other) noexcept :
    m_emitter(other.m_emitter), m_footer(std::move(other.m_footer)), m_indentBody(other.m_indentBody),
    m_active(other.m_active)
{
    other.m_active = false;
    other.m_emitter = nullptr;
}

// Closes any currently held scope before taking over other's scope.
CppSourceEmitter::Scope &CppSourceEmitter::Scope::operator=(Scope &&other) noexcept
{
    if (this != &other)
    {
        close();
        m_emitter = other.m_emitter;
        m_footer = std::move(other.m_footer);
        m_indentBody = other.m_indentBody;
        m_active = other.m_active;

        other.m_active = false;
        other.m_emitter = nullptr;
    }
    return *this;
}

// Emits the footer exactly once and restores the indentation it added.
void CppSourceEmitter::Scope::close()
{
    if (m_active && m_emitter)
    {
        m_active = false;
        if (m_indentBody)
        {
            m_emitter->dedent();
        }
        if (!m_footer.empty())
        {
            m_emitter->emitLine(m_footer);
        }
        m_emitter = nullptr;
    }
}

// ============================================================================
// CppSourceEmitter Implementation
// ============================================================================

// Reserves buffer space up front and records the indent string used for each nesting level.
CppSourceEmitter::CppSourceEmitter(size_t initialCapacity, std::string_view indentString) :
    m_indentLevel(0), m_indentString(indentString), m_atStartOfLine(true), m_lastWasBlank(false)
{
    m_buffer.reserve(initialCapacity);
}

// Increases the indentation depth applied to subsequent lines.
void CppSourceEmitter::indent() noexcept { ++m_indentLevel; }

// Decreases the indentation depth, saturating at zero.
void CppSourceEmitter::dedent() noexcept
{
    if (m_indentLevel > 0)
    {
        --m_indentLevel;
    }
}

size_t CppSourceEmitter::getIndentLevel() const noexcept { return m_indentLevel; }

void CppSourceEmitter::setIndentLevel(size_t level) noexcept { m_indentLevel = level; }

std::string_view CppSourceEmitter::getIndentString() const noexcept { return m_indentString; }

void CppSourceEmitter::setIndentString(std::string_view indentStr) { m_indentString = indentStr; }

// Appends the configured indentation once per level at the start of a fresh line.
void CppSourceEmitter::applyIndent()
{
    if (m_atStartOfLine && m_indentLevel > 0)
    {
        for (size_t i = 0; i < m_indentLevel; ++i)
        {
            m_buffer += m_indentString;
        }
        m_atStartOfLine = false;
    }
}

// Emits the header, optionally indents the body, and hands back the RAII closer.
CppSourceEmitter::Scope CppSourceEmitter::enterScope(std::string_view header, std::string_view footer, bool indentBody)
{
    if (!header.empty())
    {
        emitLine(header);
    }
    if (indentBody)
    {
        indent();
    }
    return Scope(*this, std::string(footer), indentBody);
}

// Enters a braced block, optionally preceded by a statement prefix such as `if (x)`.
CppSourceEmitter::Scope CppSourceEmitter::enterBlock(std::string_view prefix)
{
    if (prefix.empty())
    {
        return enterScope("{", "}");
    }
    emitLine(prefix);
    return enterScope("{", "}");
}

// Opens a namespace block whose closing footer echoes the namespace name.
CppSourceEmitter::Scope CppSourceEmitter::enterNamespace(std::string_view name)
{
    emitLine(std::format("namespace {}", name));
    return enterScope("{", std::format("}} // namespace {}", name));
}

// Opens a class definition, including an optional base-clause.
CppSourceEmitter::Scope CppSourceEmitter::enterClass(std::string_view name, std::string_view base)
{
    if (base.empty())
    {
        emitLine(std::format("class {}", name));
    }
    else
    {
        emitLine(std::format("class {} : {}", name, base));
    }
    return enterScope("{", "};");
}

// Opens a struct definition, including an optional base-clause.
CppSourceEmitter::Scope CppSourceEmitter::enterStruct(std::string_view name, std::string_view base)
{
    if (base.empty())
    {
        emitLine(std::format("struct {}", name));
    }
    else
    {
        emitLine(std::format("struct {} : {}", name, base));
    }
    return enterScope("{", "};");
}

// Opens an enum or enum class with an optional fixed underlying type.
CppSourceEmitter::Scope
CppSourceEmitter::enterEnum(std::string_view name, std::string_view underlyingType, bool isClass)
{
    std::string decl = isClass ? std::format("enum class {}", name) : std::format("enum {}", name);
    if (!underlyingType.empty())
    {
        decl += std::format(" : {}", underlyingType);
    }
    emitLine(decl);
    return enterScope("{", "};");
}

// Opens an `#ifdef` block whose footer is the matching `#endif`.
CppSourceEmitter::Scope CppSourceEmitter::enterIfdef(std::string_view condition)
{
    emitLine(std::format("#ifdef {}", condition));
    return Scope(*this, std::format("#endif // {}", condition), false);
}

// Opens an `#ifndef` block whose footer is the matching `#endif`.
CppSourceEmitter::Scope CppSourceEmitter::enterIfndef(std::string_view condition)
{
    emitLine(std::format("#ifndef {}", condition));
    return Scope(*this, std::format("#endif // {}", condition), false);
}

// Emits one line (or a blank line when empty), prefixing the current indentation.
void CppSourceEmitter::emitLine(std::string_view line)
{
    if (line.empty())
    {
        emitBlankLine();
        return;
    }

    applyIndent();
    m_buffer.append(line);
    m_buffer.push_back('\n');
    m_atStartOfLine = true;
    m_lastWasBlank = false;
}

// Appends text verbatim, tracking whether the buffer now sits at a line start.
void CppSourceEmitter::emit(std::string_view text)
{
    if (text.empty())
    {
        return;
    }

    applyIndent();
    m_buffer.append(text);
    m_atStartOfLine = (text.back() == '\n');
    m_lastWasBlank = false;
}

// Appends text with no indentation applied, preserving it byte for byte.
void CppSourceEmitter::emitRaw(std::string_view rawText)
{
    if (rawText.empty())
    {
        return;
    }

    m_buffer.append(rawText);
    m_atStartOfLine = (rawText.back() == '\n');
    m_lastWasBlank = false;
}

// Splits multiline text on newlines, stripping carriage returns and re-indenting each line.
void CppSourceEmitter::emitLines(std::string_view multilineText)
{
    size_t start = 0;
    while (start < multilineText.size())
    {
        size_t end = multilineText.find('\n', start);
        if (end == std::string_view::npos)
        {
            std::string_view line = multilineText.substr(start);
            if (!line.empty() && line.back() == '\r')
            {
                line.remove_suffix(1);
            }
            emitLine(line);
            break;
        }

        std::string_view line = multilineText.substr(start, end - start);
        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1);
        }
        emitLine(line);
        start = end + 1;
    }
}

// Emits a single blank line, collapsing consecutive blanks and leading blanks.
void CppSourceEmitter::emitBlankLine()
{
    if (!m_lastWasBlank && !m_buffer.empty())
    {
        m_buffer.push_back('\n');
        m_atStartOfLine = true;
        m_lastWasBlank = true;
    }
}

// Emits the standard three-line auto-generation banner.
void CppSourceEmitter::emitBanner(std::string_view generatorName, std::string_view notice)
{
    emitLine("// ============================================================================");
    emitLine(std::format("// Auto-generated by EzDSL {}. {}", generatorName, notice));
    emitLine("// ============================================================================");
}

// Emits a single-line `//` comment.
void CppSourceEmitter::emitComment(std::string_view comment) { emitLine(std::format("// {}", comment)); }

// Emits a Doxygen block comment, re-indenting each line of the supplied documentation.
void CppSourceEmitter::emitDocComment(std::string_view doc)
{
    emitLine("/**");
    emitLines(doc);
    emitLine(" */");
}

// Emits a `// --- title ---` divider surrounded by blank lines.
void CppSourceEmitter::emitSectionComment(std::string_view title)
{
    emitBlankLine();
    emitLine(std::format("// --- {} ---", title));
    emitBlankLine();
}

// Emits an `#include` choosing angle brackets for system headers and quotes otherwise.
void CppSourceEmitter::emitInclude(std::string_view header, bool isSystem)
{
    if (isSystem)
    {
        emitLine(std::format("#include <{}>", header));
    }
    else
    {
        emitLine(std::format("#include \"{}\"", header));
    }
}

// Emits `#pragma once`.
void CppSourceEmitter::emitPragmaOnce() { emitLine("#pragma once"); }

// Emits the `#ifndef`/`#define` pair that opens an include guard.
void CppSourceEmitter::emitIncludeGuardStart(std::string_view guardName)
{
    emitLine(std::format("#ifndef {}", guardName));
    emitLine(std::format("#define {}", guardName));
}

// Emits the `#endif` that closes an include guard.
void CppSourceEmitter::emitIncludeGuardEnd(std::string_view guardName)
{
    emitLine(std::format("#endif // {}", guardName));
}

const std::string &CppSourceEmitter::str() const noexcept { return m_buffer; }

std::string_view CppSourceEmitter::view() const noexcept { return m_buffer; }

// Moves out the accumulated output and resets all emitter state.
std::string CppSourceEmitter::takeStr()
{
    std::string res = std::move(m_buffer);
    m_buffer.clear();
    m_indentLevel = 0;
    m_atStartOfLine = true;
    m_lastWasBlank = false;
    return res;
}

// Discards the accumulated output and resets all emitter state.
void CppSourceEmitter::clear()
{
    m_buffer.clear();
    m_indentLevel = 0;
    m_atStartOfLine = true;
    m_lastWasBlank = false;
}

size_t CppSourceEmitter::size() const noexcept { return m_buffer.size(); }

bool CppSourceEmitter::empty() const noexcept { return m_buffer.empty(); }

} // namespace CodeGenerators
