#include "Diagnostics/DiagnosticMessage.h"

DiagnosticMessage::DiagnosticMessage(DiagnosticMessageType type,
                                     SourceReference *primarySourceRef,
                                     const std::pmr::string &mainMsg,
                                     const std::pmr::string &sender,
                                     const std::list<DiagnosticNote> &notes) :
    m_type(type), m_primarySourceRef(primarySourceRef), m_mainMessage(mainMsg), m_sender(sender), m_notes(notes)
{
}

DiagnosticMessageType DiagnosticMessage::getType() const { return m_type; }

SourceReference *DiagnosticMessage::getPrimarySourceRef() const { return m_primarySourceRef; }

void DiagnosticMessage::addNote(const DiagnosticNote &note) { m_notes.push_back(note); }

void DiagnosticMessage::addMainMsg(const std::pmr::string &str) { m_mainMessage += str; }

void DiagnosticMessage::setPrimarySourceRef(SourceReference *sourceRef) { m_primarySourceRef = sourceRef; }

void DiagnosticMessage::setSender(const std::pmr::string &sender) { m_sender = sender; }

void DiagnosticMessage::setType(DiagnosticMessageType type) { m_type = type; }

const std::list<DiagnosticNote> &DiagnosticMessage::getNotes() const { return m_notes; }

const std::pmr::string &DiagnosticMessage::getMainMsg() const { return m_mainMessage; }

const std::pmr::string &DiagnosticMessage::getSender() const { return m_sender; }
