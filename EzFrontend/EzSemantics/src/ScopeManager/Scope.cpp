#include "ScopeManager/Scope.h"

Scope::Scope(size_t id,
             std::map<size_t, std::shared_ptr<Scope>> subScopes,
             std::map<size_t, std::shared_ptr<Symbol>> symbols,
             std::shared_ptr<Scope> parent) :
    m_id(id), m_subScopes(std::move(subScopes)), m_symbols(std::move(symbols)), m_parent(std::move(parent))
{
}

size_t Scope::getId() const { return m_id; }

bool Scope::searchSymbolById(size_t id, std::shared_ptr<Symbol> *sym)
{
    bool exists = m_symbols.contains(id);
    if (sym && exists)
    {
        *sym = m_symbols.at(id);
        return true;
    }

    return exists;
}

bool Scope::searchSymbolByName(const std::string &name, std::shared_ptr<Symbol> *sym)
{
    for (auto &[symbolId, symbol] : m_symbols)
    {
        if (name == symbol->getName())
        {
            if (sym)
                *sym = symbol;

            return true;
        }
    }
    return false;
}

void Scope::appendSubScope(std::shared_ptr<Scope> scope) { m_subScopes.emplace(scope->getId(), std::move(scope)); }

void Scope::appendSymbol(std::shared_ptr<Symbol> symbol) { m_symbols.emplace(symbol->getId(), std::move(symbol)); }

const std::map<size_t, std::shared_ptr<Scope>> &Scope::getSubScopes() const { return m_subScopes; }

const std::map<size_t, std::shared_ptr<Symbol>> &Scope::getSymbols() const { return m_symbols; }

const std::shared_ptr<Scope> &Scope::getParent() const { return m_parent; }
