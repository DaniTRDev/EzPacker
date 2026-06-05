#ifndef EZPACKER_DIAGNOSTICCOLLECTOR_H
#define EZPACKER_DIAGNOSTICCOLLECTOR_H

#include "EzCoreCommon.h"
#include "DiagnosticBuilder.h"
#include "DiagnosticMessage.h"
#include "DiagnosticListener.h"
#include "DiagnosticScope.h"

/**
 * This class is the responsible of creating diagnostic builders to emit messages into scopes and later handle the
 * action of each one. It's thread-safe by default.
 */
class DiagnosticCollector
{
  public:
    /**
     * Creates the collector.
     */
    DiagnosticCollector();

    /**
     * Returns a diagnostic builder with the main diagnostic information.
     * @param type
     * @param sender
     * @return
     */
    DiagnosticBuilder builder(DiagnosticMessageType type, const std::string_view &sender);

    /**
     * Adds a listener for diagnostic messages.
     * @param listener
     */
    void addListener(DiagnosticListener *listener);

    /**
     * Begins a new scope with a default action.
     * @param action
     */
    void beginScope(DiagnosticScopeAction action);

    /**
     * Ends the current scope. If it is the top most scope, an exception is thrown.
     */
    void endScope();

    /**
     * Sets the current scope's action. This does not affect top-most scope.
     * @param action
     */
    void setScopeAction(DiagnosticScopeAction action);

    /**
     * Pushes a new message to the diagnostic message list. If the current scope is the top-most (1), every listener
     * is notified.
     * @param msg
     */
    void onDiag(DiagnosticMessage message);

  private:
    std::list<DiagnosticListener *> m_listeners;
    std::pmr::unsynchronized_pool_resource m_diagScopePool;
    std::pmr::vector<DiagnosticMessage> m_messages; // A set of notified messages. Will be filled with elements that
                                                    // were actually notified to the listener.
    std::pmr::vector<DiagnosticScope> m_scopes;
    std::recursive_mutex m_mutex;
};

#endif // EZPACKER_DIAGNOSTICCOLLECTOR_H
