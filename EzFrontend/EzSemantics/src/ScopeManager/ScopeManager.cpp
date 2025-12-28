#include "ScopeManager/ScopeManager.h"

ScopeManager::ScopeManager(std::map<size_t, std::shared_ptr<Scope>> scopes) : m_scopes(std::move(scopes)) {}

bool ScopeManager::createSymbolAtCurrentScope(SymbolType symbolType,
                                              std::string name,
                                              std::string symbolDataType,
                                              std::shared_ptr<Symbol> *sym)
{
    if (!m_currentScope)
        beginScope();

    if (m_currentScope->searchSymbolByName(name, sym))
    {
        return false;
    }

    uint64_t seed = m_currentScope->getId() * 0x2557340b9cf3abdeULL;
    uint64_t id = ((seed >> 22u) ^ seed) * 6364136223846793005ULL;
    id = (((id >> 28u) ^ id) * 0x2557340b9cf3abdeULL) >> 22u;
    id ^= (((id >> 28u) ^ m_currentScope->getSymbols().size()) * 0x2557340b9cf3abdeULL) >> 22u;

    std::shared_ptr<Symbol> created =
            std::make_shared<Symbol>(id, symbolType, std::move(name), std::move(symbolDataType));

    if (sym)
        *sym = created;

    m_currentScope->appendSymbol(std::move(created));
    return true;
}

bool ScopeManager::searchById(size_t id, size_t minimumLevel, size_t maximumLevel, std::shared_ptr<Scope> *scope)
{
    return searchByIdImpl(id, minimumLevel, maximumLevel, m_scopes, scope);
}

bool ScopeManager::searchSymbolById(size_t id, size_t minimumLevel, size_t maximumLevel, std::shared_ptr<Symbol> *sym)
{
    bool found = false;
    static auto onSymbolCallback = [&found, &id, sym](const std::shared_ptr<Scope> &scope,
                                                      size_t scopeTreeLevel) -> bool
    {
        found = scope->searchSymbolById(id, sym);
        return !found; // Stop traversing when found becomes true (which will return false).
    };

    forEach(minimumLevel, maximumLevel, onSymbolCallback);
    return found;
}

bool ScopeManager::searchSymbolByName(const std::string &name,
                                      size_t minimumLevel,
                                      size_t maximumLevel,
                                      std::shared_ptr<Symbol> *sym)
{
    bool found = false;
    static auto onSymbolCallback = [&found, &name, sym](const std::shared_ptr<Scope> &scope,
                                                        size_t scopeTreeLevel) -> bool
    {
        found = scope->searchSymbolByName(name, sym);
        return !found; // Stop traversing when found becomes true (which will return false).
    };

    forEach(minimumLevel, maximumLevel, onSymbolCallback);
    return found;
}

bool ScopeManager::forEachImpl(size_t maximumTreeLevel,
                               size_t currentLevel,
                               const ScopeForEachCallbackT &callback,
                               const std::map<size_t, std::shared_ptr<Scope>> &scopes)
{
    if (currentLevel >= maximumTreeLevel)
        return true;

    for (auto &[id, scope] : scopes)
    {
        if (!callback(scope, currentLevel) ||
            !forEachImpl(maximumTreeLevel, currentLevel + 1, callback, scope->getSubScopes()))
        {
            return false;
        }
    }

    return true;
}

bool ScopeManager::isTopScope() { return m_currentScope && m_currentScope->getParent(); }

bool ScopeManager::searchByIdImpl(size_t id,
                                  size_t currentLevel,
                                  size_t maximumLevel,
                                  const std::map<size_t, std::shared_ptr<Scope>> &scopes,
                                  std::shared_ptr<Scope> *scope)
{
    if (currentLevel >= maximumLevel)
        return false;

    if (m_scopes.contains(id))
        return true;

    for (auto &[currentScopeId, currentScope] : m_scopes)
    {
        if (searchByIdImpl(id, currentLevel + 1, maximumLevel, currentScope->getSubScopes(), scope))
            return true;
    }

    return false;
}

size_t ScopeManager::getCurrentScopeId() const
{
    if (!m_currentScope)
        return 0;

    return m_currentScope->getId();
}

size_t ScopeManager::getScopeCount() const { return m_scopes.size(); }

size_t ScopeManager::getSubScopeCount() const { return m_currentScope->getSubScopes().size(); }

void ScopeManager::beginScope()
{
    if (!m_currentScope)
    {
        m_currentScope = std::make_shared<Scope>(0xDEADBEEF,
                                                 std::map<size_t, std::shared_ptr<Scope>>{},
                                                 std::map<size_t, std::shared_ptr<Symbol>>{},
                                                 nullptr);

        m_scopes.insert({ m_currentScope->getId(), m_currentScope });
        return;
    }

    uint64_t seed = m_currentScope->getParent() ? m_currentScope->getParent()->getId() : 0x7A3F29C1B8E4D52A;
    seed *= 6364136223846793005ULL;

    uint64_t id = ((seed >> 22u) ^ seed) * 0x2557340b9cf3abdeULL;
    id = (((id >> 28u) ^ id) * 0x2557340b9cf3abdeULL) >> 22u;

    std::shared_ptr<Scope> newScope = std::make_shared<Scope>(id,
                                                              std::map<size_t, std::shared_ptr<Scope>>{},
                                                              std::map<size_t, std::shared_ptr<Symbol>>{},
                                                              m_currentScope);
    m_currentScope->appendSubScope(newScope);
    m_currentScope = newScope;
    m_scopes.insert({ newScope->getId(), std::move(newScope) });
}

void ScopeManager::endScope()
{
    if (m_currentScope && m_currentScope->getParent())
    {
        m_currentScope = m_currentScope->getParent();
    }
}

void ScopeManager::forEach(size_t minimumTreeLevel, size_t maximumTreeLevel, const ScopeForEachCallbackT &callback)
{
    forEachImpl(maximumTreeLevel, minimumTreeLevel, callback, m_scopes);
}

const std::shared_ptr<Scope> &ScopeManager::getCurrentScope() const { return m_currentScope; }
