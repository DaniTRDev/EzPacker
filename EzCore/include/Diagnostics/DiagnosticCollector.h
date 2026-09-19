#ifndef EZCORE_DIAGNOSTIC_COLLECTOR_H
#define EZCORE_DIAGNOSTIC_COLLECTOR_H

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
     * Returns true if the given message type can be pushed into the collectors queue.
     */
    bool isDiagEnabledForType(DiagnosticMessageType type) const;

    /**
     * Returns a diagnostic builder with the main diagnostic information.
     */
    DiagnosticBuilder builder(DiagnosticMessageType type, const std::string_view &sender);

    /**
     * Creates an error diagnostic builder with a predefined format message and sender. If error diag is not enabled,
     * this function will return an empty builder.
     */
    template <typename... Args>
    DiagnosticBuilder error(const std::string_view &sender, std::format_string<Args...> fmt, Args &&...args)
    {
        auto b = builder(Diag_Error, sender);

        // Only format and append if enabled.
        // If disabled, 'b' acts as an inactive dummy builder that consumes chained calls for free.
        if (isDiagEnabledForType(Diag_Error))
        {
            b << std::format(fmt, std::forward<Args>(args)...);
        }

        return b;
    }

    /**
     * Creates an error diagnostic builder with a predefined message and sender. If error diag is not enabled, this
     * function returns an empty builder.
     */
    DiagnosticBuilder error(const std::string_view &sender, const std::string_view &message);
    /**
     * Creates an error diagnostic builder with a predefined format message and sender. If trace diag is not enabled,
     * this function will return an empty builder.
     */
    template <typename... Args>
    DiagnosticBuilder trace(const std::string_view &sender, std::format_string<Args...> fmt, Args &&...args)
    {
        auto b = builder(Diag_Trace, sender);

        if (isDiagEnabledForType(Diag_Trace))
        {
            b << std::format(fmt, std::forward<Args>(args)...);
        }

        return b;
    }

    /**
     * Creates a trace diagnostic builder with a predefined message and sender. If trace diag is not enabled, this
     * function returns an empty builder.
     */
    DiagnosticBuilder trace(const std::string_view &sender, const std::string_view &message);

    /**
     * Adds a listener for diagnostic messages.
     */
    void addListener(DiagnosticListener *listener);

    /**
     * Begins a new scope with a default action.
     */
    void beginScope(DiagnosticScopeAction action);

    /**
     * Enables the diagnostic for the given message type.
     */
    void enableDiag(DiagnosticMessageType type);

    /**
     * Ends the current scope. If it is the top most scope, an exception is thrown.
     */
    void endScope();

    /**
     * Sets the current scope's action. This does not affect top-most scope.
     */
    void setScopeAction(DiagnosticScopeAction action);

    /**
     * Pushes a new message to the diagnostic message list. If the current scope is the top-most (1), every listener
     * is notified.
     */
    void onDiag(DiagnosticMessage message);

    /**
     * Returns the allocator of this class.
     */
    std::pmr::memory_resource *getAllocator();

  private:
    uint8_t m_enabledDiags;                               // Used to know which diagnostic types are enabled.
    std::list<DiagnosticListener *> m_listeners;          // Registered observers notified when messages are committed.
    std::pmr::synchronized_pool_resource m_diagScopePool; // Thread-safe arena backing all messages and scopes.
    std::pmr::vector<DiagnosticMessage> m_messages; // A set of notified messages. Will be filled with elements that
                                                    // were actually notified to the listener.
    std::pmr::vector<DiagnosticScope> m_scopes;     // Stack of active scopes; index 0 is the always-present root scope.
    std::recursive_mutex m_mutex;                   // Guards listeners, scopes and messages from concurrent access.
};

#endif // EZCORE_DIAGNOSTIC_COLLECTOR_H
