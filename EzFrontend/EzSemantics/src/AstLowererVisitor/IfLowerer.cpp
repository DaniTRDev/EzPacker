#include "AstLowererVisitor/IfLowerer.h"
#include "AstLowererVisitor/AstLowererVisitor.h"

bool IfLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    IfAstNode *ifNode = dynamic_cast<IfAstNode *>(node);

    MirBlock *trueBlock = ctx->getEmitterContext()->createBlock();
    MirBlock *falseBlock = ctx->getEmitterContext()->createBlock();
    MirBlock *mergeBlock = ctx->getEmitterContext()->createBlock(); // Code after the if/else

    // Tell the ConditionLowerer where to jump
    ctx->pushBlock(trueBlock);
    ctx->pushBlock(falseBlock);

    // Lower the Condition (it will emit CMP, JXX trueBlock, JMP falseBlock)
    ifNode->getCondition()->accept(ctx->getOwnerLowererVisitor());

    // Lower the TRUE path
    ctx->getEmitterContext()->bindToBlock(trueBlock);
    {
        ifNode->getTrueScope()->accept(ctx->getOwnerLowererVisitor());
        ctx->getEmitter()->emitJMP(MirReference{ mergeBlock->getId() }); // Jump over the 'else' after executing the if.
    }

    // Lower the FALSE (Else) path
    ctx->getEmitterContext()->bindToBlock(falseBlock);
    if (ifNode->getFalseScope())
    {
        // This also handles (else-if)-(else) or (else) cases.
        ifNode->getFalseScope()->accept(ctx->getOwnerLowererVisitor());
    }

    // Continue execution to the next block after the if-else.
    ctx->getEmitter()->emitJMP(MirReference{ mergeBlock->getId() });

    // Continue emitting the following code into the Merge Block
    return ctx->getEmitterContext()->bindToBlock(mergeBlock);
}
