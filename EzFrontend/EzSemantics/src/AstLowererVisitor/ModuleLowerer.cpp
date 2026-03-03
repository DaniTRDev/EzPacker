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
    MirBlock *block = ctx->getEmitterContext()->createBlock();
    ModuleHeader *header = module->getHeader();
    SymbolAnnotation *sym = module->getAnnotation<SymbolAnnotation>();

    ctx->getSemanticContext()->linkSymbolToMirId(sym->getSymbol(), block->getId());
    ctx->getEmitterContext()->bindToBlock(block);

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
