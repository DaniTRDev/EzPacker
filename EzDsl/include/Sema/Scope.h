#ifndef EZDSL_SCOPE_H
#define EZDSL_SCOPE_H

#include "EzDslCommon.h"

using ScopeId = size_t;
inline constexpr ScopeId InvalidScopeId = UINT64_MAX;

class Scope
{
  public:
    /**
     * Creates the scope with the given ID, parent ID and debug name.
     */
    Scope(ScopeId id, ScopeId parentId, std::string_view debugName, std::pmr::memory_resource *alloc);

    /**
     * Returns the ID of the scope.
     */
    ScopeId getId() const;

    /**
     * Returns the ID of the parent scope. If it's InvalidScope, this is the top scope.
     */
    ScopeId getParentId() const;

    /**
     * Adds a symbol to the symbol list.
     */
    void addSymbol(ScopeId symbolId);

    /**
     * Returns a vector with the IDs of the symbols defined in this scope.
     */
    const std::pmr::vector<ScopeId> &getSymbols() const;

    /**
     * Returns the debug name of the scope.
     */
    const std::string_view &getDebugName() const;

  private:
    ScopeId m_id;
    ScopeId m_parentId;
    std::pmr::vector<ScopeId> m_symbols; // Same data type as SymbolId, but we can't use it here....
    std::string_view m_debugName;
};

#endif // EZDSL_SCOPE_H