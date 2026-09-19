#include "Diagnostics/DiagnosticScope.h"

/**
 * Creates a scope backed by the given arena, defaulting to Propagate and no fatal errors.
 */
DiagnosticScope::DiagnosticScope(std::pmr::memory_resource *pool) :
    m_hasFatalErrors(false), m_action(DiagnosticScopeAction::Propagate), m_messages(pool)
{
}

/**
 * Returns how the messages will be handled when the scope closes.
 */
DiagnosticScopeAction DiagnosticScope::getAction() const { return m_action; }

/**
 * Copies the message into the arena-managed vector of this scope.
 */
void DiagnosticScope::appendMessage(const DiagnosticMessage &message) { m_messages.push_back(message); }

/**
 * Moves the half-open range [begin, end) of messages to the end of this scope's message list.
 */
void DiagnosticScope::insert(std::pmr::vector<DiagnosticMessage>::const_iterator begin,
                             std::pmr::vector<DiagnosticMessage>::const_iterator end)
{
    m_messages.insert(m_messages.end(), begin, end);
}

/**
 * Selects the disposition applied to the collected messages on scope exit.
 */
void DiagnosticScope::setAction(DiagnosticScopeAction action) { m_action = action; }

/**
 * Flags this scope as containing at least one fatal error.
 */
void DiagnosticScope::setHasFatalErrors(bool value) { m_hasFatalErrors = value; }

/**
 * Returns the arena-backed list of messages accumulated in this scope.
 */
const std::pmr::vector<DiagnosticMessage> &DiagnosticScope::getMessages() const { return m_messages; }
