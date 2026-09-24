#include "Sema/Scope.h"

Scope::Scope(ScopeId id, ScopeId parentId, std::string_view debugName, std::pmr::memory_resource *alloc) :
    m_id(id), m_parentId(parentId), m_symbolMap(alloc), m_debugName(debugName)
{
}

ScopeId Scope::getId() const { return m_id; }

ScopeId Scope::getParentId() const { return m_parentId; }

void Scope::addSymbol(std::string_view name, ScopeId symbolId)
{
    // Record the symbol in the name-to-id multimap used for scope-local lookup.
    m_symbolMap.emplace(name, symbolId);
}

const std::string_view &Scope::getDebugName() const { return m_debugName; }