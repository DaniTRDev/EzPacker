#include "Lowerers/WhileLowerer.h"
#include "AstLowererVisitor.h"

bool WhileLowerer::lower(AstNode *node, AstLoweringContext *ctx)
{
    WhileAstNode *whileNode = dynamic_cast<WhileAstNode *>(node);

    MirBlock *checkCondition = ctx->getEmitterContext()->createBlock();
    MirBlock *trueBlock = ctx->getEmitterContext()->createBlock();  // Loop
    MirBlock *falseBlock = ctx->getEmitterContext()->createBlock(); // Block after loop.

    // Tell the ConditionLowerer where to jump
    ctx->pushBlock(trueBlock);
    ctx->pushBlock(falseBlock);

    // Jump to the block that checks the condition.
    ctx->getEmitter()->emitJMP(ctx->getEmitterContext()->createReference(checkCondition));

    // Lower the Condition (it will emit CMP, JXX trueBlock, JMP falseBlock)
    ctx->getEmitterContext()->bindToBlock(checkCondition);
    whileNode->getCondition()->accept(ctx->getOwnerLowererVisitor());

    ctx->enterLoop(LoopContext{ .m_breakTarget = falseBlock, .m_continueTarget = checkCondition });
    {
        // Lower the loop path.
        ctx->getEmitterContext()->bindToBlock(trueBlock);
        {
            whileNode->getCodeScope()->accept(ctx->getOwnerLowererVisitor());
            ctx->getEmitter()->emitJMP(ctx->getEmitterContext()->createReference(checkCondition));
        }
    }
    ctx->exitLoop();

    return ctx->getEmitterContext()->bindToBlock(falseBlock);
}
