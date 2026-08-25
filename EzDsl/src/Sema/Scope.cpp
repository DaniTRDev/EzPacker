#include "Sema/Scope.h"

Scope::Scope(ScopeId id, ScopeId parentId, std::string_view debugName, std::pmr::memory_resource *alloc) :
    m_id(id), m_parentId(parentId), m_symbols(alloc), m_symbolMap(alloc), m_debugName(debugName)
{
}

ScopeId Scope::getId() const { return m_id; }

ScopeId Scope::getParentId() const { return m_parentId; }

ScopeId Scope::findSymbol(std::string_view name) const
{
    auto it = m_symbolMap.find(name);
    if (it != m_symbolMap.end())
    {
        return it->second;
    }
    return InvalidScopeId;
}

void Scope::addSymbol(std::string_view name, ScopeId symbolId)
{
    m_symbols.push_back(symbolId);
    m_symbolMap.emplace(name, symbolId);
}

const std::pmr::vector<ScopeId> &Scope::getSymbols() const { return m_symbols; }

const std::string_view &Scope::getDebugName() const { return m_debugName; }