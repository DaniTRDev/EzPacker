#include "Diagnostics/DiagnosticCollector.h"

DiagnosticCollector::DiagnosticCollector()
{
    m_messages = std::pmr::vector<DiagnosticMessage>(&m_diagScopePool);
    m_scopes = std::pmr::vector<DiagnosticScope>(&m_diagScopePool);
    m_scopes.emplace_back(&m_diagScopePool); // Ensure there's at least 1 scope available.
}

DiagnosticBuilder DiagnosticCollector::builder(DiagnosticMessageType type, const std::string_view &sender)
{
    return DiagnosticBuilder(this, type, sender);
}

void DiagnosticCollector::addListener(DiagnosticListener *listener) { m_listeners.push_back(listener); }

void DiagnosticCollector::beginScope(DiagnosticScopeAction action)
{
    DiagnosticScope scope(&m_diagScopePool);
    scope.setAction(action);

    std::lock_guard lock(m_mutex);
    m_scopes.push_back(std::move(scope));
}

void DiagnosticCollector::endScope()
{
    std::lock_guard lock(m_mutex);
    if (m_scopes.size() <= 1)
    {
        throw std::runtime_error("Compiler Error: Attempted to pop the root diagnostic scope.");
    }

    // Extract the active scope
    auto closingScope = m_scopes.back();
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

void DiagnosticCollector::setScopeAction(DiagnosticScopeAction action)
{
    std::lock_guard lock(m_mutex);
    if (m_scopes.size() > 1)
    {
        m_scopes.back().setAction(action);
    }
}

void DiagnosticCollector::onDiag(DiagnosticMessage message)
{
    std::scoped_lock lock(m_mutex);

    if (m_scopes.empty())
    {
        throw std::runtime_error("Internal Compiler Error: Tried to push a message to a non-existent scope.");
    }

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

    if (message.getType() == Diag_Error)
    {
        scope.setHasFatalErrors(true);
    }
}
