#include "Diagnostics/DiagnosticMessage.h"
#include "SourceManager/SourceManager.h"

DiagnosticMessage::DiagnosticMessage(DiagnosticMessageType type,
                                     class SourceReference *primarySourceRef,
                                     const std::string_view &mainMsg,
                                     const std::string_view &sender,
                                     const std::list<DiagnosticNote> &notes) :
    m_type(type), m_primarySourceRef(primarySourceRef), m_mainMessage(mainMsg), m_sender(sender), m_notes(notes)
{
}

DiagnosticMessageType DiagnosticMessage::getType() const { return m_type; }

class SourceReference *DiagnosticMessage::getPrimarySourceRef() const { return m_primarySourceRef; }

void DiagnosticMessage::addNote(const DiagnosticNote &note) { m_notes.push_back(note); }

void DiagnosticMessage::addMainMsg(const std::string_view &str) { m_mainMessage += str; }

void DiagnosticMessage::setPrimarySourceRef(class SourceReference *sourceRef) { m_primarySourceRef = sourceRef; }

void DiagnosticMessage::setSender(const std::string_view &sender) { m_sender = sender; }

void DiagnosticMessage::setType(DiagnosticMessageType type) { m_type = type; }

const std::list<DiagnosticNote> &DiagnosticMessage::getNotes() const { return m_notes; }

std::string_view DiagnosticMessage::getMainMsg() const { return m_mainMessage; }

std::string_view DiagnosticMessage::getSender() const { return m_sender; }
