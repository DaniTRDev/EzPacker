#include "Lowerers/LabelLowerer.h"
#include "AstLowererVisitor.h"

bool LabelLowerer::lower(AstNode *node, AstLoweringContext *ctx)
{
    Label *labelNode = dynamic_cast<Label *>(node);
    MirBlock *labelBlock = ctx->getEmitterContext()->createBlock();
    SymbolAnnotation *symbolAnnot = labelNode->getAnnotation<SymbolAnnotation>();

    ctx->linkSymbolToMirId(symbolAnnot->getSymbol(), labelBlock->getId());
    ctx->getEmitter()->emitJMP(ctx->getEmitterContext()->createReference(labelBlock));

    ctx->getEmitterContext()->bindToBlock(labelBlock);
    if (!labelNode->getCodeScope()->accept(ctx->getOwnerLowererVisitor()))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal, "Could not lower label body", "LabelLowerer");
        return false; // Error handling
    }

    // We leave the context bound to the label block. Caller can just bind the context to another block if needed.
    return true;
}
