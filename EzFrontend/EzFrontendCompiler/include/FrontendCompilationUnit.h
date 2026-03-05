/**
 * @file FrontendCompilationUnit.h
 * @brief Self-contained compilation unit for the frontend. This class encapsulates the entire process of compiling a
 * source file, including tokenization and parsing.
 */
#ifndef EZPACKER_FRONTENDCOMPILATIONUNIT_H
#define EZPACKER_FRONTENDCOMPILATIONUNIT_H

#include "EzFrontendCompilerCommon.h"

/**
 * Represents a single compilation unit in the frontend. It encapsulates the entire process of compiling a source file,
 * including including tokenization and parsing.
 *
 * To act in consonance with other components, it inherits from ErrorEmitter, allowing it to emit errors that can be
 * collected and logged by an ErrorCollector. This design ensures that any errors encountered during the compilation
 * process are properly reported and can be handled by the caller.
 */
class FrontendCompilationUnit : public ErrorEmitter
{
  public:
    /**
     * Initializes the compilation unit without source content. This allows for a two-step initialization where the unit
     * is created first and then the source content is provided later through the create() function. This can be useful
     * in scenarios where the source content is not immediately available at the time of construction.
     * @param errorCollector
     * @param sourceManager
     */
    FrontendCompilationUnit(const std::shared_ptr<ErrorCollector> &errorCollector,
                            const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Creates the compilation unit and adds the given source to the source manager.
     * @param sourceContent
     * @param sourceName
     * @return bool
     */
    bool create(const std::string &sourceContent, const std::string &sourceName);

    /**
     * Returns the source ID of the source file being compiled. This ID is assigned by the source manager when the
     * source content is added. If no source was added, returns 0.
     * @return size_t
     */
    size_t getTargetSourceId() const;

    /**
     * Returns a pointer to the slice of AST nodes that belong to the global scope. This slice is populated during the
     * parsing phase and is used in subsequent phases of the compilation process, such as semantic analysis and MIR
     * emission.
     * @return const TypedPoolSlice<AstNode> *
     */
    const TypedPoolSlice<AstNode> *getGlobalScopeAstNodes() const;
    
    /**
     * Returns a pointer to the slice of AST nodes that belong to the global scope. This slice is populated during the
     * parsing phase and is used in subsequent phases of the compilation process, such as semantic analysis and MIR
     * emission.
     * @return TypedPoolSlice<AstNode> *&
     */
    TypedPoolSlice<AstNode> *&getGlobalScopeAstNodes();
    
    /**
     * Cleans up all resources used by the compilation unit. This includes clearing the tokenizer, parsing context,
     * semantic context, and MIR emitter. After this function is called, the compilation unit should be in a state where
     * it can be safely destroyed or re-initialized.
     */
    void cleanup();
    
    /**
     * Sets the slice of AST nodes that belong to the global scope.
     * @param globalScopeAstNodes
     */
    void setGlobalScopeAstNodes(TypedPoolSlice<AstNode> *globalScopeAstNodes);
    
    /**
     * Sets the lowering context used by the compilation unit.
     * @param loweringContext
     */
    void setLoweringContext(const std::shared_ptr<LoweringContext> &loweringContext);
    
    /**
     * Sets the MIR emitter used by the compilation unit.
     * @param mirEmitter
     */
    void setMirEmitter(const std::shared_ptr<MirEmitter> &mirEmitter);
    
    /**
     * Sets the MIR emitter context used by the compilation unit.
     * @param mirEmitterContext
     */
    void setMirEmitterContext(const std::shared_ptr<MirEmitterContext> &mirEmitterContext);
    
    /**
     * Sets the MIR global data emitter used by the compilation unit.
     * @param mirGlobalDataEmitter
     */
    void setMirGlobalDataEmitter(const std::shared_ptr<MirGlobalDataEmitter> &mirGlobalDataEmitter);
    
    /**
     * Sets the parsing context used by the compilation unit.
     * @param parsingContext
     */
    void setParsingContext(const std::shared_ptr<BasicParsingContext> &parsingContext);
    
    /**
     * Sets the semantic context used by the compilation unit.
     * @param semanticContext
     */
    void setSemanticContext(const std::shared_ptr<BasicSemanticContext> &semanticContext);
    
    /**
     * Sets the tokenizer used by the compilation unit.
     * @param tokenizer
     */
    void setTokenizer(const std::shared_ptr<BasicTokenizer> &tokenizer);
    
    /**
     * Returns a const reference to the parsing context used by the compilation unit.
     * @return const std::shared_ptr<BasicParsingContext> &
     */
    const std::shared_ptr<BasicParsingContext> &getParsingContext() const;
    
    /**
     * Returns a const reference to the semantic context used by the compilation unit.
     * @return const std::shared_ptr<BasicSemanticContext> &
     */
    const std::shared_ptr<BasicSemanticContext> &getSemanticContext() const;
    
    /**
     * Returns a const reference to the tokenizer used by the compilation unit.
     * @return const std::shared_ptr<BasicTokenizer> &
     */
    const std::shared_ptr<BasicTokenizer> &getTokenizer() const;
    
    /**
     * Returns a const reference to the lowering context used by the compilation unit.
     * @return const std::shared_ptr<LoweringContext> &
     */
    const std::shared_ptr<LoweringContext> &getLoweringContext() const;
    
    /**
     * Returns a const reference to the MIR emitter used by the compilation unit.
     * @return const std::shared_ptr<MirEmitter> &
     */
    const std::shared_ptr<MirEmitter> &getMirEmitter() const;
    
    /**
     * Returns a const reference to the MIR emitter context used by the compilation unit.
     * @return const std::shared_ptr<MirEmitterContext> &
     */
    const std::shared_ptr<MirEmitterContext> &getMirEmitterContext() const;
    
    /**
     * Returns a const reference to the MIR global data emitter used by the compilation unit.
     * @return const std::shared_ptr<MirGlobalDataEmitter> &
     */
    const std::shared_ptr<MirGlobalDataEmitter> &getMirGlobalDataEmitter() const;
    
    /**
     * Returns a pointer to the global scope of the source file being compiled. The global scope is the top-level scope
     * that contains all other scopes and declarations in the source file. It is populated during the parsing phase and
     * is used in subsequent phases of the compilation process, such as semantic analysis and MIR emission.
     * @return const std::shared_ptr<Scope> &
     */
    const std::shared_ptr<Scope> &getGlobalScope() const;

  private:
    TypedPoolSlice<AstNode> *m_globalScopeAstNodes; // AST nodes that belong to the global scope.
    size_t m_targetSourceId;                        // The source ID of the source file being compiled.
    std::shared_ptr<BasicParsingContext> m_parsingContext;
    std::shared_ptr<BasicSemanticContext> m_semanticContext;
    std::shared_ptr<BasicTokenizer> m_tokenizer;
    std::shared_ptr<LoweringContext> m_loweringContext;
    std::shared_ptr<MirEmitter> m_mirEmitter;
    std::shared_ptr<MirEmitterContext> m_mirEmitterContext;
    std::shared_ptr<MirGlobalDataEmitter> m_mirGlobalDataEmitter;
    std::shared_ptr<Scope> m_globalScope;                           // The global scope of the source file being compiled.
};

#endif // EZPACKER_FRONTENDCOMPILATIONUNIT_H
