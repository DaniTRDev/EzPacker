#include "AstLowererVisitor/ModuleLowerer.h"
#include "AstLowererVisitor/AstLowererVisitor.h"

bool ModuleHeaderLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    ModuleHeader *header = dynamic_cast<ModuleHeader *>(node);

    for (AstNode *expr : *header->getExpressions())
    {
        if (!expr->accept(ctx->getOwnerLowererVisitor()))
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Failed to lower module header expression",
                                                 "ModuleHeaderLowerer");
            return false;
        }
    }

    return true;
}

bool ModuleLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    Module *module = dynamic_cast<Module *>(node);
    CodeScope *body = module->getBody();
    ModuleHeader *header = module->getHeader();
    SymbolAnnotation *symAnnot = module->getAnnotation<SymbolAnnotation>();
    Symbol *sym = symAnnot->getSymbol();

    MirType *moduleType = ctx->createMirTypeFromSemanticType(sym->getSymbolDataType());
    if (!moduleType)
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "Failed to link module symbol to MIR ID",
                                             "ModuleLowerer");
        return false;
    }

    MirFunction *func = ctx->getEmitterContext()->createFunction(moduleType->getId());

    ctx->linkSymbolToMirId(sym, func->getId());
    ctx->getEmitterContext()->bindToBlock(func->getEntryPoint());

    if (!header->accept(ctx->getOwnerLowererVisitor()))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal, "Could not lower module header", "ModuleLowerer");
        return false;
    }

    if (!body->accept(ctx->getOwnerLowererVisitor()))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal, "Could not lower module body", "ModuleLowerer");
        return false;
    }

    return true;
}
