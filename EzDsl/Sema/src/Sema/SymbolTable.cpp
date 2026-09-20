#include "Sema/Scope.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"

SymbolTable::SymbolTable(std::pmr::memory_resource *alloc) :
    m_currentScopeId(InvalidScopeId), m_scopes(alloc), m_symbols(alloc), m_alloc(alloc)
{
    // Enters the global scope at the moment of creating the table.
    enterScope("global");
}

ScopeId SymbolTable::createScope(ScopeId parentId, const std::string_view &debugName)
{
    // Allocate the scope in the arena; its id is its index in the scope list.
    std::pmr::polymorphic_allocator<> alloc(m_alloc);
    Scope *scope = alloc.new_object<Scope>(m_scopes.size(), parentId, debugName, m_alloc);

    m_scopes.push_back(scope);
    return scope->getId();
}

Symbol *SymbolTable::getSymById(SymbolId id) const
{
    if (id >= m_symbols.size())
        return nullptr;

    return m_symbols[id];
}

Symbol *SymbolTable::getSymByName(const std::string_view &name, std::optional<ScopeId> startingScope)
{
    return getSymByNameImpl(name, std::nullopt, startingScope);
}

Symbol *SymbolTable::getSymByName(const std::string_view &name, SymbolType type, std::optional<ScopeId> startingScope)
{
    return getSymByNameImpl(name, type, startingScope);
}

Symbol *
SymbolTable::getSymByNameImpl(const std::string_view &name, std::optional<SymbolType> type, std::optional<ScopeId> startingScope)
{
    // Same bottom-up lookup for both overloads, optionally matching only the requested symbol type.
    ScopeId cursor = startingScope.value_or(m_currentScopeId);

    while (cursor != InvalidScopeId)
    {
        Scope *scope = m_scopes[cursor];

        if (Symbol *sym = getSymInScope(cursor, name, type); sym)
            return sym;

        cursor = scope->getParentId();
    }

    return nullptr;
}

SymbolId SymbolTable::declareSym(class SourceReference *sourceRef,
                                 SymbolType type,
                                 Symbol::SymbolData data,
                                 std::string_view name)
{
    // Check for duplicate symbols of the same kind within the current scope
    bool duplicate = false;
    if (type == SymbolType::Type || type == SymbolType::TypeSet)
    {
        duplicate = (getSymInScope(m_currentScopeId, name, SymbolType::Type) != nullptr) ||
                (getSymInScope(m_currentScopeId, name, SymbolType::TypeSet) != nullptr);
    }
    else
    {
        duplicate = (getSymInScope(m_currentScopeId, name, type) != nullptr);
    }

    if (duplicate)
    {
        return InvalidSymbolId;
    }

    std::pmr::polymorphic_allocator<> alloc(m_alloc);
    Symbol *symbol = alloc.new_object<Symbol>(sourceRef, m_currentScopeId, m_symbols.size(), type, name);
    symbol->setData(std::move(data));

    SymbolId symId = symbol->getId();

    m_symbols.push_back(symbol);
    m_scopes[m_currentScopeId]->addSymbol(name, symId);

    return symId;
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

std::pmr::memory_resource *SymbolTable::getAllocator() { return m_alloc; }

const std::pmr::vector<Symbol *> &SymbolTable::getSymbols() const { return m_symbols; }

Symbol *SymbolTable::getSymInScope(ScopeId id, const std::string_view &name, std::optional<SymbolType> type) const
{
    if (id >= m_scopes.size())
        return nullptr;

    Scope *scope = m_scopes[id];
    if (!scope)
        return nullptr;

    // Iterate every declaration sharing the name and return the first matching the optional type.
    auto [begin, end] = scope->findSymbols(name);
    for (auto it = begin; it != end; ++it)
    {
        ScopeId symId = it->second;
        if (symId < m_symbols.size())
        {
            Symbol *sym = m_symbols[symId];
            if (!type.has_value() || sym->getType() == *type)
            {
                return sym;
            }
        }
    }

    return nullptr;
}