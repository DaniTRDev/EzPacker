/**
 * @file FrontendCompilationUnit.h
 * @brief Represents a single source file or compilation unit.
 *
 * A FrontendCompilationUnit encapsulates all the state and resources associated with
 * compiling a single source file. This includes its source code, token stream, AST,
 * and various contexts used during compilation (parsing, semantics, MIR generation).
 *
 * It acts as a container that moves through the compilation pipeline, accumulating
 * information at each stage.
 */
#ifndef EZPACKER_FRONTENDCOMPILATIONUNIT_H
#define EZPACKER_FRONTENDCOMPILATIONUNIT_H

#include "EzFrontendCompilerCommon.h"

/**
 * @class FrontendCompilationUnit
 * @brief Encapsulates the compilation state for a single source file.
 *
 * This class manages the lifecycle of a compilation unit, from source code loading
 * to MIR generation. It holds references to the tokenizer, parser, semantic analyzer,
 * and MIR emitter contexts specific to this unit.
 *
 * It inherits from `ErrorEmitter` to allow reporting diagnostics associated with this unit.
 */
class FrontendCompilationUnit : public ErrorEmitter
{
  public:
    /**
     * @brief Constructs a new FrontendCompilationUnit.
     *
     * Initializes the unit with the necessary error handling and source management components.
     * Note that the source content is not loaded until `create()` is called.
     *
     * @param errorCollector Shared pointer to the error collector for reporting diagnostics.
     * @param sourceManager Shared pointer to the source manager for handling source files.
     */
    FrontendCompilationUnit(const std::shared_ptr<ErrorCollector> &errorCollector,
                            const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * @brief Initializes the compilation unit with a source file.
     *
     * Links the unit to a specific source file identified by `sourceId`. This prepares the unit
     * for tokenization and parsing.
     *
     * @param sourceId The unique identifier for the source file (assigned by SourceManager).
     * @return `true` if initialization was successful; `false` otherwise (e.g., invalid source ID).
     */
    bool create(size_t sourceId);

    /**
     * @brief Gets the source ID associated with this unit.
     *
     * @return The unique identifier for the source file, or 0 if not initialized.
     */
    size_t getTargetSourceId() const;

    /**
     * @brief Retrieves the AST nodes belonging to the global scope of this unit.
     *
     * These nodes represent top-level declarations (functions, globals, etc.) parsed from the source.
     *
     * @return Pointer to the `TypedPoolLinkedList` containing the global AST nodes.
     */
    TypedPoolLinkedList<AstNode> *getGlobalScopeAstNodes();

    /**
     * @brief Releases resources held by this compilation unit.
     *
     * Clears the tokenizer, parser, semantic context, and other resources to free memory.
     * This is typically called after the unit has been fully processed and its output (MIR)
     * has been consumed.
     */
    void cleanup();

    // ── Setters ─────────────────────────────────────────────────────────────

    /**
     * @brief Sets the global scope AST nodes for this unit.
     *
     * @param globalScopeAstNodes Pointer to the AST node slice.
     */
    void setGlobalScopeAstNodes(TypedPoolLinkedList<AstNode> *globalScopeAstNodes);

    /**
     * @brief Sets the lowering context used for AST-to-MIR conversion.
     *
     * @param loweringContext Shared pointer to the lowering context.
     */
    void setLoweringContext(const std::shared_ptr<AstLoweringContext> &loweringContext);

    /**
     * @brief Sets the MIR emitter used for generating intermediate representation.
     *
     * @param mirEmitter Shared pointer to the MIR emitter.
     */
    void setMirEmitter(const std::shared_ptr<MirEmitter> &mirEmitter);

    /**
     * @brief Sets the MIR emitter context.
     *
     * @param mirEmitterContext Shared pointer to the MIR emitter context.
     */
    void setMirEmitterContext(const std::shared_ptr<MirEmitterContext> &mirEmitterContext);
    
    /**
     * @brief Sets the parsing context used for syntax analysis.
     *
     * @param parsingContext Shared pointer to the parsing context.
     */
    void setParsingContext(const std::shared_ptr<BasicParsingContext> &parsingContext);

    /**
     * @brief Sets the semantic context used for type checking and analysis.
     *
     * @param semanticContext Shared pointer to the semantic context.
     */
    void setSemanticContext(const std::shared_ptr<BasicSemanticContext> &semanticContext);

    /**
     * @brief Sets the tokenizer used for lexical analysis.
     *
     * @param tokenizer Shared pointer to the tokenizer.
     */
    void setTokenizer(const std::shared_ptr<BasicTokenizer> &tokenizer);

    // ── Getters ─────────────────────────────────────────────────────────────

    /**
     * @brief Gets the parsing context.
     *
     * @return Const reference to the shared pointer of the parsing context.
     */
    const std::shared_ptr<BasicParsingContext> &getParsingContext() const;

    /**
     * @brief Gets the semantic context.
     *
     * @return Const reference to the shared pointer of the semantic context.
     */
    const std::shared_ptr<BasicSemanticContext> &getSemanticContext() const;

    /**
     * @brief Gets the tokenizer.
     *
     * @return Const reference to the shared pointer of the tokenizer.
     */
    const std::shared_ptr<BasicTokenizer> &getTokenizer() const;

    /**
     * @brief Gets the lowering context.
     *
     * @return Const reference to the shared pointer of the lowering context.
     */
    const std::shared_ptr<AstLoweringContext> &getLoweringContext() const;

    /**
     * @brief Gets the MIR emitter.
     *
     * @return Const reference to the shared pointer of the MIR emitter.
     */
    const std::shared_ptr<MirEmitter> &getMirEmitter() const;

    /**
     * @brief Gets the MIR emitter context.
     *
     * @return Const reference to the shared pointer of the MIR emitter context.
     */
    const std::shared_ptr<MirEmitterContext> &getMirEmitterContext() const;
    
    /**
     * @brief Gets the global scope associated with this unit.
     *
     * The global scope contains top-level symbols visible in this unit.
     *
     * @return Const reference to the shared pointer of the global scope.
     */
    const std::shared_ptr<Scope> &getGlobalScope() const;

  private:
    TypedPoolLinkedList<AstNode> *m_globalScopeAstNodes; // AST nodes that belong to the global scope.
    size_t m_targetSourceId;                        // The source ID of the source file being compiled.
    std::set<std::string> m_includedFiles; // Set of file paths that have been included during the compilation process.
    std::shared_ptr<BasicParsingContext> m_parsingContext;
    std::shared_ptr<BasicSemanticContext> m_semanticContext;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<AstLoweringContext> m_loweringContext;
    std::shared_ptr<MirEmitter> m_mirEmitter;
    std::shared_ptr<MirEmitterContext> m_mirEmitterContext;
    std::shared_ptr<Scope> m_globalScope; // The global scope of the source file being compiled.
};

#endif // EZPACKER_FRONTENDCOMPILATIONUNIT_H
