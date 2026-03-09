#include "CompilationPhases/Semantic/TypeCheck.h"

bool TypeCheckPhase::execute(struct FrontendCompilationUnit *unit)
{
    TypeCheckVisitor typeCheckVisitor;
    typeCheckVisitor.setSemanticContext(unit->getSemanticContext());

    for (AstNode *node : *unit->getGlobalScopeAstNodes())
    {
        if (!node->accept(&typeCheckVisitor))
        {
            unit->emitError(ErrorSeverity::Fatal, "Semantic analysis failed during type checking", getName());
            return false;
        }
    }

    return true;
}

const char *TypeCheckPhase::getName() { return "TypeCheckPhase"; }
