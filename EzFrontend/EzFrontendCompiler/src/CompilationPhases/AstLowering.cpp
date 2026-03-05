#include "CompilationPhases/AstLowering.h"

bool AstLoweringPhase::execute(struct FrontendCompilationUnit *unit)
{
    const std::shared_ptr<ErrorCollector> &errorCollector = unit->getErrorCollector();
    const std::shared_ptr<BasicSemanticContext> &semanticContext = unit->getSemanticContext();
    const std::shared_ptr<SourceManager> &sourceManager = unit->getSourceManager();
    TypedPoolSlice<AstNode> *globalScopeAstNodes = unit->getGlobalScopeAstNodes();

    auto mirEmitterContext = std::make_shared<MirEmitterContext>(errorCollector, sourceManager);
    auto mirEmitter = std::make_shared<MirEmitter>(mirEmitterContext.get());
    auto mirGlobalDataEmitter = std::make_shared<MirGlobalDataEmitter>(mirEmitterContext.get());

    auto loweringContext =
            std::make_shared<LoweringContext>(semanticContext, mirEmitter, mirEmitterContext, mirGlobalDataEmitter);

    AstLowererVisitor lowererVisitor(loweringContext);
    lowererVisitor.setSemanticContext(unit->getSemanticContext());
    
    loweringContext->setOwnerVisitor(&lowererVisitor);
    
    for (AstNode *node : *globalScopeAstNodes)
    {
        if (!node->accept(&lowererVisitor))
        {
            unit->emitError(ErrorSeverity::Fatal, "MIR emission failed during AST lowering.", getName());
            return false;
        }
    }

    unit->setMirEmitterContext(mirEmitterContext);
    unit->setMirEmitter(mirEmitter);
    unit->setMirGlobalDataEmitter(mirGlobalDataEmitter);
    unit->setLoweringContext(loweringContext);
    return true;
}

const char *AstLoweringPhase::getName() { return "AstLoweringPhase"; }
