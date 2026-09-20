#ifndef EZCORE_DIAGNOSTIC_SCOPE_H
#define EZCORE_DIAGNOSTIC_SCOPE_H

#include "EzCoreCommon.h"
#include "DiagnosticMessage.h"

enum class DiagnosticScopeAction : uint8_t
{
    Commit,   // Flush current scopes' messages to listeners immediately.
    Discard,  // Completely drop all messages gathered inside this scope.
    Propagate // Bubble up messages to the parent scope. If there is no scope, they are commited.
};

class DiagnosticScope
{
  public:
    /**
     * Creates a scope linked to a pool that will contain a list of diagnostic messages.
     */
    DiagnosticScope(std::pmr::memory_resource *pool);

    /**
     * Returns the action to be performed on the messages on scope's end.
     */
    DiagnosticScopeAction getAction() const;

    /**
     * Appends a message to the current scope. Creates a copy used the arena-managed vector.
     */
    void appendMessage(const DiagnosticMessage &message);

    /**
     * Inserts the given range of a vector into the current scope.
     */
    void insert(std::pmr::vector<DiagnosticMessage>::const_iterator begin,
                std::pmr::vector<DiagnosticMessage>::const_iterator end);

    /**
     * Sets the action to be performed on the messages on scope's end.
     */
    void setAction(DiagnosticScopeAction action);

    /**
     * Returns the list of arena-backed messages.
     */
    const std::pmr::vector<DiagnosticMessage> &getMessages() const;

  private:
    DiagnosticScopeAction m_action;                 // How the collected messages are handled when the scope ends.
    std::pmr::vector<DiagnosticMessage> m_messages; // Arena-allocated messages gathered while inside the scope.
};

#endif // EZCORE_DIAGNOSTIC_SCOPE_H
