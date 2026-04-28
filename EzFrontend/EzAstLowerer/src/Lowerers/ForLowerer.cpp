#include "Lowerers/ForLowerer.h"
#include "AstLowererVisitor.h"

bool ForLowerer::lower(AstNode *node, AstLoweringContext *ctx)
{
    ForAstNode *_for = dynamic_cast<ForAstNode *>(node);
    MirEmitter *emitter = ctx->getEmitter().get();
    MirEmitterContext *emitterCtx = ctx->getEmitterContext().get();

    MirBlock *conditionBlock = emitterCtx->createBlock();
    MirBlock *bodyBlock = emitterCtx->createBlock();
    MirBlock *initializationBlock = emitterCtx->createBlock();
    MirBlock *mergeBlock = emitterCtx->createBlock();
    MirBlock *nextItBlock = emitterCtx->createBlock();

    emitter->emitJMP(ctx->getEmitterContext()->createReference(initializationBlock));

    emitterCtx->bindToBlock(initializationBlock);
    _for->getInitialization()->accept(ctx->getOwnerLowererVisitor());
    emitter->emitJMP(ctx->getEmitterContext()->createReference(conditionBlock));

    emitterCtx->bindToBlock(conditionBlock);
    ctx->pushBlock(bodyBlock);
    ctx->pushBlock(mergeBlock);
    _for->getCondition()->accept(ctx->getOwnerLowererVisitor());

    ctx->enterLoop(LoopContext{ .m_breakTarget = mergeBlock, .m_continueTarget = nextItBlock });
    {
        emitterCtx->bindToBlock(bodyBlock);
        _for->getBody()->accept(ctx->getOwnerLowererVisitor());
        emitter->emitJMP(ctx->getEmitterContext()->createReference(nextItBlock));
    }
    ctx->exitLoop();

    emitterCtx->bindToBlock(nextItBlock);
    _for->getNextItClause()->accept(ctx->getOwnerLowererVisitor());
    emitter->emitJMP(ctx->getEmitterContext()->createReference(conditionBlock));

    emitterCtx->bindToBlock(mergeBlock);

    return true;
}
