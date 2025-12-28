#ifndef EZPACKER_SCOPE_H
#define EZPACKER_SCOPE_H

#include "EzSemanticsCommon.h"
#include "SymbolResolverVisitor/Symbol.h"

/**
 * Represents a scope, which is a batch of important data that should be preserved in certain locations for the
 * compilation.
 */
class Scope
{
  public:
    /**
     * Creates the scope with the given id, subScopes, symbols and parent.
     * @param id
     * @param subScopes
     * @param symbols
     * @param parent
     */
    Scope(size_t id,
          std::map<size_t, std::shared_ptr<Scope>> subScopes,
          std::map<size_t, std::shared_ptr<Symbol>> symbols,
          std::shared_ptr<Scope> parent);

    /**
     * If given symbol id exists in this scope, if sym is not nullptr, it's set to its Symbol pointer and true is
     * returned.
     * @param id
     * @param sym
     * @return bool
     */
    bool searchSymbolById(size_t id, std::shared_ptr<Symbol> *sym = nullptr);

    /**
     * If given symbol name exists in this scope, if sym is not nullptr, it's set to its Symbol pointer and true is
     * returned. This type of search is slow, please use searchSymbolById for notably more performance.
     * @param name
     * @return bool
     */
    bool searchSymbolByName(const std::string &name, std::shared_ptr<Symbol> *sym = nullptr);

    /**
     * Returns the ID of this scope.
     * @return size_t
     */
    size_t getId() const;

    /**
     * Appends the given subscope to the list, if it's valid.
     * @param scope
     */
    void appendSubScope(std::shared_ptr<Scope> scope);

    /**
     * Appends the given symbol to the list, if it's valid.
     * @param symbol
     */
    void appendSymbol(std::shared_ptr<Symbol> symbol);

    /**
     * Returns the sub scopes defined inside this.
     * @return const std::map<size_t, std::shared_ptr<Scope>> &
     */
    const std::map<size_t, std::shared_ptr<Scope>> &getSubScopes() const;

    /**
     * Returns the symbols defined in this scope.
     * @return const std::map<size_t, std::shared_ptr<Symbol>> &
     */
    const std::map<size_t, std::shared_ptr<Symbol>> &getSymbols() const;

    /**
     * Returns the parent of this scope.
     * @return std::shared_ptr<Scope>
     */
    const std::shared_ptr<Scope> &getParent() const;

  private:
    size_t m_id;
    std::map<size_t, std::shared_ptr<Scope>> m_subScopes;
    std::map<size_t, std::shared_ptr<Symbol>> m_symbols;
    std::shared_ptr<Scope> m_parent;
};

#endif // EZPACKER_SCOPE_H
