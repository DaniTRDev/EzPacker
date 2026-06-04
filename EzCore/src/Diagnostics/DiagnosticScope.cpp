#include "Diagnostics/DiagnosticScope.h"

DiagnosticScope::DiagnosticScope(std::pmr::memory_resource *pool) : m_messages(pool) {}

DiagnosticScopeAction DiagnosticScope::getAction() const { return m_action; }

void DiagnosticScope::appendMessage(const DiagnosticMessage &message) { m_messages.push_back(message); }

void DiagnosticScope::insert(std::pmr::vector<DiagnosticMessage>::const_iterator begin,
                             std::pmr::vector<DiagnosticMessage>::const_iterator end)
{
    m_messages.insert(m_messages.end(), begin, end);
}

void DiagnosticScope::setAction(DiagnosticScopeAction action) { m_action = action; }

void DiagnosticScope::setHasFatalErrors(bool value) { m_hasFatalErrors = value; }

const std::pmr::vector<DiagnosticMessage> &DiagnosticScope::getMessages() const { return m_messages; }
