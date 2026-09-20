#include "Diagnostics/DiagnosticCollector.h"

/**
 * Initializes the arena-backed message/scope containers, enables error and warning diagnostics by
 * default and installs the permanent root scope.
 */
DiagnosticCollector::DiagnosticCollector()
{
    m_enabledDiags = Diag_Error | Diag_Warning;
    m_messages = std::pmr::vector<DiagnosticMessage>(&m_diagScopePool);
    m_scopes = std::pmr::vector<DiagnosticScope>(&m_diagScopePool);
    m_scopes.emplace_back(&m_diagScopePool); // Ensure there's at least 1 scope available.
}

/**
 * Returns true when the given type's bit is set among the enabled diagnostics.
 */
bool DiagnosticCollector::isDiagEnabledForType(DiagnosticMessageType type) const
{
    return (m_enabledDiags.load() & type) != 0;
}

/**
 * Creates a builder attached to this collector with the given type and sender.
 */
DiagnosticBuilder DiagnosticCollector::builder(DiagnosticMessageType type, const std::string_view &sender)
{
    return DiagnosticBuilder(this, type, sender);
}

/**
 * Creates an error builder and appends the message only when error diagnostics are enabled;
 * otherwise the returned inactive builder silently absorbs further chained operations.
 */
DiagnosticBuilder DiagnosticCollector::error(const std::string_view &sender, const std::string_view &message)
{
    return buildAndAppend(Diag_Error, sender, "{}", message);
}

/**
 * Creates a trace builder and appends the message only when trace diagnostics are enabled.
 */
DiagnosticBuilder DiagnosticCollector::trace(const std::string_view &sender, const std::string_view &message)
{
    return buildAndAppend(Diag_Trace, sender, "{}", message);
}

/**
 * Registers a listener under the collector mutex so it receives committed diagnostics.
 */
void DiagnosticCollector::addListener(DiagnosticListener *listener)
{
    std::lock_guard lock(m_mutex);
    m_listeners.push_back(listener);
}

/**
 * Pushes a new scope, configured with the given commit/discard/propagate action, onto the scope stack.
 */
void DiagnosticCollector::beginScope(DiagnosticScopeAction action)
{
    DiagnosticScope scope(&m_diagScopePool);
    scope.setAction(action);

    std::lock_guard lock(m_mutex);
    m_scopes.push_back(std::move(scope));
}

/**
 * Adds the given diagnostic type to the enabled bitmask.
 */
void DiagnosticCollector::enableDiag(DiagnosticMessageType type) { m_enabledDiags.fetch_or(type); }

/**
 * Pops the innermost scope and applies its action: Commit forwards messages to listeners and the
 * permanent record, Propagate moves them to the parent scope, Discard drops them. Throws if the
 * root scope is the only one remaining.
 */
void DiagnosticCollector::endScope()
{
    std::lock_guard lock(m_mutex);
    if (m_scopes.size() <= 1)
    {
        throw std::runtime_error("Compiler Error: Attempted to pop the root diagnostic scope.");
    }

    // Extract the active scope
    auto closingScope = std::move(m_scopes.back());
    m_scopes.pop_back();

    switch (closingScope.getAction())
    {
        case DiagnosticScopeAction::Commit:
        {
            for (auto &msg : closingScope.getMessages())
            {
                // Record the message and notify every listener.

                m_messages.push_back(msg);
                for (auto *listener : m_listeners)
                {
                    if (listener)
                    {
                        listener->onDiag(m_messages.back());
                    }
                }
            }
            break;
        }
        case DiagnosticScopeAction::Propagate:
        {
            // Pass messages cleanly to the parent scope vector
            auto &parentScope = m_scopes.back();
            parentScope.insert(closingScope.getMessages().begin(), closingScope.getMessages().end());
            break;
        }
        case DiagnosticScopeAction::Discard:
        {
            // Scope is freed after this function returns.
            break;
        }
    }
}

/**
 * Receives a message from a builder. At the root scope it notifies listeners immediately and
 * records the message; inside nested scopes it buffers the message. Error messages additionally
 * mark the current scope as having fatal errors.
 */
void DiagnosticCollector::onDiag(DiagnosticMessage message)
{
    std::scoped_lock lock(m_mutex);

    if (m_scopes.empty())
    {
        throw std::runtime_error("Internal Compiler Error: Tried to push a message to a non-existent scope.");
    }

    if (!isDiagEnabledForType(message.getType()))
        return;

    DiagnosticScope &scope = m_scopes.back();
    if (m_scopes.size() == 1)
    {
        for (auto &listener : m_listeners)
        {
            listener->onDiag(message);
        }

        // Move the message into the permanent record of messages.
        m_messages.push_back(message);
    }
    else
    {
        scope.appendMessage(message);
    }
}

/**
 * Exposes the synchronized pool used to allocate diagnostic messages and scopes.
 */
std::pmr::memory_resource *DiagnosticCollector::getAllocator() { return &m_diagScopePool; }