#include "AstLowererVisitor/ForLowerer.h"
#include "AstLowererVisitor/AstLowererVisitor.h"

bool ForLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    ForAstNode *_for = dynamic_cast<ForAstNode *>(node);
    MirEmitter *emitter = ctx->getEmitter().get();
    MirEmitterContext *emitterCtx = ctx->getEmitterContext().get();

    MirBlock *conditionBlock = emitterCtx->createBlock();
    MirBlock *bodyBlock = emitterCtx->createBlock();
    MirBlock *initializationBlock = emitterCtx->createBlock();
    MirBlock *mergeBlock = emitterCtx->createBlock();
    MirBlock *nextItBlock = emitterCtx->createBlock();

    emitter->emitJMP(MirReference{ initializationBlock->getId() });

    emitterCtx->bindToBlock(initializationBlock);
    _for->getInitialization()->accept(ctx->getOwnerLowererVisitor());
    emitter->emitJMP(MirReference{ conditionBlock->getId() });

    emitterCtx->bindToBlock(conditionBlock);
    ctx->pushBlock(bodyBlock);
    ctx->pushBlock(mergeBlock);
    _for->getCondition()->accept(ctx->getOwnerLowererVisitor());

    ctx->enterLoop(LoopContext{ .m_breakTarget = mergeBlock, .m_continueTarget = nextItBlock });
    {
        emitterCtx->bindToBlock(bodyBlock);
        _for->getBody()->accept(ctx->getOwnerLowererVisitor());
        emitter->emitJMP(MirReference{ nextItBlock->getId() });
    }
    ctx->exitLoop();

    emitterCtx->bindToBlock(nextItBlock);
    _for->getNextItClause()->accept(ctx->getOwnerLowererVisitor());
    emitter->emitJMP(MirReference{ conditionBlock->getId() });

    emitterCtx->bindToBlock(mergeBlock);

    return true;
}
