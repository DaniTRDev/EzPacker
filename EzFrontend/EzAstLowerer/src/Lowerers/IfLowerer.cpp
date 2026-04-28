#include "Lowerers/IfLowerer.h"
#include "AstLowererVisitor.h"

bool IfLowerer::lower(AstNode *node, AstLoweringContext *ctx)
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
        ctx->getEmitter()->emitJMP(
                ctx->getEmitterContext()->createReference(mergeBlock)); // Jump over the 'else' after executing the if.
    }

    // Lower the FALSE (Else) path
    ctx->getEmitterContext()->bindToBlock(falseBlock);
    if (ifNode->getFalseScope())
    {
        // This also handles (else-if)-(else) or (else) cases.
        ifNode->getFalseScope()->accept(ctx->getOwnerLowererVisitor());
    }

    // Continue execution to the next block after the if-else.
    ctx->getEmitter()->emitJMP(ctx->getEmitterContext()->createReference(mergeBlock));

    // Continue emitting the following code into the Merge Block
    return ctx->getEmitterContext()->bindToBlock(mergeBlock);
}
