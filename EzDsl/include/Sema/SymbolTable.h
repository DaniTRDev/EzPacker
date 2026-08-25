#ifndef EZDSL_SYMBOL_TABLE_H
#define EZDSL_SYMBOL_TABLE_H

#include "EzDslCommon.h"
#include "Scope.h"
#include "Symbol.h"

class SymbolTable
{
  public:
    /**
     * Builds the symbol table with the given pmr resource.
     */
    SymbolTable(std::pmr::memory_resource *alloc);

    /**
     * Returns the ID of the current scope.
     */
    ScopeId getCurrentScopeId() const;

    /**
     * Creates a scope with the given parent and debug name.
     */
    ScopeId createScope(ScopeId parentId, const std::string_view &debugName);

    /**
     * Returns the scope at the given index. If no scope exists, nullptr is returned.
     */
    Scope *getScopeById(ScopeId id) const;

    /**
     * Returns the symbol at the given index. If no symbol exists, nullptr is returned.
     */
    Symbol *getSymById(SymbolId id) const;

    /**
     * Starts a bottom-up search for the given scope name. The first scope checked is starting scope, if it's nullopt,
     * the current scope is the starting point.
     */
    Symbol *getSymByName(const std::string_view &name, std::optional<ScopeId> startingScope = std::nullopt);

    /**
     * Declares a symbol in the current scope with the given source reference, flags, type, data and name.
     * If the symbol already exists in the current scope, InvalidSymbolId is returned.
     */
    SymbolId declareSym(class SourceReference *sourceRef,
                        SymbolFlags flags,
                        SymbolType type,
                        Symbol::SymbolData data,
                        std::string_view name);

    /**
     * Creates and enters a scope with the given debug name. It internally calls createScope with the appropiate
     * parameters.
     */
    void enterScope(std::string_view debugName = "");

    /**
     * Exits the current scope and sets m_current scope to the parent. If the current scope is the root scope, nothing
     * is done.
     */
    void exitScope();

    /**
     * Returns the allocator of the symbol table.
     */
    std::pmr::memory_resource *getAllocator();

    /**
     * Returns the list of symbols.
     */
    const std::pmr::vector<Symbol *> &getSymbols() const;

  private:
    /**
     * Returns the symbol if the the target scope has defined it. Returns nullptr if not.
     */
    Symbol *getSymInScope(ScopeId id, const std::string_view &name) const;

  private:
    ScopeId m_currentScopeId;
    std::pmr::memory_resource *m_alloc;
    std::pmr::vector<Scope *> m_scopes;
    std::pmr::vector<Symbol *> m_symbols;
};

#endif // EZDSL_SYMBOL_TABLE_H