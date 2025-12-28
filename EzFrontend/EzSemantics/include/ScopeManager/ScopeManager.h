#ifndef EZPACKER_SCOPEMANAGER_H
#define EZPACKER_SCOPEMANAGER_H

#include "EzSemanticsCommon.h"
#include "Scope.h"

/**
 * This class acts as a wrapper for common scope operations.
 */
class ScopeManager
{
  public:
    
    
    static constexpr size_t MaxScopeLevel = UINT64_MAX;
    
    /**
     * If this callback returns true, iterations will continue. If it returns false, iterations will stop.
     */
    using ScopeForEachCallbackT = std::function<bool(const std::shared_ptr<Scope> &scope, size_t scopeTreeLevel)>;

    /**
     * Creates the ScopeManager with the given scopes
     * @param scopes
     */
    ScopeManager(std::map<size_t, std::shared_ptr<Scope>> scopes);

    /**
     * Creates the given symbol, if it didn't exist before in the current scope. If no scope was began, begin will be
     * called. If the symbol already existed, false is returned.
     *
     * If symbol existed, or it was created, sym will contain a pointer to the symbol.
     * @param symbolType
     * @param name
     * @param symbolDataType
     * @return bool
     */
    bool createSymbolAtCurrentScope(SymbolType symbolType,
                                    std::string name,
                                    std::string symbolDataType,
                                    std::shared_ptr<Symbol> *sym);

    /**
     * Returns true if current scope is the top scope. Its parent is nullptr.
     * @return bool
     */
    bool isTopScope();

    /**
     * Traverses the given tree range and searches for an ID. Returns true if found.
     * @param id
     * @param minimumLevel
     * @param maximumLevel
     * @param scope
     * @return bool
     */
    bool searchById(size_t id, size_t minimumLevel, size_t maximumLevel, std::shared_ptr<Scope> *scope);

    /**
     * Traverses the given tree range and searches for a symbol ID. Returns true if found.
     * @param id
     * @param minimumLevel
     * @param maximumLevel
     * @param sym
     * @return bool
     */
    bool searchSymbolById(size_t id, size_t minimumLevel, size_t maximumLevel, std::shared_ptr<Symbol> *sym);

    /**
     * Traverses the given tree range and searches for a symbol name and returns true if found. Please, use
     * searchSymbolById when possible for better performance. This function is rather expensive.
     * @param name
     * @param minimumLevel
     * @param maximumLevel
     * @param sym
     * @return bool
     */
    bool
    searchSymbolByName(const std::string &name, size_t minimumLevel, size_t maximumLevel, std::shared_ptr<Symbol> *sym);

    /**
     * Returns the ID of the current scope. If there's no current scope, 0 is returned.
     * @return size_t
     */
    size_t getCurrentScopeId() const;

    /**
     * Returns the number of scopes defined.
     * @return size_t
     */
    size_t getScopeCount() const;

    /**
     * Returns the number of scopes stored inside the current one.
     * @return size_t
     */
    size_t getSubScopeCount() const;

    /**
     * Begins a new scope. If there's no scope to enter, a new one will be created and manager will enter into it.
     */
    void beginScope();

    /**
     * Ends the current scope. It does nothing if current scope is the top.
     */
    void endScope();

    /**
     * Traverses the tree from minimumTreeLevel to maximumTreeLevel. If callback is called and it returns false,
     * traversing will stop.
     * @param minimumTreeLevel
     * @param maximumTreeLevel
     * @param callback
     */
    void forEach(size_t minimumTreeLevel, size_t maximumTreeLevel, const ScopeForEachCallbackT &callback);

    /**
     * Returns the current scope.
     * @return const std::shared_ptr<Scope> &
     */
    const std::shared_ptr<Scope> &getCurrentScope() const;
    
  private:
    /**
     * Traverses the scope tree until currentLevel >= maximumTreeLevel or callback returns false.
     * @param maximumTreeLevel
     * @param currentLevel
     * @param callback
     * @param scopes
     * @return bool
     */
    bool forEachImpl(size_t maximumTreeLevel,
                     size_t currentLevel,
                     const ScopeForEachCallbackT &callback,
                     const std::map<size_t, std::shared_ptr<Scope>> &scopes);

    /**
     * Traverses the the tree until currentLevel >= maximumLevel.
     * @param id
     * @param currentLevel
     * @param maximumLevel
     * @param scopes
     * @param scope
     * @return bool
     */
    bool searchByIdImpl(size_t id,
                        size_t currentLevel,
                        size_t maximumLevel,
                        const std::map<size_t, std::shared_ptr<Scope>> &scopes,
                        std::shared_ptr<Scope> *scope);

  private:
    std::shared_ptr<Scope> m_currentScope;
    std::map<size_t, std::shared_ptr<Scope>> m_scopes;
};

#endif // EZPACKER_SCOPEMANAGER_H
