#include "Diagnostics/DiagnosticMessage.h"
#include "SourceManager/SourceManager.h"

DiagnosticMessage::DiagnosticMessage(std::pmr::memory_resource *alloc) :
    m_type(Diag_None), m_primarySourceRef(nullptr), m_mainMessage(alloc), m_sender(alloc), m_notes({})
{
}

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

DiagnosticMessageType DiagnosticMessage::getType() const { return m_type; }

class SourceReference *DiagnosticMessage::getPrimarySourceRef() const { return m_primarySourceRef; }

void DiagnosticMessage::addNote(const DiagnosticNote &note) { m_notes.push_back(note); }

void DiagnosticMessage::addMainMsg(const std::string_view &str) { m_mainMessage += str; }

void DiagnosticMessage::setPrimarySourceRef(class SourceReference *sourceRef) { m_primarySourceRef = sourceRef; }

void DiagnosticMessage::setSender(const std::string_view &sender)
{
    m_sender.clear();
    m_sender += sender;
}

void DiagnosticMessage::setType(DiagnosticMessageType type) { m_type = type; }

const std::list<DiagnosticNote> &DiagnosticMessage::getNotes() const { return m_notes; }

std::string_view DiagnosticMessage::getMainMsg() const { return m_mainMessage; }

std::string_view DiagnosticMessage::getSender() const { return m_sender; }
