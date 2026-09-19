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
        /** Captures the emitter, the closing footer text, and whether the body should be indented. */
        Scope(CppSourceEmitter &emitter, std::string footer, bool indentBody = true);

        /** Closes the scope if still active, restoring indentation and emitting the footer. */
        ~Scope();

        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;

        /** Transfers ownership of the open scope, disarming the moved-from object. */
        Scope(Scope &&other) noexcept;

        /** Closes any currently held scope, then takes ownership of other's scope. */
        Scope &operator=(Scope &&other) noexcept;

        /** Emits the footer and dedents, or does nothing if the scope was already closed/moved from. */
        void close();

      private:
        CppSourceEmitter *m_emitter{ nullptr }; ///< Emitter to close against; null once closed or moved from.
        std::string m_footer;                   ///< Text emitted when the scope closes (e.g. `};` or `}`).
        bool m_indentBody{ true };              ///< Whether the scope increased indentation that must be restored.
        bool m_active{ true };                  ///< False once the scope has been closed or moved from.
    };

  public:
    /** Constructs an emitter, pre-reserving buffer capacity and configuring the per-level indent string. */
    explicit CppSourceEmitter(size_t initialCapacity = 4096, std::string_view indentString = "    ");

    // --- Indentation Management ---

    /** Increases the indentation level applied to subsequent lines. */
    void indent() noexcept;

    /** Decreases the indentation level, saturating at zero. */
    void dedent() noexcept;

    /** Returns the current indentation level. */
    size_t getIndentLevel() const noexcept;

    /** Forces the indentation level to an absolute value. */
    void setIndentLevel(size_t level) noexcept;

    /** Returns the string emitted once per indentation level. */
    std::string_view getIndentString() const noexcept;

    /** Sets the string emitted once per indentation level. */
    void setIndentString(std::string_view indentStr);

    // --- Scoped Block Helpers ---

    /** Emits header, optionally indents the body, and returns an RAII Scope that closes the block on destruction. */
    [[nodiscard]] Scope
    enterScope(std::string_view header = "{", std::string_view footer = "}", bool indentBody = true);

    /** Enters a braced block, optionally preceded by a statement prefix such as `if (x)`. */
    [[nodiscard]] Scope enterBlock(std::string_view prefix = "");

    /** Enters a namespace block and schedules its matching `} // namespace` footer. */
    [[nodiscard]] Scope enterNamespace(std::string_view name);

    /** Enters a class definition, optionally with a base-clause, and schedules the closing `};`. */
    [[nodiscard]] Scope enterClass(std::string_view name, std::string_view base = "");

    /** Enters a struct definition, optionally with a base-clause, and schedules the closing `};`. */
    [[nodiscard]] Scope enterStruct(std::string_view name, std::string_view base = "");

    /** Enters an enum (class or plain) with an optional fixed underlying type and schedules the closing `};`. */
    [[nodiscard]] Scope enterEnum(std::string_view name, std::string_view underlyingType = "", bool isClass = true);

    /** Opens an `#ifdef` block; the body is not indented and the footer is the matching `#endif`. */
    [[nodiscard]] Scope enterIfdef(std::string_view condition);

    /** Opens an `#ifndef` block; the body is not indented and the footer is the matching `#endif`. */
    [[nodiscard]] Scope enterIfndef(std::string_view condition);

    // --- Line and Content Emission ---

    /** Appends a single line (or a blank line when empty), applying the current indentation. */
    void emitLine(std::string_view line = "");

    /** Formats the arguments and appends the result as a line. */
    template <typename... Args> void emitLine(std::format_string<Args...> fmt, Args &&...args)
    {
        emitLine(std::format(fmt, std::forward<Args>(args)...));
    }

    /** Appends raw text verbatim; a trailing newline is not added unless present in text. */
    void emit(std::string_view text);

    /** Formats the arguments and appends the result verbatim. */
    template <typename... Args> void emit(std::format_string<Args...> fmt, Args &&...args)
    {
        emit(std::format(fmt, std::forward<Args>(args)...));
    }

    /** Appends text without applying indentation, preserving it exactly as given. */
    void emitRaw(std::string_view rawText);

    /** Splits multilineText on newlines and emits each line through emitLine, re-indenting and stripping CR. */
    void emitLines(std::string_view multilineText);

    /** Emits a blank line, collapsing runs of consecutive blanks into one. */
    void emitBlankLine();

    // --- C++ Idioms & Common Constructs ---

    /** Emits the standard `Auto-generated by EzDSL ...` banner comment block. */
    void emitBanner(std::string_view generatorName, std::string_view notice = "DO NOT EDIT.");

    /** Emits a single-line `//` comment. */
    void emitComment(std::string_view comment);

    /** Emits a multi-line Doxygen documentation comment with each line re-indented. */
    void emitDocComment(std::string_view doc);

    /** Emits a `// --- title ---` section divider surrounded by blank lines. */
    void emitSectionComment(std::string_view title);

    /** Emits an `#include` directive, choosing angle or quoted form based on isSystem. */
    void emitInclude(std::string_view header, bool isSystem = false);

    /** Emits `#pragma once`. */
    void emitPragmaOnce();

    /** Emits an `#ifndef`/`#define` include-guard pair for guardName. */
    void emitIncludeGuardStart(std::string_view guardName);

    /** Emits the `#endif` matching a guard opened by emitIncludeGuardStart. */
    void emitIncludeGuardEnd(std::string_view guardName);

    // --- Output Access ---

    /** Returns a const reference to the accumulated output buffer. */
    const std::string &str() const noexcept;

    /** Returns a non-owning view of the accumulated output buffer. */
    std::string_view view() const noexcept;

    /** Moves out the accumulated output, resetting all emitter state to its initial values. */
    std::string takeStr();

    /** Discards the accumulated output and resets all emitter state. */
    void clear();

    /** Returns the number of bytes currently buffered. */
    size_t size() const noexcept;

    /** Returns true when nothing has been buffered. */
    bool empty() const noexcept;

  private:
    /** Appends the configured indentation to the buffer at the start of a non-empty line, once per level. */
    void applyIndent();

  private:
    std::string m_buffer;         ///< Accumulated generated source text.
    size_t m_indentLevel{ 0 };    ///< Current indent depth applied at the start of each line.
    std::string m_indentString;   ///< Text emitted once per indentation level.
    bool m_atStartOfLine{ true }; ///< True when the next emitted text begins a fresh line.
    bool m_lastWasBlank{ false }; ///< True when the previous emitted line was blank (used to collapse runs).
};

} // namespace CodeGenerators

#endif // EZDSL_CPP_SOURCE_EMITTER_H
