#ifndef EZCORE_DIAGNOSTIC_COLLECTOR_H
#define EZCORE_DIAGNOSTIC_COLLECTOR_H

#include "EzCoreCommon.h"
#include "DiagnosticBuilder.h"
#include "DiagnosticMessage.h"
#include "DiagnosticListener.h"
#include "DiagnosticScope.h"
#include <atomic>

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
    DiagnosticBuilder builder(DiagnosticMessageType type, std::string_view sender);

    /**
     * Creates an error diagnostic builder with a predefined format message and sender. If error diag is not enabled,
     * this function will return an empty builder.
     */
    template <typename... Args>
    DiagnosticBuilder error(std::string_view sender, std::format_string<Args...> fmt, Args &&...args)
    {
        return buildAndAppend(Diag_Error, sender, fmt, std::forward<Args>(args)...);
    }

    /**
     * Creates an error diagnostic builder with a predefined message and sender. If error diag is not enabled, this
     * function returns an empty builder.
     */
    DiagnosticBuilder error(std::string_view sender, std::string_view message);
    /**
     * Creates an error diagnostic builder with a predefined format message and sender. If trace diag is not enabled,
     * this function will return an empty builder.
     */
    template <typename... Args>
    DiagnosticBuilder trace(std::string_view sender, std::format_string<Args...> fmt, Args &&...args)
    {
        return buildAndAppend(Diag_Trace, sender, fmt, std::forward<Args>(args)...);
    }

    /**
     * Creates a trace diagnostic builder with a predefined message and sender. If trace diag is not enabled, this
     * function returns an empty builder.
     */
    DiagnosticBuilder trace(std::string_view sender, std::string_view message);

    /**
     * Creates a warning diagnostic builder with a predefined format message and sender. If warning diag is not
     * enabled, this function returns an empty builder.
     */
    template <typename... Args>
    DiagnosticBuilder warn(std::string_view sender, std::format_string<Args...> fmt, Args &&...args)
    {
        return buildAndAppend(Diag_Warning, sender, fmt, std::forward<Args>(args)...);
    }

    /**
     * Creates a warning diagnostic builder with a predefined message and sender. If warning diag is not enabled,
     * this function returns an empty builder.
     */
    DiagnosticBuilder warn(std::string_view sender, std::string_view message);

    /**
     * Adds a listener for diagnostic messages. A null listener is ignored, so callers may pass an
     * optional listener without checking it first.
     */
    void addListener(DiagnosticListener *listener);

    /**
     * Unregisters a previously added listener. Unknown or null listeners are ignored.
     */
    void removeListener(DiagnosticListener *listener);

    /**
     * Begins a new scope with a default action.
     */
    void beginScope(DiagnosticScopeAction action);

    /**
     * Enables the diagnostic for the given message type.
     */
    void enableDiag(DiagnosticMessageType type);

    /**
     * Replaces the set of enabled diagnostic types with the given bitmask. Used to apply a
     * severity threshold (e.g. error-only, or error+warning+trace) after construction.
     */
    void setEnabledDiags(DiagnosticMessageType types);

    /**
     * Ends the current scope. If it is the top most scope, an exception is thrown.
     */
    void endScope();

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
    /**
     * Shared implementation for the error/trace convenience overloads: builds a message of the
     * given type and appends the formatted text only when that type is enabled, so disabled
     * diagnostics never pay for formatting.
     */
    template <typename... Args>
    DiagnosticBuilder
    buildAndAppend(DiagnosticMessageType type, std::string_view sender, std::format_string<Args...> fmt, Args &&...args)
    {
        auto b = builder(type, sender);

        // If disabled, 'b' acts as an inactive dummy builder that consumes chained calls for free.
        if (isDiagEnabledForType(type))
        {
            b << std::format(fmt, std::forward<Args>(args)...);
        }

        return b;
    }

    std::atomic<uint8_t> m_enabledDiags; // Enabled diagnostic types; atomic to match the documented thread-safety.
    std::list<DiagnosticListener *> m_listeners;          // Registered observers notified when messages are committed.
    std::pmr::synchronized_pool_resource m_diagScopePool; // Thread-safe arena backing all messages and scopes.
    std::pmr::vector<DiagnosticScope> m_scopes; // Stack of active scopes; index 0 is the always-present root scope.
    std::recursive_mutex m_mutex;               // Guards listeners, scopes and messages from concurrent access.
};

#endif // EZCORE_DIAGNOSTIC_COLLECTOR_H
