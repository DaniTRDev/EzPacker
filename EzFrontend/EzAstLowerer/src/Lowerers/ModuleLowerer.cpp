#include "Lowerers/ModuleLowerer.h"
#include "AstLowererVisitor.h"

bool ModuleLowerer::lower(AstNode *node, AstLoweringContext *ctx)
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

    MirFunction *func = ctx->getEmitterContext()->createFunction(moduleType, nullptr, nullptr, sym->getName());

    ctx->linkSymbolToMirId(sym, func->getId());
    ctx->getEmitterContext()->bindToBlock(func->getEntryPoint());

    for (AstNode *expr : *header->getExpressions())
    {
        if (!expr->accept(ctx->getOwnerLowererVisitor()))
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Failed to lower module header expression",
                                                 "ModuleHeaderLowerer");
            return false;
        }

        SymbolAnnotation *sym = expr->getAnnotation<SymbolAnnotation>();
        std::pair<MirOperand, MirType *> op = ctx->popOperand();

        func->appendParameter(op.first, op.second, sym->getSymbol()->getName().data());
    }

    if (!body->accept(ctx->getOwnerLowererVisitor()))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal, "Could not lower module body", "ModuleLowerer");
        return false;
    }

    return true;
}
