#include "CompilationPhases/AstLowering.h"

bool AstLoweringPhase::execute(struct FrontendCompilationUnit *unit)
{
    const std::shared_ptr<ErrorCollector> &errorCollector = unit->getErrorCollector();
    const std::shared_ptr<BasicSemanticContext> &semanticContext = unit->getSemanticContext();
    const std::shared_ptr<SourceManager> &sourceManager = unit->getSourceManager();
    TypedPoolLinkedList<AstNode> *globalScopeAstNodes = unit->getGlobalScopeAstNodes();

    auto mirEmitterContext = std::make_shared<MirEmitterContext>(errorCollector, sourceManager);
    auto mirEmitter = std::make_shared<MirEmitter>(mirEmitterContext.get());

    auto loweringContext = std::make_shared<AstLoweringContext>(semanticContext, mirEmitter, mirEmitterContext);

    TypeLowerer typeLowerer;
    AstLowererVisitor lowererVisitor(loweringContext);

    if (!typeLowerer.lower(semanticContext->getTypeTable().get(), loweringContext.get()))
    {
        unit->emitError(ErrorSeverity::Fatal, "Type lowering failed during AST lowering.", getName());
        return false;
    }

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
    unit->setLoweringContext(loweringContext);
    return true;
}

const char *AstLoweringPhase::getName() { return "AstLoweringPhase"; }
