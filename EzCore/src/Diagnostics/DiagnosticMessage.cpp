#include "Diagnostics/DiagnosticMessage.h"
#include "SourceManager/SourceManager.h"

/**
 * Creates an empty message (type Diag_None, no source reference or notes) using the given arena.
 */
DiagnosticMessage::DiagnosticMessage(std::pmr::memory_resource *alloc) :
    m_type(Diag_None), m_primarySourceRef(nullptr), m_mainMessage(alloc), m_sender(alloc), m_notes({})
{
}

/**
 * Creates a fully-populated message, copying the main text and sender into arena-backed strings
 * and adopting the supplied note list.
 */
DiagnosticMessage::DiagnosticMessage(DiagnosticMessageType type,
                                     class SourceReference *primarySourceRef,
                                     const std::string_view &mainMsg,
                                     const std::string_view &sender,
                                     std::pmr::memory_resource *alloc,
                                     const std::list<DiagnosticNote> &notes) :
    m_type(type), m_primarySourceRef(primarySourceRef), m_mainMessage(alloc), m_sender(alloc), m_notes(notes)
{
    // Ensure copy.
    m_mainMessage += mainMsg;
    m_sender += sender;
}

/**
 * Returns the severity classification of this message.
 */
DiagnosticMessageType DiagnosticMessage::getType() const { return m_type; }

/**
 * Returns the primary source span this message points at, or nullptr when none was set.
 */
class SourceReference *DiagnosticMessage::getPrimarySourceRef() const { return m_primarySourceRef; }

/**
 * Appends a contextual note to this message.
 */
void DiagnosticMessage::addNote(const DiagnosticNote &note) { m_notes.push_back(note); }

/**
 * Appends text to the main message body.
 */
void DiagnosticMessage::addMainMsg(const std::string_view &str) { m_mainMessage += str; }

/**
 * Sets the primary source span used when rendering this message.
 */
void DiagnosticMessage::setPrimarySourceRef(class SourceReference *sourceRef) { m_primarySourceRef = sourceRef; }

/**
 * Replaces the sender identifier, clearing any previously stored value first.
 */
void DiagnosticMessage::setSender(const std::string_view &sender)
{
    m_sender.clear();
    m_sender += sender;
}

/**
 * Sets the severity classification of this message.
 */
void DiagnosticMessage::setType(DiagnosticMessageType type) { m_type = type; }

/**
 * Returns the notes attached to this message.
 */
const std::list<DiagnosticNote> &DiagnosticMessage::getNotes() const { return m_notes; }

/**
 * Returns the main message text.
 */
std::string_view DiagnosticMessage::getMainMsg() const { return m_mainMessage; }

/**
 * Returns the sender identifier of the component that produced this message.
 */
std::string_view DiagnosticMessage::getSender() const { return m_sender; }
