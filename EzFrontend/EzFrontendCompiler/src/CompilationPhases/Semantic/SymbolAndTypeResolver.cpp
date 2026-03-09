#include "CompilationPhases/Semantic/SymbolAndTypeResolver.h"

bool SymbolAndTypeResolverPhase::execute(struct FrontendCompilationUnit *unit)
{
    SymbolAndTypeResolverVisitor symbolAndTypeResolverVisitor;
    symbolAndTypeResolverVisitor.setSemanticContext(unit->getSemanticContext());

    for (AstNode *node : *unit->getGlobalScopeAstNodes())
    {
        if (!node->accept(&symbolAndTypeResolverVisitor))
        {
            unit->emitError(ErrorSeverity::Fatal,
                            "Semantic analysis failed during symbol and type resolution",
                            getName());
            return false;
        }
    }

    return true;
}

const char *SymbolAndTypeResolverPhase::getName() { return "SymbolAndTypeResolverPhase"; }
