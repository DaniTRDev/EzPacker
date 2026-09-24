#include "Diagnostics/DiagnosticScope.h"
#include <iterator>

/**
 * Creates a scope backed by the given arena, defaulting to Propagate.
 */
DiagnosticScope::DiagnosticScope(std::pmr::memory_resource *pool) :
    m_action(DiagnosticScopeAction::Propagate), m_messages(pool)
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
 * Moves the message into the arena-managed vector of this scope.
 */
void DiagnosticScope::appendMessage(DiagnosticMessage &&message) { m_messages.push_back(std::move(message)); }

/**
 * Move-inserts every message of this scope into target, then clears this scope's list.
 */
void DiagnosticScope::moveMessagesTo(DiagnosticScope &target)
{
    target.m_messages.insert(target.m_messages.end(),
                             std::make_move_iterator(m_messages.begin()),
                             std::make_move_iterator(m_messages.end()));
    m_messages.clear();
}

/**
 * Selects the disposition applied to the collected messages on scope exit.
 */
void DiagnosticScope::setAction(DiagnosticScopeAction action) { m_action = action; }

/**
 * Returns the arena-backed list of messages accumulated in this scope.
 */
const std::pmr::vector<DiagnosticMessage> &DiagnosticScope::getMessages() const { return m_messages; }
