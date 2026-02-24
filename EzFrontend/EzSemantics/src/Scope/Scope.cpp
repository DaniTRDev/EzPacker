#include "Scope/Scope.h"

Scope::Scope(const std::map<std::string, std::shared_ptr<Symbol>> &symbols,
             const std::shared_ptr<Scope> &parent,
             const std::string &name) : m_symbols(symbols), m_parent(std::move(parent)), m_name(name)
{
}

Scope::Scope(const std::shared_ptr<Scope> &parent, const std::string &name) : Scope({}, parent, name) {}

bool Scope::define(const std::string &name, const std::shared_ptr<Symbol> &symbol)
{
    if (m_symbols.contains(name))
        return false;

    m_symbols.insert({ name, std::move(symbol) });
    return true;
}

bool Scope::resolve(const std::string &name, std::shared_ptr<Symbol> *outSymbol, bool searchParent)
{
    auto it = m_symbols.find(name);
    if (it == m_symbols.end())
    {
        if (searchParent && m_parent)
        {
            return m_parent->resolve(name, outSymbol, searchParent);
        }

        return false;
    }

    if (outSymbol)
    {
        *outSymbol = it->second;
    }
    return true;
}

const std::shared_ptr<Scope> &Scope::getParent() const { return m_parent; }
