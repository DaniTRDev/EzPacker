#ifndef EZDSL_SYMBOL_TABLE_H
#define EZDSL_SYMBOL_TABLE_H

#include "EzDslCommon.h"
#include "Scope.h"
#include "Symbol.h"

/**
 * Central repository managing hierarchical lexical/semantic scopes and Symbol instances.
 * Allocates scopes and symbols using a PMR arena resource.
 */
class SymbolTable
{
  public:
    /**
     * Constructs a symbol table with the given PMR memory resource and initializes the root global scope.
     */
    SymbolTable(std::pmr::memory_resource *alloc);

    /**
     * Returns the numeric ID of the active scope.
     */
    ScopeId getCurrentScopeId() const;

    /**
     * Creates a new scope as a child of parentId with the specified debug name.
     */
    ScopeId createScope(ScopeId parentId, const std::string_view &debugName);

    /**
     * Returns a pointer to the Scope with the specified ID, or nullptr if out of bounds.
     */
    Scope *getScopeById(ScopeId id) const;

    /**
     * Returns a pointer to the Symbol with the specified SymbolId, or nullptr if out of bounds.
     */
    Symbol *getSymById(SymbolId id) const;

    /**
     * Performs a bottom-up lexical lookup for a symbol name starting at startingScope (defaults to current scope).
     * Climbs the parent scope chain until found or the root scope is exceeded.
     */
    Symbol *getSymByName(const std::string_view &name, std::optional<ScopeId> startingScope = std::nullopt);

    /**
     * Declares a new symbol in the active scope with source reference, flags, type, semantic payload, and name.
     * Returns InvalidSymbolId if a symbol with the same name already exists in the active scope.
     */
    SymbolId declareSym(class SourceReference *sourceRef,
                        SymbolFlags flags,
                        SymbolType type,
                        Symbol::SymbolData data,
                        std::string_view name);

    /**
     * Creates a new child scope under the active scope and makes it the active scope.
     */
    void enterScope(std::string_view debugName = "");

    /**
     * Exits the current scope and restores its parent as the active scope.
     */
    void exitScope();

    /**
     * Returns the underlying PMR memory resource.
     */
    std::pmr::memory_resource *getAllocator();

    /**
     * Returns the complete list of all allocated symbols.
     */
    const std::pmr::vector<Symbol *> &getSymbols() const;

  private:
    /**
     * Looks up a symbol name within a single specific scope without ascending to parents.
     */
    Symbol *getSymInScope(ScopeId id, const std::string_view &name) const;

  private:
    ScopeId m_currentScopeId;
    std::pmr::memory_resource *m_alloc;
    std::pmr::vector<Scope *> m_scopes;
    std::pmr::vector<Symbol *> m_symbols;
};

#endif // EZDSL_SYMBOL_TABLE_H