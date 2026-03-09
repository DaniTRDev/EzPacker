#include "CompilationPhases/Semantic/SymbolDefinition.h"

bool SymbolDefinitionPhase::execute(struct FrontendCompilationUnit *unit)
{
    SymbolDefinitionVisitor definitionVisitor;
    definitionVisitor.setSemanticContext(unit->getSemanticContext());

    for (AstNode *node : *unit->getGlobalScopeAstNodes())
    {
        if (!node->accept(&definitionVisitor))
        {
            unit->emitError(ErrorSeverity::Fatal, "Semantic analysis failed during symbol definition", getName());
            return false;
        }
    }

    return true;
}

const char *SymbolDefinitionPhase::getName() { return "SymbolDefinition"; }
