#include "CompilationPhases/SemanticAnalysis.h"

bool SemanticAnalysisPhase::execute(struct FrontendCompilationUnit *unit)
{
    SymbolAndTypeResolverVisitor symbolAndTypeResolverVisitor;
    SymbolDefinitionVisitor definitionVisitor;
    TypeCheckVisitor typeCheckVisitor;

    const std::shared_ptr<ErrorCollector> &errorCollector = unit->getErrorCollector();
    const std::shared_ptr<SourceManager> &sourceManager = unit->getSourceManager();
    std::shared_ptr<BasicSemanticContext> semanticContext =
            std::make_shared<BasicSemanticContext>(errorCollector, sourceManager, unit->getGlobalScope());
    TypedPoolSlice<AstNode> *globalScopeAstNodes = unit->getGlobalScopeAstNodes();

    symbolAndTypeResolverVisitor.setSemanticContext(semanticContext);
    definitionVisitor.setSemanticContext(semanticContext);
    typeCheckVisitor.setSemanticContext(semanticContext);

    /**
     * Important: the order of these visitors is important. Instead of traversing the AST in a single loop and executing
     * the three visitors, we are delaying it to three separate loops. This allows forward declarations to work
     * correctly, as the definition visitor will populate the symbol table with all symbols before the resolution and
     * type checking visitors run. If we were to run all three visitors in a single loop, we would encounter issues with
     * forward declarations, as the resolution and type checking visitors would not be able to find symbols that have
     * not been defined yet.
     */

    for (AstNode *node : *globalScopeAstNodes)
    {
        if (!node->accept(&definitionVisitor))
        {
            unit->emitError(ErrorSeverity::Fatal, "Semantic analysis failed during symbol definition", getName());
            return false;
        }
    }

    for (AstNode *node : *globalScopeAstNodes)
    {
        if (!node->accept(&symbolAndTypeResolverVisitor))
        {
            unit->emitError(ErrorSeverity::Fatal,
                            "Semantic analysis failed during symbol and type resolution",
                            getName());
            return false;
        }
    }

    for (AstNode *node : *globalScopeAstNodes)
    {
        if (!node->accept(&typeCheckVisitor))
        {
            unit->emitError(ErrorSeverity::Fatal, "Semantic analysis failed during type checking", getName());
            return false;
        }
    }

    unit->setSemanticContext(semanticContext);
    unit->setGlobalScopeAstNodes(globalScopeAstNodes);
    return true;
}

const char *SemanticAnalysisPhase::getName() { return "SemanticAnalysisPhase"; }
