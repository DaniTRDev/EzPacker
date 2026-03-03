#include "AstLowererVisitor/WhileLowerer.h"
#include "AstLowererVisitor/AstLowererVisitor.h"

bool WhileLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    WhileAstNode *whileNode = dynamic_cast<WhileAstNode *>(node);

    MirBlock *checkCondition = ctx->getEmitterContext()->createBlock();
    MirBlock *trueBlock = ctx->getEmitterContext()->createBlock();  // Loop
    MirBlock *falseBlock = ctx->getEmitterContext()->createBlock(); // Block after loop.

    // Tell the ConditionLowerer where to jump
    ctx->pushBlock(trueBlock);
    ctx->pushBlock(falseBlock);

    // Jump to the block that checks the condition.
    ctx->getEmitter()->emitJMP(MirReference{ checkCondition->getId() });

    // Lower the Condition (it will emit CMP, JXX trueBlock, JMP falseBlock)
    ctx->getEmitterContext()->bindToBlock(checkCondition);
    whileNode->getCondition()->accept(ctx->getOwnerLowererVisitor());

    ctx->enterLoop(LoopContext{ .m_breakTarget = falseBlock, .m_continueTarget = checkCondition });
    {
        // Lower the loop path.
        ctx->getEmitterContext()->bindToBlock(trueBlock);
        {
            whileNode->getCodeScope()->accept(ctx->getOwnerLowererVisitor());
            ctx->getEmitter()->emitJMP(MirReference{ checkCondition->getId() });
        }
    }
    ctx->exitLoop();

    return ctx->getEmitterContext()->bindToBlock(falseBlock);
}
