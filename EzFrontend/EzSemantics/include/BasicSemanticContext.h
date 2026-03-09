/**
 * @file BasicSemanticContext.h
 * @brief Shared semantic state used by all EzSemantics passes.
 *
 * `BasicSemanticContext` is the main coordination object for semantic
 * analysis and lowering. It owns the root scope, tracks the currently active
 * scope, provides allocation pools for semantic annotations and symbols, and
 * centralises diagnostic emission.
 *
 * Public responsibilities:
 *   - Manage lexical scopes created during `SymbolDefinitionVisitor`.
 *   - Re-enter previously created scopes in later passes.
 *   - Create symbols with unique IDs and resolve them by name.
 *   - Track whether the current traversal is inside a loop and/or switch.
 *   - Emit user-facing diagnostics tied to AST source locations.
 *
 * Lifetime notes:
 *   - A global scope always exists after construction.
 *   - Symbols and annotations are allocated from the internal pools owned by
 *     the context; callers should treat returned raw pointers as non-owning.
 *   - `Scope` objects are heap-allocated and retained by the context for the
 *     lifetime of the semantic session.
 *
 * Two scope-related RAII guards and one loop-related guard are provided:
 *   - `ScopeGuard`: temporarily re-enters an already existing scope.
 *   - `ScopeCreatorGuard`: creates a fresh scope, attaches it to an AST node
 *     as a `ScopeAnnotation`, then closes it on destruction.
 *   - `LoopGuard`: increments loop nesting on entry and decrements it on exit.
 */
#ifndef EZPACKER_BASICSEMANTICCONTEXT_H
#define EZPACKER_BASICSEMANTICCONTEXT_H

#include "EzSemanticsCommon.h"
#include "Scope/Scope.h"
#include "SemanticAnnotations/ScopeAnnotation.h"

/**
 * Shared semantic-analysis context.
 *
 * The constructor ensures there is always an active global scope. Until a
 * nested scope is created or re-entered, both `getCurrentScope()` and
 * `getGlobalScope()` refer to that root scope.
 */
class BasicSemanticContext : public ErrorEmitter
{
  public:
    /**
     * Creates a semantic context.
     *
     * If `globalScope` is null, a fresh scope named `"global"` is created.
     * Passing an existing global scope allows several ASTs/files to share the
     * same symbol table across semantic passes.
     *
     * @param errorCollector Diagnostic sink used by `ErrorEmitter`.
     * @param sourceManager Source manager used to format source locations.
     * @param globalScope Optional pre-existing root scope to reuse.
     */
    BasicSemanticContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                         const std::shared_ptr<SourceManager> &sourceManager,
                         const std::shared_ptr<Scope> &globalScope = nullptr);

    /**
     * Defines a new symbol in the current scope.
     *
     * The symbol is created only if another symbol with the same name does not
     * already exist in the current scope. Parent scopes are not considered a
     * conflict, which means shadowing is allowed and handled by callers.
     *
     * On success, `*outSymbol` receives the newly created symbol and the method
     * assigns it a unique context-wide ID. On failure, the method returns
     * `false` and performs no mutation.
     *
     * @param definingNode AST node that introduces the symbol.
     * @param symbolType Semantic category of the symbol.
     * @param outSymbol Output parameter for the created symbol.
     * @param symbolDataType Declared type of the symbol, if any.
     * @param symbolName Unqualified symbol name as it appears in source.
     * @return `true` if the symbol was inserted in the current scope.
     */
    bool createSymbol(AstNode *definingNode,
                      SymbolType symbolType,
                      Symbol **outSymbol,
                      Type *symbolDataType,
                      const std::string_view &symbolName);
    /**
     * Returns whether the current traversal is nested inside at least one
     * loop.
     *
     * This is consumed by semantic checks for `break` / `continue`.
     */
    bool isContextInsideLoop() const;

    /**
     * Returns whether the current traversal is nested inside at least one
     * `switch`.
     *
     * This is consumed by semantic checks for `break`.
     */
    bool isContextInsideSwitch() const;

    /**
     * Returns `true` when the active scope is the root/global scope.
     */
    bool isCurrentScopeGlobalScope() const;

    /**
     * Resolves a symbol name starting from the current scope.
     *
     * If `searchParent` is `false`, only the current lexical scope is queried.
     * If `searchParent` is `true`, lookup walks parent scopes until the symbol
     * is found or the global scope is exhausted.
     *
     * @param symbolName Symbol name to resolve.
     * @param outSymbol Optional output pointer receiving the matching symbol.
     * @param searchParent Whether to continue lookup in parent scopes.
     * @return `true` when a symbol with that name is found.
     */
    bool resolveSymbolInScope(const std::string_view &symbolName, Symbol **outSymbol, bool searchParent);

    /**
     * Returns the currently active lexical scope.
     */
    Scope *getCurrentScope() const;

    /**
     * Returns the annotation pool used to allocate AST annotations.
     */
    TypedPool *getAnnotPool();

    /**
     * Returns the symbol pool used to allocate `Symbol` instances.
     */
    TypedPool *getSymbolPool();

    /**
     * Creates and enters a child scope below the current scope.
     *
     * The created scope is retained by the context and remains valid for the
     * rest of the semantic session even after `endScope()` / `exitScope()`.
     * Scope names are descriptive only and are allowed to repeat.
     */
    void beginScope(const std::string_view &name);

    /**
     * Emits a redefinition diagnostic for a symbol already present in the
     * current scope.
     *
     * The implementation also emits a secondary diagnostic pointing to the
     * original definition site.
     */
    void
    emitSymbolRedefinitionError(const std::string_view &module, const std::string_view &symbolName, AstNode *errorNode);

    /**
     * Emits an "unknown symbol" diagnostic for an unresolved name use.
     */
    void emitUnknownSymbolError(const std::string_view &module, const std::string_view &symbolName, AstNode *errorNode);

    /**
     * Leaves the current child scope.
     *
     * This is equivalent to `exitScope()` with an additional guard that
     * rejects attempts to leave the global scope.
     */
    void endScope();

    /**
     * Marks the beginning of a loop-sensitive region.
     */
    void enterLoop();

    /**
     * Re-enters an already existing scope.
     *
     * This is intended for passes after symbol definition, where the scope was
     * already created and attached to the AST via `ScopeAnnotation` or
     * `ScopedSymbolAnnotation`.
     */
    void enterScope(Scope *scope);

    /**
     * Marks the beginning of a switch-sensitive region.
     */
    void enterSwitch();

    /**
     * Leaves the current loop-sensitive region.
     *
     * Throws if no loop is currently active.
     */
    void exitLoop();

    /**
     * Restores the active scope to the parent of the current scope.
     *
     * Throws if called while already in the global scope.
     */
    void exitScope();

    /**
     * Leaves the current switch-sensitive region.
     *
     * Throws if no switch is currently active.
     */
    void exitSwitch();

    /**
     * Returns the root/global scope used by this semantic session.
     */
    const std::shared_ptr<Scope> &getGlobalScope() const;

  private:
    Scope *m_currentScope;
    size_t m_currentSwitchLevel;   // Used to track the current switch nesting level. It starts at 0 (not in a switch).
    size_t m_currentLoopNestLevel; // Used to track the current loop nesting level. It starts at 0 (not in any loop).
    size_t m_currentSymbolId;      // Used to give symbols an ID. Error is 0, this starts at 1.
    TypedPool m_annotationPool;    // Cache-friendly container of annotations.
    TypedPool m_symbolPool;        // Cache-friendly container of symbols.
    std::shared_ptr<Scope> m_globalScope;
    std::vector<std::shared_ptr<Scope>>
            m_scopes; /*
                       * Scopes can't be easily adapted into the cache-friendly pool because
                       * they have a dynamic Map. There will be quite frequent searches so
                       * and for this reason, O(log n) (map search) < O (n) (linked list search) is preferred, even if
                       * it means that scopes are not stored contiguously in memory.
                       */
};

/**
 * RAII helper that temporarily enters an already existing scope.
 *
 * This is the guard used by passes that revisit scopes created earlier by
 * `SymbolDefinitionVisitor`.
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
 * RAII helper that creates a new child scope and attaches it to an AST node.
 *
 * Construction calls `beginScope(name)`. Destruction captures the current
 * scope, writes it into a `ScopeAnnotation` owned by `outAst`, and then calls
 * `endScope()`.
 *
 * This is intended for `SymbolDefinitionVisitor`, the pass that owns scope
 * creation.
 */
class ScopeCreatorGuard
{
  public:
    inline ScopeCreatorGuard(AstNode *outAst,
                             const std::shared_ptr<class BasicSemanticContext> &ctx,
                             const std::string_view &name) : m_ctx(ctx)
    {
        m_outAst = outAst;
        m_ctx->beginScope(name);
    }

    ~ScopeCreatorGuard()
    {
        Scope *exitedScope = m_ctx->getCurrentScope();
        ScopeAnnotation *annotation = m_outAst->createAnnotation<ScopeAnnotation>(m_ctx->getAnnotPool());
        annotation->setOwnedScope(exitedScope);

        m_ctx->endScope();
    }

    // Disable copying to prevent double-exiting
    ScopeCreatorGuard(const ScopeGuard &) = delete;
    ScopeCreatorGuard &operator=(const ScopeGuard &) = delete;

  private:
    AstNode *m_outAst;
    std::shared_ptr<BasicSemanticContext> m_ctx;
};

/**
 * RAII helper that marks a region as being inside a loop for semantic checks.
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
