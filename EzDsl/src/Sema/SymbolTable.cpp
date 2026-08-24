#include "Sema/Scope.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"

SymbolTable::SymbolTable(std::pmr::memory_resource *alloc) :
    m_currentScopeId(InvalidScopeId), m_scopes(alloc), m_symbols(alloc), m_alloc(alloc)
{
}

ScopeId SymbolTable::getCurrentScopeId() const { return m_currentScopeId; }

ScopeId SymbolTable::createScope(ScopeId parentId, const std::string_view &debugName)
{
    std::pmr::polymorphic_allocator<> alloc(m_alloc);
    Scope *scope = alloc.new_object<Scope>(m_scopes.size(), m_currentScopeId, debugName, m_alloc);

    m_scopes.push_back(scope);
    return scope->getId();
}

Scope *SymbolTable::getScopeById(ScopeId id) const
{
    if (id >= m_scopes.size())
        return nullptr;

    return m_scopes[id];
}

Symbol *SymbolTable::getSymById(SymbolId id) const
{
    if (id >= m_symbols.size())
        return nullptr;

    return m_symbols[id];
}

Symbol *SymbolTable::getSymByName(const std::string_view &name, std::optional<ScopeId> startingScope)
{
    ScopeId cursor = startingScope.value_or(m_currentScopeId);

    while (cursor != InvalidScopeId)
    {
        Scope *scope = m_scopes[cursor];

        if (Symbol *sym = getSymInScope(cursor, name); sym)
            return sym;

        cursor = scope->getParentId();
    }

    return nullptr;
}

SymbolId SymbolTable::declareSym(class SourceReference *sourceRef,
                                 SymbolFlags flags,
                                 SymbolType type,
                                 Symbol::SymbolData data,
                                 std::string_view name)
{
    if (getSymByName(name) != nullptr)
    {
        return InvalidSymbolId;
    }

    std::pmr::polymorphic_allocator<> alloc(m_alloc);
    Symbol *symbol =
            alloc.new_object<Symbol>(sourceRef, flags, m_currentScopeId, m_symbols.size(), type, std::move(name));
    symbol->setData(std::move(data));

    m_symbols.push_back(symbol);
    return symbol->getId();
}

void SymbolTable::enterScope(std::string_view debugName)
{
    m_currentScopeId = createScope(m_currentScopeId, debugName);
}

void SymbolTable::exitScope()
{
    Scope *scope = m_scopes[m_currentScopeId];
    ScopeId parentId = scope->getParentId();

    if (parentId != InvalidScopeId)
        m_currentScopeId = parentId;
}

const std::pmr::vector<Symbol *> &SymbolTable::getSymbols() const { return m_symbols; }

Symbol *SymbolTable::getSymInScope(ScopeId id, const std::string_view &name) const
{
    Scope *scope = m_scopes[id];
    const auto &symbolIds = scope->getSymbols();

    for (auto &symId : symbolIds)
    {
        Symbol *sym = m_symbols[symId];
        if (sym->getName() == name)
            return sym;
    }

    return nullptr;
}