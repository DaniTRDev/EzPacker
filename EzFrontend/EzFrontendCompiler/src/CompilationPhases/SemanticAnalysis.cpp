#include "CompilationPhases/SemanticAnalysis.h"

bool SemanticAnalysisPhase::execute(struct FrontendCompilationUnit *unit)
{


    SymbolDefinitionPhase symbolDefinitionPhase;
    if (!symbolDefinitionPhase.execute(unit))
    {
        return false;
    }

    SymbolAndTypeResolverPhase symbolResolutionPhase;
    if (!symbolResolutionPhase.execute(unit))
    {
        return false;
    }

    TypeCheckPhase typeCheckPhase;
    if (!typeCheckPhase.execute(unit))
    {
        return false;
    }

    return true;
}

const char *SemanticAnalysisPhase::getName() { return "SemanticAnalysisPhase"; }
