#ifndef EZPACKER_BASICSEMANTICCONTEXT_H
#define EZPACKER_BASICSEMANTICCONTEXT_H

#include "EzSemanticsCommon.h"
#include "Scope/Scope.h"

/**
 * Context used by the semantic analyzer.
 *
 * IMPORTANT: By default global scope is created on object creation. It is available by getCurrentScope (if no
 * more scopes were begin) or through getGlobalScope.
 */
class BasicSemanticContext : public ErrorEmitter
{
  public:
    /**
     * Creates the context with the given error collector and source manager.
     * @param errorCollector
     * @param sourceManager
     */
    BasicSemanticContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                         const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Creates a symbol in the current scope linked to an AstNode. If symbol is present in the scope false is returned
     * and nothing is done. If symbol was correctly created, outSymbol will be set to it.
     * @param symbolType
     * @param definingNode
     * @param outSymbol
     * @param symbolDataType
     * @param symbolName
     * @return bool
     */
    bool createSymbol(SymbolType symbolType,
                      const std::shared_ptr<AstNode> &definingNode,
                      std::shared_ptr<Symbol> *outSymbol,
                      const std::shared_ptr<Type> &symbolDataType,
                      const std::string &symbolName);
    /**
     * Returns true if the current scope is the global scope.
     * @return bool
     */
    bool isCurrentScopeGlobalScope() const;

    /**
     * Tries to search for a symbol in the current scope. If it's found, true is returned. If it's found and
     * outSymbol != nullptr, outSymbol will be set to the occurrence.
     *
     * If the symbol is not found in this scope and searchParent is set to true, it will start a recursive bottom-to-top
     * search in parent scopes.
     * @param symbolName
     * @param outSymbol
     * @param searchParent
     * @return bool
     */
    bool resolveSymbolInScope(const std::string &symbolName, std::shared_ptr<Symbol> *outSymbol, bool searchParent);

    /**
     * Begins a new scope with the given name. Scope names CAN BE duplicated.
     * @param name
     */
    void beginScope(const std::string &name);

    /**
     * Emits an error because of the redefinition of a symbol. It will print the first place the symbol was defined in.
     * @param module
     * @param symbolName
     * @param errorNode
     */
    void emitSymbolRedefinitionError(const std::string &module,
                                     const std::string &symbolName,
                                     const std::shared_ptr<AstNode> &errorNode);

    /**
     * Emits an error because an unknown symbol.
     * @param module
     * @param symbolName
     * @param errorNode
     */
    void emitUnknownSymbolError(const std::string &module,
                                const std::string &symbolName,
                                const std::shared_ptr<AstNode> &errorNode);

    /**
     * Tries to end a scope. If currentScope == globalScope, an exception will be thrown.
     *
     * Internally, this function calls exitScope.
     */
    void endScope();

    /**
     * This method is intended for passes after the first semantic pass (SymbolDefinitionVisitor).
     */
    void enterScope(const std::shared_ptr<Scope> &scope);

    /**
     * Sets current scope to the parent of current scope. If current scope is global scope, an exception is thrown.
     */
    void exitScope();

    /**
     * Returns the current scope. If no scope is opened, global scope is returned.
     * @return const std::shared_ptr<Scope> &
     */
    const std::shared_ptr<Scope> &getCurrentScope() const;

    /**
     * Returns the global scope.
     * @return const std::shared_ptr<Scope> &
     */
    const std::shared_ptr<Scope> &getGlobalScope() const;

  private:
    size_t m_currentSymbolId; // Used to give symbols an ID.
    std::shared_ptr<Scope> m_currentScope;
    std::shared_ptr<Scope> m_globalScope;
};

/**
 * Simple RAII (Resource Acquisition Is Initialization) guard to manage scopes AFTER SymbolDefinitionVisitor.
 */
class ScopeGuard
{
  public:
    inline ScopeGuard(const std::shared_ptr<class BasicSemanticContext> &ctx, const std::shared_ptr<Scope> &scope) :
        m_ctx(ctx)
    {
        m_ctx->enterScope(scope);
    }

    ~ScopeGuard() { m_ctx->exitScope(); }

    // Disable copying to prevent double-exiting
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;

  private:
    std::shared_ptr<BasicSemanticContext> m_ctx;
};

#endif // EZPACKER_BASICSEMANTICCONTEXT_H
