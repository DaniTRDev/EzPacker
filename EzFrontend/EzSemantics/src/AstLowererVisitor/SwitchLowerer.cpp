#include "AstLowererVisitor/SwitchLowerer.h"
#include "AstLowererVisitor/AstLowererVisitor.h"

bool SwitchLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    MirEmitter *emitter = ctx->getEmitter().get();
    MirEmitterContext *emitterCtx = ctx->getEmitterContext().get();

    SwitchAstNode *_switch = dynamic_cast<SwitchAstNode *>(node);

    // Only lower once.
    _switch->getSwitchVariable()->accept(ctx->getOwnerLowererVisitor());
    MirOperand switchVarOperand = ctx->popOperand();

    MirBlock *currentCheckerBlock = emitterCtx->createBlock();
    MirBlock *mergeBlock = emitterCtx->createBlock();

    // Jump from the current execution flow into our first checker block.
    emitter->emitJMP(MirReference{ currentCheckerBlock->getId() });
    ctx->enterSwitch(mergeBlock);
    {
        for (AstNode *switchCase : *_switch->getCases())
        {
            SwitchCaseAstNode *casted = dynamic_cast<SwitchCaseAstNode *>(switchCase);

            MirBlock *caseBodyBlock = emitterCtx->createBlock();
            MirBlock *nextCheckerBlock = emitterCtx->createBlock();

            emitterCtx->bindToBlock(currentCheckerBlock);

            // Get the immediate value for this case.
            casted->getCaseValue()->accept(ctx->getOwnerLowererVisitor());
            MirOperand caseValOperand = ctx->popOperand();

            emitter->emitCMP(switchVarOperand, caseValOperand);

            // If equal, jump to the body of this case.
            emitter->emitJE(MirReference{ caseBodyBlock->getId() });
            // If not equal, jump to the NEXT checker block.
            emitter->emitJMP(MirReference{ nextCheckerBlock->getId() });

            emitterCtx->bindToBlock(caseBodyBlock);
            casted->getBody()->accept(ctx->getOwnerLowererVisitor());

            // After the body finishes executing, jump to the end of the switch.
            emitter->emitJMP(MirReference{ mergeBlock->getId() });

            // Advance our checker block for the next loop iteration.
            currentCheckerBlock = nextCheckerBlock;
        }
    }
    ctx->exitSwitch();

    emitterCtx->bindToBlock(currentCheckerBlock);

    if (_switch->getDefault())
    {
        // Lower the default body directly into the final checker block.
        _switch->getDefault()->accept(ctx->getOwnerLowererVisitor());
    }

    // Whether we ran a default block or not, we must jump to the merge block.
    emitter->emitJMP(MirReference{ mergeBlock->getId() });

    // Bind to the merge block so subsequent AST nodes are placed after the switch.
    emitterCtx->bindToBlock(mergeBlock);

    return true;
}
