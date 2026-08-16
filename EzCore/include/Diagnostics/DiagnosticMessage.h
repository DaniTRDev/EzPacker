#ifndef EZCORE_DIAGNOSTIC_MESSAGE_H
#define EZCORE_DIAGNOSTIC_MESSAGE_H

#include "EzCoreCommon.h"

enum DiagnosticMessageType : uint8_t
{
    Diag_Debug = 0, // The diagnostic contains debug information.
    Diag_Error,     // The diagnostic contains information about an error.
    Diag_Trace,     // The diagnostic contains information about a trace during a specific algorithm execution.
    Diag_Warning    // The diagnostic contains information that should be considered by the reader. MAY or MAY NOT be
                    // important.
};

/**
 * A note is an extra piece of information that can be attached into a diagnostic message.
 */
struct DiagnosticNote
{
    class SourceReference *m_sourceRef{ nullptr }; // Note was appended with a source reference.
    std::pmr::string m_noteContent{};
};

class DiagnosticMessage
{
  public:
    friend class DiagnosticBuilder; // Ensure the builder has access to private methods of this class.

    /**
     * Returns the type of the diagnostic message.
     * @return
     */
    DiagnosticMessageType getType() const;

    /**
     * Returns the primary source reference. It is used to locate the parent object/scope that executed an algorithm and
     * any of its sub-steps emitted a diagnostic.
     */
    class SourceReference *getPrimarySourceRef() const;

    /**
     * Appends a note to the diagnostic message.
     */
    void addNote(const DiagnosticNote &note);

    /**
     * Appends the string to the main message of the diagnostic.
     */
    void addMainMsg(const std::string_view &str);

    /**
     * Sets the primary source reference.
     */
    void setPrimarySourceRef(class SourceReference *sourceRef);

    /**
     * Sets the sender of the diagnostic message.
     */
    void setSender(const std::string_view &sender);

    /**
     * Sets the type of the diagnostic message.
     */
    void setType(DiagnosticMessageType type);

    /**
     * Returns the notes (if any) attached to this message.
     */
    const std::list<DiagnosticNote> &getNotes() const;

    /**
     * Returns the main message.
     */
    std::string_view getMainMsg() const;

    /**
     * Returns the sender of the message.
     */
    std::string_view getSender() const;

  private:
    /**
     * Default constructor is made private because a message is going to be built using a builder.
     */
    DiagnosticMessage() = default;

    /**
     * Creates a diagnostic message with the given type, main msg, sender and notes (if specified). Appending notes
     * after executing this constructor IS ALLOWED.
     *
     * This constructor is made private because a message is going to be built using a builder.
     */
    DiagnosticMessage(DiagnosticMessageType type,
                      class SourceReference *primarySourceRef,
                      const std::string_view &mainMsg,
                      const std::string_view &sender,
                      const std::list<DiagnosticNote> &notes = {});

  private:
    DiagnosticMessageType m_type;

    // A reference to the parent scope/object that executed a traverse operation and created a diagnostic in any of
    // its sub-steps.
    class SourceReference *m_primarySourceRef{ nullptr };

    std::list<DiagnosticNote> m_notes{}; // List ensure O(1) appends/removes (linked list).
    std::pmr::string m_mainMessage;      // The main message of the diagnostic.
    std::pmr::string m_sender;           // The component that sent the diagnostic.
};

#endif // EZCORE_DIAGNOSTIC_MESSAGE_H
