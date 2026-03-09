/**
 * @file Scope.h
 * @brief Lexical scope used by EzSemantics name resolution.
 *
 * A `Scope` owns the symbol table for one lexical region and points to its
 * parent scope. `BasicSemanticContext` creates and stores these scopes during
 * symbol definition, and later passes re-enter them using annotations stored
 * in the AST.
 *
 * Lookup semantics:
 *   - `define()` inserts only in the current scope.
 *   - `resolve(..., false)` queries only the current scope.
 *   - `resolve(..., true)` walks the parent chain until a match is found.
 *
 * Duplicate names in the same scope are rejected. Reusing a name from a
 * parent scope is allowed and implements lexical shadowing.
 */
#ifndef EZPACKER_SCOPE_H
#define EZPACKER_SCOPE_H

#include "EzSemanticsCommon.h"
#include "Symbol.h"

/**
 * Symbol table for one lexical scope.
 */
class Scope
{
  public:
    /**
     * Creates a scope with an existing symbol map.
     *
     * This overload is mainly useful when a caller already has a prepared
     * symbol table and wants to attach it to a parent scope.
     */
    Scope(Scope *parent, const std::map<std::string_view, Symbol *> &symbols, const std::string_view &name);

    /**
     * Creates an empty scope with the given parent and descriptive name.
     */
    Scope(Scope *parent, const std::string_view &name);

    /**
     * Inserts a symbol into this scope only.
     *
     * @return `false` if another symbol with the same name already exists in
     *         this scope.
     */
    bool define(Symbol *symbol, const std::string_view &name);

    /**
     * Merges another symbol map into this scope.
     *
     * If a name collision is found, no special recovery is performed: the
     * method returns `false` and optionally reports the conflicting incoming
     * symbol through `outErrSym`.
     */
    bool mergeSymbols(const std::map<std::string_view, Symbol *> &symbols, Symbol **outErrSym);

    /**
     * Resolves a symbol name in this scope, optionally walking parent scopes.
     *
     * @param name Name to resolve.
     * @param outSymbol Optional output receiving the matching symbol.
     * @param searchParent Whether lookup may continue in parent scopes.
     * @return `true` if the symbol is found.
     */
    bool resolve(const std::string_view &name, Symbol **outSymbol, bool searchParent);

    /**
     * Returns the lexical parent of this scope, or `nullptr` for the global
     * scope.
     */
    Scope *getParent() const;

    /**
     * Replaces the parent scope pointer.
     */
    void setParent(Scope *parent);

    /**
     * Returns the symbols defined directly in this scope.
     *
     * Parent-scope symbols are not included.
     */
    const std::map<std::string_view, Symbol *> &getSymbols() const;

  private:
    Scope *m_parent;
    std::string_view m_name;
    std::map<std::string_view, Symbol *> m_symbols;
};

#endif // EZPACKER_SCOPE_H
