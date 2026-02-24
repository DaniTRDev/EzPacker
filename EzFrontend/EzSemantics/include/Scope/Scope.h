#ifndef EZPACKER_SCOPE_H
#define EZPACKER_SCOPE_H

#include "EzSemanticsCommon.h"
#include "Symbol.h"

/**
 * Represents a scope, which is a batch of important data that should be preserved in certain locations for the
 * compilation.
 */
class Scope
{
  public:
    /**
     * Creates the scope with the given id, subScopes, symbols and parent.
     * @param symbols
     * @param parent
     * @param name
     */
    Scope(const std::map<std::string, std::shared_ptr<Symbol>> &symbols,
          const std::shared_ptr<Scope> &parent,
          const std::string &name);

    /**
     * Creates an empty scope with its id and parent.
     * @param parent
     * @param name
     */
    Scope(const std::shared_ptr<Scope> &parent, const std::string &name);

    /**
     * Tries to define a symbol in the current scope. If symbol is already defined, an error will be thrown.
     * @param name
     * @param symbol
     * @return
     */
    bool define(const std::string &name, const std::shared_ptr<Symbol> &symbol);

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
    bool resolve(const std::string &name, std::shared_ptr<Symbol> *outSymbol, bool searchParent);

    /**
     * Returns the parent of this scope.
     * @return const std::shared_ptr<Scope> &
     */
    const std::shared_ptr<Scope> &getParent() const;

  private:
    std::string m_name;
    std::map<std::string, std::shared_ptr<Symbol>> m_symbols;
    std::shared_ptr<Scope> m_parent;
};

#endif // EZPACKER_SCOPE_H
