#ifndef EZCORE_DIAGNOSTIC_MESSAGE_H
#define EZCORE_DIAGNOSTIC_MESSAGE_H

#include "EzCoreCommon.h"

/**
 * Bitflag enumeration categorizing the severity and purpose of a diagnostic message.
 */
enum DiagnosticMessageType : uint8_t
{
    Diag_None = 0,
    Diag_Debug = 1,         // Debug information emitted during development or tracing.
    Diag_Error = (1 << 1),  // Error condition preventing compilation or analysis.
    Diag_Trace = (1 << 2),  // Trace information during specific compiler passes.
    Diag_Warning = (1 << 3) // Warning condition that does not halt compilation.
};

/**
 * Supplementary note attached to a diagnostic message providing additional context or source spans.
 */
struct DiagnosticNote
{
    class SourceReference *m_sourceRef{ nullptr }; // Source span this note refers to, or nullptr.
    std::pmr::string m_noteContent{};              // Arena-allocated note text.
};

/**
 * Diagnostic record containing severity, sender component, primary source span, main message, and attached notes.
 * Managed and emitted through DiagnosticCollector and DiagnosticBuilder.
 */
class DiagnosticMessage
{
  public:
    friend class DiagnosticBuilder;

    /**
     * Returns the severity classification of this diagnostic message.
     */
    DiagnosticMessageType getType() const;

    /**
     * Returns the primary source reference pointing to the source code span where the error/warning originated.
     */
    class SourceReference *getPrimarySourceRef() const;

    /**
     * Appends an additional DiagnosticNote to this message.
     */
    void addNote(const DiagnosticNote &note);

    /**
     * Appends string text to the primary diagnostic description buffer.
     */
    void addMainMsg(const std::string_view &str);

    /**
     * Sets the primary source span reference for this diagnostic.
     */
    void setPrimarySourceRef(class SourceReference *sourceRef);

    /**
     * Sets the identifier name of the compiler component emitting this message.
     */
    void setSender(const std::string_view &sender);

    /**
     * Sets the diagnostic severity type.
     */
    void setType(DiagnosticMessageType type);

    /**
     * Returns the list of contextual notes attached to this diagnostic.
     */
    const std::list<DiagnosticNote> &getNotes() const;

    /**
     * Returns a string view of the primary message text.
     */
    std::string_view getMainMsg() const;

    /**
     * Returns a string view of the sender component name.
     */
    std::string_view getSender() const;

  private:
    /**
     * Internal constructor initializing PMR strings with the specified memory resource.
     */
    DiagnosticMessage(std::pmr::memory_resource *alloc);

    DiagnosticMessageType m_type; // Severity classification of this message.

    // A reference to the parent scope/object that executed a traverse operation and created a diagnostic in any of
    // its sub-steps.
    class SourceReference *m_primarySourceRef{ nullptr };

    std::list<DiagnosticNote> m_notes{}; // List ensure O(1) appends/removes (linked list).
    std::pmr::string m_mainMessage;      // The main message of the diagnostic.
    std::pmr::string m_sender;           // The component that sent the diagnostic.
};

#endif // EZCORE_DIAGNOSTIC_MESSAGE_H
