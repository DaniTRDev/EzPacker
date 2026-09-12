#ifndef EZDSL_CPP_SOURCE_EMITTER_H
#define EZDSL_CPP_SOURCE_EMITTER_H

#include "EzDslCodeGeneratorsCommon.h"

namespace CodeGenerators
{

/**
 * High-performance fluent C++ source code formatting and emission engine.
 * Handles automatic indentation, scoped syntactic structures (namespaces, classes,
 * enums, blocks, preprocessor conditionals), standard banners, and include management.
 */
class CppSourceEmitter
{
  public:
    /**
     * RAII scope guard for automatically closing and dedenting C++ syntactic blocks.
     */
    class Scope
    {
      public:
        Scope(CppSourceEmitter &emitter, std::string footer, bool indentBody = true);
        ~Scope();

        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;

        Scope(Scope &&other) noexcept;
        Scope &operator=(Scope &&other) noexcept;

        void close();

      private:
        CppSourceEmitter *m_emitter{ nullptr };
        std::string m_footer;
        bool m_indentBody{ true };
        bool m_active{ true };
    };

  public:
    explicit CppSourceEmitter(size_t initialCapacity = 4096, std::string_view indentString = "    ");

    // --- Indentation Management ---

    void indent() noexcept;
    void dedent() noexcept;
    size_t getIndentLevel() const noexcept;
    void setIndentLevel(size_t level) noexcept;
    std::string_view getIndentString() const noexcept;
    void setIndentString(std::string_view indentStr);

    // --- Scoped Block Helpers ---

    [[nodiscard]] Scope enterScope(std::string_view header = "{",
                                   std::string_view footer = "}",
                                   bool indentBody = true);

    [[nodiscard]] Scope enterBlock(std::string_view prefix = "");
    [[nodiscard]] Scope enterNamespace(std::string_view name);
    [[nodiscard]] Scope enterClass(std::string_view name, std::string_view base = "");
    [[nodiscard]] Scope enterStruct(std::string_view name, std::string_view base = "");
    [[nodiscard]] Scope enterEnum(std::string_view name, std::string_view underlyingType = "", bool isClass = true);
    [[nodiscard]] Scope enterIfdef(std::string_view condition);
    [[nodiscard]] Scope enterIfndef(std::string_view condition);

    // --- Line and Content Emission ---

    void emitLine(std::string_view line = "");

    template <typename... Args>
    void emitLine(std::format_string<Args...> fmt, Args &&...args)
    {
        emitLine(std::format(fmt, std::forward<Args>(args)...));
    }

    void emit(std::string_view text);

    template <typename... Args>
    void emit(std::format_string<Args...> fmt, Args &&...args)
    {
        emit(std::format(fmt, std::forward<Args>(args)...));
    }

    void emitRaw(std::string_view rawText);
    void emitLines(std::string_view multilineText);
    void emitBlankLine();

    // --- C++ Idioms & Common Constructs ---

    void emitBanner(std::string_view generatorName, std::string_view notice = "DO NOT EDIT.");
    void emitComment(std::string_view comment);
    void emitDocComment(std::string_view doc);
    void emitSectionComment(std::string_view title);

    void emitInclude(std::string_view header, bool isSystem = false);
    void emitPragmaOnce();
    void emitIncludeGuardStart(std::string_view guardName);
    void emitIncludeGuardEnd(std::string_view guardName);

    // --- Output Access ---

    const std::string &str() const noexcept;
    std::string_view view() const noexcept;
    std::string takeStr();
    void clear();
    size_t size() const noexcept;
    bool empty() const noexcept;

  private:
    void applyIndent();

  private:
    std::string m_buffer;
    size_t m_indentLevel{ 0 };
    std::string m_indentString;
    bool m_atStartOfLine{ true };
    bool m_lastWasBlank{ false };
};

} // namespace CodeGenerators

#endif // EZDSL_CPP_SOURCE_EMITTER_H
