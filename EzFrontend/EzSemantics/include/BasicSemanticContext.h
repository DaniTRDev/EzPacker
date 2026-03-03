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
     * @param definingNode
     * @param symbolType
     * @param outSymbol
     * @param symbolDataType
     * @param symbolName
     * @return bool
     */
    bool createSymbol(AstNode *definingNode,
                      SymbolType symbolType,
                      Symbol **outSymbol,
                      Type *symbolDataType,
                      const std::string_view &symbolName);
    /**
     * Returns true if the current context is inside a loop. This is used to check if break and continue statements are
     * valid (in most cases)-
     * @return bool
     */
    bool isContextInsideLoop() const;

    /**
     * Returns true if the current scope is the global scope.
     * @return bool
     */
    bool isCurrentScopeGlobalScope() const;

    /**
     * Returns true if the given symbol has a MIR ID linked.
     * @param symbol
     * @return bool
     */
    bool isSymbolLinkedToMir(Symbol *symbol) const;

    /**
     * Links the given symbol to the MIR id and returns true if succeded. If the symbol is already linked with a MIR ID,
     * false is returned.
     * @param sym
     * @param mirId
     * @return bool
     */
    bool linkSymbolToMirId(Symbol *sym, size_t mirId);

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
    bool resolveSymbolInScope(const std::string_view &symbolName, Symbol **outSymbol, bool searchParent);

    /**
     * Returns the MIR ID of the given symbol, if any. It returns MIRID_INVALID if the symbol is not linked to any
     * MIR ID.
     * @param sym
     * @return MirId
     */
    MirId getMirIdOfSymbol(Symbol *sym) const;

    /**
     * Returns the current scope. If no scope is opened, global scope is returned.
     * @return Scope *
     */
    Scope *getCurrentScope() const;

    /**
     * Returns the pool of annotations.
     * @return TypedPool *
     */
    TypedPool *getAnnotPool();

    /**
     * Returns the pool of symbols.
     * @return TypedPool *
     */
    TypedPool *getSymbolPool();

    /**
     * Begins a new scope with the given name. Scope names CAN BE duplicated.
     * @param name
     */
    void beginScope(const std::string_view &name);

    /**
     * Emits an error because of the redefinition of a symbol. It will print the first place the symbol was defined in.
     * @param module
     * @param symbolName
     * @param errorNode
     */
    void
    emitSymbolRedefinitionError(const std::string_view &module, const std::string_view &symbolName, AstNode *errorNode);

    /**
     * Emits an error because an unknown symbol.
     * @param module
     * @param symbolName
     * @param errorNode
     */
    void emitUnknownSymbolError(const std::string_view &module, const std::string_view &symbolName, AstNode *errorNode);

    /**
     * Tries to end a scope. If currentScope == globalScope, an exception will be thrown.
     *
     * Internally, this function calls exitScope.
     */
    void endScope();

    /**
     * Enters in a loop causing the loop nesting level to increase by one. This is used to track if we are in a loop and
     how many nested loops we are in. This is useful for break and continue statements, which need to know if they are
     inside a loop and how many loops they need to break/continue.
     */
    void enterLoop();

    /**
     * This method is intended for passes after the first semantic pass (SymbolDefinitionVisitor).
     * @param scope
     */
    void enterScope(Scope *scope);

    /**
     * Exits a loop causing the loop nesting level to decrease by one. If we are not in any loop, an exception is
     * thrown.
     */
    void exitLoop();

    /**
     * Sets current scope to the parent of current scope. If current scope is global scope, an exception is thrown.
     */
    void exitScope();

    /**
     * Returns the global scope.
     * @return const std::shared_ptr<Scope> &
     */
    const std::shared_ptr<Scope> &getGlobalScope() const;

  private:
    Scope *m_currentScope;
    size_t m_currentLoopNestLevel; // Used to track the current loop nesting level. It starts at 0 (not in any loop).
    size_t m_currentSymbolId;      // Used to give symbols an ID. Error is 0, this starts at 1.
    TypedPool m_annotationPool;    // Cache-friendly container of annotations.
    TypedPool m_symbolPool;        // Cache-friendly container of symbols.
    std::map<size_t, size_t> m_symbolToMirMap; // Map that links a symbol with its corresponding MIR ID.
    std::shared_ptr<Scope> m_globalScope;
    std::vector<std::shared_ptr<Scope>>
            m_scopes; /*
                       * Scopes can't be easily adapted into the cache-friendly pool because
                       * they have a dynamic Map. There will be quite frequent searches so
                       * and for this reason, O(log n) (map search) < O (n) (linked list search)
                       */
};

/**
 * Simple RAII (Resource Acquisition Is Initialization) guard to manage scopes AFTER SymbolDefinitionVisitor.
 */
class ScopeGuard
{
  public:
    inline ScopeGuard(const std::shared_ptr<class BasicSemanticContext> &ctx, Scope *scope) : m_ctx(ctx)
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

/**
 * Simple RAII (Resource Acquisition Is Initialization) guard to manage loops. It will call enterLoop on construction
 * and exitLoop on destruction.
 */
class LoopGuard
{
  public:
    inline LoopGuard(const std::shared_ptr<class BasicSemanticContext> &ctx) : m_ctx(ctx) { m_ctx->enterLoop(); }

    ~LoopGuard() { m_ctx->exitLoop(); }

    // Disable copying to prevent double-exiting
    LoopGuard(const LoopGuard &) = delete;
    LoopGuard &operator=(const LoopGuard &) = delete;

  private:
    std::shared_ptr<BasicSemanticContext> m_ctx;
};

#endif // EZPACKER_BASICSEMANTICCONTEXT_H
