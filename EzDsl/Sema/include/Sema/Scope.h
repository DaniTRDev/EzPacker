#ifndef EZDSLSEMA_SCOPE_H
#define EZDSLSEMA_SCOPE_H

#include "EzDslSemaCommon.h"

#include <unordered_map>

using ScopeId = size_t;
inline constexpr ScopeId InvalidScopeId = UINT64_MAX;

/**
 * Lexical and semantic scope tracking locally declared symbols in an arena-backed hash table.
 * Supports hierarchical tree traversal to parent scopes.
 */
class Scope
{
  public:
    /**
     * Constructs a scope with its unique ID, parent scope ID, debug label, and PMR allocator.
     */
    Scope(ScopeId id, ScopeId parentId, std::string_view debugName, std::pmr::memory_resource *alloc);

    /**
     * Returns the unique numeric ID of this scope.
     */
    ScopeId getId() const;

    /**
     * Returns the parent scope ID, or InvalidScopeId if this is the root global scope.
     */
    ScopeId getParentId() const;

    /**
     * Finds a locally declared symbol ID by name. Returns InvalidScopeId if not found in this scope.
     */
    ScopeId findSymbol(std::string_view name) const;

    /**
     * Finds all locally declared symbol IDs by name.
     */
    auto findSymbols(std::string_view name) const
    {
        return m_symbolMap.equal_range(name);
    }

    /**
     * Registers a symbol name and its SymbolId into this scope's lookup table and ordered list.
     */
    void addSymbol(std::string_view name, ScopeId symbolId);

    /**
     * Returns the ordered list of symbol IDs declared directly in this scope.
     */
    const std::pmr::vector<ScopeId> &getSymbols() const;

    /**
     * Returns the human-readable debug name of this scope.
     */
    const std::string_view &getDebugName() const;

  private:
    ScopeId m_id;
    ScopeId m_parentId;
    std::pmr::vector<ScopeId> m_symbols;
    std::pmr::unordered_multimap<std::string_view, ScopeId> m_symbolMap;
    std::string_view m_debugName;
};

#endif // EZDSLSEMA_SCOPE_H