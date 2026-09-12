#include "CodeGenerators/CppSourceEmitter.h"

namespace CodeGenerators
{

// ============================================================================
// CppSourceEmitter::Scope Implementation
// ============================================================================

CppSourceEmitter::Scope::Scope(CppSourceEmitter &emitter, std::string footer, bool indentBody) :
    m_emitter(&emitter),
    m_footer(std::move(footer)),
    m_indentBody(indentBody),
    m_active(true)
{
}

CppSourceEmitter::Scope::~Scope()
{
    close();
}

CppSourceEmitter::Scope::Scope(Scope &&other) noexcept :
    m_emitter(other.m_emitter),
    m_footer(std::move(other.m_footer)),
    m_indentBody(other.m_indentBody),
    m_active(other.m_active)
{
    other.m_active = false;
    other.m_emitter = nullptr;
}

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

CppSourceEmitter::CppSourceEmitter(size_t initialCapacity, std::string_view indentString) :
    m_indentLevel(0),
    m_indentString(indentString),
    m_atStartOfLine(true),
    m_lastWasBlank(false)
{
    m_buffer.reserve(initialCapacity);
}

void CppSourceEmitter::indent() noexcept
{
    ++m_indentLevel;
}

void CppSourceEmitter::dedent() noexcept
{
    if (m_indentLevel > 0)
    {
        --m_indentLevel;
    }
}

size_t CppSourceEmitter::getIndentLevel() const noexcept
{
    return m_indentLevel;
}

void CppSourceEmitter::setIndentLevel(size_t level) noexcept
{
    m_indentLevel = level;
}

std::string_view CppSourceEmitter::getIndentString() const noexcept
{
    return m_indentString;
}

void CppSourceEmitter::setIndentString(std::string_view indentStr)
{
    m_indentString = indentStr;
}

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

CppSourceEmitter::Scope CppSourceEmitter::enterScope(std::string_view header,
                                                     std::string_view footer,
                                                     bool indentBody)
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

CppSourceEmitter::Scope CppSourceEmitter::enterBlock(std::string_view prefix)
{
    if (prefix.empty())
    {
        return enterScope("{", "}");
    }
    emitLine(prefix);
    return enterScope("{", "}");
}

CppSourceEmitter::Scope CppSourceEmitter::enterNamespace(std::string_view name)
{
    emitLine(std::format("namespace {}", name));
    return enterScope("{", std::format("}} // namespace {}", name));
}

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

CppSourceEmitter::Scope CppSourceEmitter::enterEnum(std::string_view name,
                                                    std::string_view underlyingType,
                                                    bool isClass)
{
    std::string decl = isClass ? std::format("enum class {}", name) : std::format("enum {}", name);
    if (!underlyingType.empty())
    {
        decl += std::format(" : {}", underlyingType);
    }
    emitLine(decl);
    return enterScope("{", "};");
}

CppSourceEmitter::Scope CppSourceEmitter::enterIfdef(std::string_view condition)
{
    emitLine(std::format("#ifdef {}", condition));
    return Scope(*this, std::format("#endif // {}", condition), false);
}

CppSourceEmitter::Scope CppSourceEmitter::enterIfndef(std::string_view condition)
{
    emitLine(std::format("#ifndef {}", condition));
    return Scope(*this, std::format("#endif // {}", condition), false);
}

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

void CppSourceEmitter::emitBlankLine()
{
    if (!m_lastWasBlank && !m_buffer.empty())
    {
        m_buffer.push_back('\n');
        m_atStartOfLine = true;
        m_lastWasBlank = true;
    }
}

void CppSourceEmitter::emitBanner(std::string_view generatorName, std::string_view notice)
{
    emitLine("// ============================================================================");
    emitLine(std::format("// Auto-generated by EzDSL {}. {}", generatorName, notice));
    emitLine("// ============================================================================");
}

void CppSourceEmitter::emitComment(std::string_view comment)
{
    emitLine(std::format("// {}", comment));
}

void CppSourceEmitter::emitDocComment(std::string_view doc)
{
    emitLine("/**");
    emitLines(doc);
    emitLine(" */");
}

void CppSourceEmitter::emitSectionComment(std::string_view title)
{
    emitBlankLine();
    emitLine(std::format("// --- {} ---", title));
    emitBlankLine();
}

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

void CppSourceEmitter::emitPragmaOnce()
{
    emitLine("#pragma once");
}

void CppSourceEmitter::emitIncludeGuardStart(std::string_view guardName)
{
    emitLine(std::format("#ifndef {}", guardName));
    emitLine(std::format("#define {}", guardName));
}

void CppSourceEmitter::emitIncludeGuardEnd(std::string_view guardName)
{
    emitLine(std::format("#endif // {}", guardName));
}

const std::string &CppSourceEmitter::str() const noexcept
{
    return m_buffer;
}

std::string_view CppSourceEmitter::view() const noexcept
{
    return m_buffer;
}

std::string CppSourceEmitter::takeStr()
{
    std::string res = std::move(m_buffer);
    m_buffer.clear();
    m_indentLevel = 0;
    m_atStartOfLine = true;
    m_lastWasBlank = false;
    return res;
}

void CppSourceEmitter::clear()
{
    m_buffer.clear();
    m_indentLevel = 0;
    m_atStartOfLine = true;
    m_lastWasBlank = false;
}

size_t CppSourceEmitter::size() const noexcept
{
    return m_buffer.size();
}

bool CppSourceEmitter::empty() const noexcept
{
    return m_buffer.empty();
}

} // namespace CodeGenerators
