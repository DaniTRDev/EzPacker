#include "Diagnostics/DiagnosticCollector.h"

/**
 * Initializes the arena-backed message/scope containers, enables error and warning diagnostics by
 * default and installs the permanent root scope.
 */
DiagnosticCollector::DiagnosticCollector()
{
    m_enabledDiags = Diag_Error | Diag_Warning;
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
DiagnosticBuilder DiagnosticCollector::builder(DiagnosticMessageType type, std::string_view sender)
{
    return DiagnosticBuilder(this, type, sender);
}

/**
 * Creates an error builder and appends the message only when error diagnostics are enabled;
 * otherwise the returned inactive builder silently absorbs further chained operations.
 */
DiagnosticBuilder DiagnosticCollector::error(std::string_view sender, std::string_view message)
{
    return buildAndAppend(Diag_Error, sender, "{}", message);
}

/**
 * Creates a trace builder and appends the message only when trace diagnostics are enabled.
 */
DiagnosticBuilder DiagnosticCollector::trace(std::string_view sender, std::string_view message)
{
    return buildAndAppend(Diag_Trace, sender, "{}", message);
}

/**
 * Creates a warning builder and appends the message only when warning diagnostics are enabled.
 */
DiagnosticBuilder DiagnosticCollector::warn(std::string_view sender, std::string_view message)
{
    return buildAndAppend(Diag_Warning, sender, "{}", message);
}

/**
 * Registers a listener under the collector mutex so it receives committed diagnostics. Null
 * listeners are ignored at registration so notification sites never need to special-case them.
 */
void DiagnosticCollector::addListener(DiagnosticListener *listener)
{
    if (!listener)
        return;

    std::lock_guard lock(m_mutex);
    m_listeners.push_back(listener);
}

/**
 * Removes every registration of the given listener under the collector mutex.
 */
void DiagnosticCollector::removeListener(DiagnosticListener *listener)
{
    if (!listener)
        return;

    std::lock_guard lock(m_mutex);
    m_listeners.remove(listener);
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
 * Replaces the enabled diagnostic type bitmask.
 */
void DiagnosticCollector::setEnabledDiags(DiagnosticMessageType types)
{
    m_enabledDiags.store(static_cast<uint8_t>(types));
}

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
            // Notify listeners directly from the closing scope. The scope (and its messages) stays
            // alive until this function returns, so no collector-side retained copy is required and
            // committed diagnostics do not accumulate for the collector's lifetime.
            for (const auto &msg : closingScope.getMessages())
            {
                for (auto *listener : m_listeners)
                {
                    if (listener)
                    {
                        listener->onDiag(msg);
                    }
                }
            }
            break;
        }
        case DiagnosticScopeAction::Propagate:
        {
            // Move messages into the parent scope instead of copying them.
            closingScope.moveMessagesTo(m_scopes.back());
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
 * Receives a message from a builder. At the root scope it notifies listeners immediately; inside
 * nested scopes it buffers the message for the enclosing scope to commit, propagate or discard.
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
        // The by-value message is alive for the whole notification, so it can be passed directly
        // without retaining a collector-lifetime copy.
        for (auto *listener : m_listeners)
        {
            if (listener)
            {
                listener->onDiag(message);
            }
        }
    }
    else
    {
        scope.appendMessage(std::move(message));
    }
}

/**
 * Exposes the synchronized pool used to allocate diagnostic messages and scopes.
 */
std::pmr::memory_resource *DiagnosticCollector::getAllocator() { return &m_diagScopePool; }