#include "AstLowererVisitor/ContinueLowerer.h"

bool ContinueLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    ContinueAstNode *continueNode = dynamic_cast<ContinueAstNode *>(node);
    const LoopContext &loopCtx = ctx->getCurrentLoopContext();

    ctx->getEmitter()->emitJMP(MirReference{ loopCtx.m_continueTarget->getId() });

    // Any code written after a 'break' is unreachable. To prevent generating
    // invalid MIR (where a basic block has instructions *after* a terminator JMP),
    // we create a dummy "Dead Code" block and bind the emitter to it.
    MirBlock *deadCodeBlock = ctx->getEmitterContext()->createBlock();
    return ctx->getEmitterContext()->bindToBlock(deadCodeBlock);
}
