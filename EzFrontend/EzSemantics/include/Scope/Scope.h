#ifndef EZPACKER_SCOPE_H
#define EZPACKER_SCOPE_H

#include "EzSemanticsCommon.h"
#include "Symbol.h"

/**
 * Represents a scope, which is a batch of defined symbols in the AST.
 */
class Scope
{
  public:
    /**
     * Creates the scope with the given id, subScopes, symbols and parent.
     * @param parent
     * @param symbols
     * @param name
     */
    Scope(Scope *parent, const std::map<std::string_view, Symbol *> &symbols, const std::string_view &name);

    /**
     * Creates an empty scope with its id and parent.
     * @param parent
     * @param name
     */
    Scope(Scope *parent, const std::string_view &name);

    /**
     * Tries to define a symbol in the current scope. If symbol is already defined, an error will be thrown.
     * @param symbol
     * @param name
     * @return bool
     */
    bool define(Symbol *symbol, const std::string_view &name);

    /**
     * Tries to resolve the given symbol by its name. If outSymbol is not nullptr and if there's a match, outSymbol will
     * be set to this occurrence. If the symbol does exist in this scope, true is returned.
     *
     * If searchParent is true, if the given name does not exist in this scope and m_parent is valid, m_parent->resolve
     * will be used with the same parameters given to this method.
     * @param name
     * @param outSymbol
     * @param searchParent
     * @return bool
     */
    bool resolve(const std::string_view &name, Symbol **outSymbol, bool searchParent);

    /**
     * Returns the parent of this scope.
     * @return Scope*
     */
    Scope *getParent() const;

  private:
    Scope *m_parent;
    std::string_view m_name;
    std::map<std::string_view, Symbol *> m_symbols;
};

#endif // EZPACKER_SCOPE_H
