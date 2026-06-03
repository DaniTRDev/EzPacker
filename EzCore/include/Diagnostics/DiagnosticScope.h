#ifndef EZPACKER_DIAGNOSTICSCOPE_H
#define EZPACKER_DIAGNOSTICSCOPE_H

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
     * @param pool
     */
    DiagnosticScope(std::pmr::memory_resource *pool);

    /**
     * Returns the action to be performed on the messages on scope's end.
     * @return
     */
    DiagnosticScopeAction getAction() const;

    /**
     * Appends a message to the current scope. Creates a copy used the arena-managed vector.
     * @param message
     */
    void appendMessage(const DiagnosticMessage &message);

    /**
     * Inserts the given range of a vector into the current scope.
     * @param begin
     * @param end
     */
    void insert(std::pmr::vector<DiagnosticMessage>::const_iterator begin,
                std::pmr::vector<DiagnosticMessage>::const_iterator end);

    /**
     * Sets the action to be performed on the messages on scope's end.
     * @param action
     */
    void setAction(DiagnosticScopeAction action);

    /**
     * Returns the list of arena-backed messages.
     * @return
     */
    const std::pmr::vector<DiagnosticMessage> &getMessages() const;

  private:
    bool m_hasFatalErrors;
    DiagnosticScopeAction m_action;
    std::pmr::vector<DiagnosticMessage> m_messages;
};

#endif // EZPACKER_DIAGNOSTICSCOPE_H
