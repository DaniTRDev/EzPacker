#ifndef EZDSL_SCOPE_H
#define EZDSL_SCOPE_H

#include "EzDslCommon.h"
#include <string_view>
#include <unordered_map>
#include <vector>

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
     * Finds a symbol ID in this scope by name in O(1). Returns InvalidScopeId if not found.
     */
    ScopeId findSymbol(std::string_view name) const;

    /**
     * Adds a symbol to the symbol list and lookup map.
     */
    void addSymbol(std::string_view name, ScopeId symbolId);

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
    std::pmr::vector<ScopeId> m_symbols;
    std::pmr::unordered_map<std::string_view, ScopeId> m_symbolMap;
    std::string_view m_debugName;
};

#endif // EZDSL_SCOPE_H