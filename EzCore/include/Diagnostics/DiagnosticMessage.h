#ifndef EZPACKER_DIAGNOSTICMESSAGE_H
#define EZPACKER_DIAGNOSTICMESSAGE_H

#include "EzCoreCommon.h"
#include "SourceManager/SourceManager.h"

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
    SourceReference *m_sourceRef{ nullptr }; // Note was appended with a source reference.
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
     * @return
     */
    SourceReference *getPrimarySourceRef() const;

    /**
     * Appends a note to the diagnostic message.
     * @param note
     */
    void addNote(const DiagnosticNote &note);

    /**
     * Appends the string to the main message of the diagnostic.
     * @param mainMsg
     */
    void addMainMsg(const std::pmr::string &str);

    /**
     * Sets the primary source reference.
     * @param sourceRef
     */
    void setPrimarySourceRef(SourceReference *sourceRef);

    /**
     * Sets the sender of the diagnostic message.
     * @param sender
     */
    void setSender(const std::pmr::string &sender);

    /**
     * Sets the type of the diagnostic message.
     * @param type
     */
    void setType(DiagnosticMessageType type);

    /**
     * Returns the notes (if any) attached to this message.
     * @return
     */
    const std::list<DiagnosticNote> &getNotes() const;

    /**
     * Returns the main message.
     * @return
     */
    const std::pmr::string &getMainMsg() const;

    /**
     * Returns the sender of the message.
     * @return
     */
    const std::pmr::string &getSender() const;

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
     * @param type
     * @param primarySourceRef
     * @param mainMsg
     * @param sender
     * @param notes
     */
    DiagnosticMessage(DiagnosticMessageType type,
                      SourceReference *primarySourceRef,
                      const std::pmr::string &mainMsg,
                      const std::pmr::string &sender,
                      const std::list<DiagnosticNote> &notes = {});
    
  private:
    DiagnosticMessageType m_type;
    SourceReference *m_primarySourceRef; // A reference to the parent scope/object that executed a traverse operation
                                         // and created a diagnostic in any of its sub-steps.
    std::list<DiagnosticNote> m_notes;   // List ensure O(1) appends/removes (linked list).
    std::pmr::string m_mainMessage;      // The main message of the diagnostic.
    std::pmr::string m_sender;           // The component that sent the diagnostic.
};

#endif // EZPACKER_DIAGNOSTICMESSAGE_H
