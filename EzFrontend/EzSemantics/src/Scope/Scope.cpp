#include "Scope/Scope.h"

Scope::Scope(Scope *parent, const std::map<std::string_view, Symbol *> &symbols, const std::string_view &name) :
    m_parent(parent), m_symbols(symbols), m_name(name)
{
}

Scope::Scope(Scope *parent, const std::string_view &name) : Scope(parent, {}, name) {}

bool Scope::define(Symbol *symbol, const std::string_view &name)
{
    if (m_symbols.contains(name))
        return false;

    m_symbols.insert({ name, symbol });
    return true;
}

bool Scope::mergeSymbols(const std::map<std::string_view, Symbol *> &symbols, Symbol **outErrSym)
{
    for (auto &sym : symbols)
    {
        if (m_symbols.contains(sym.first))
        {
            if (outErrSym)
            {
                *outErrSym = sym.second;
            }
            return false;
        }

        m_symbols[sym.first] = sym.second;
    }
    return true;
}

bool Scope::resolve(const std::string_view &name, Symbol **outSymbol, bool searchParent)
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

Scope *Scope::getParent() const { return m_parent; }

void Scope::setParent(Scope *parent) { m_parent = parent; }

const std::map<std::string_view, Symbol *> &Scope::getSymbols() const { return m_symbols; }
