#include "Lowerers/ConditionLowerer.h"
#include "AstLowererVisitor.h"

bool ConditionLowerer::lower(AstNode *node, AstLoweringContext *ctx)
{
    ConditionAstNode *cond = dynamic_cast<ConditionAstNode *>(node);

    if (!cond->getLeft()->accept(ctx->getOwnerLowererVisitor()) ||
        !cond->getRight()->accept(ctx->getOwnerLowererVisitor()))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "Could not lower condition nodes of ConditionAstNode",
                                             "ConditionLowerer");
        return false;
    }

    if (!ctx->hasBlocks())
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "There is no 'false-branch' block to jump to",
                                             "ConditionLowerer");
        return false;
    }

    MirOperand right = ctx->popOperand().first;

    if (!ctx->hasBlocks())
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "There is no 'true-branch' block to jump to",
                                             "ConditionLowerer");
        return false;
    }

    MirOperand left = ctx->popOperand().first;

    // Emit CMP into the CURRENT block
    ctx->getEmitter()->emitCMP(left, right);

    // Grab jump targets provided by the caller. FIFO order (false is first and true is after).
    MirReference falseTarget = ctx->getEmitterContext()->createReference(ctx->popBlock());
    MirReference trueTarget = ctx->getEmitterContext()->createReference(ctx->popBlock());
    
    // Emit the Conditional Jump to the True Block
    switch (cond->getComparisonType())
    {
        case ConditionComparisonType::Equal:
        {
            ctx->getEmitter()->emitJE(trueTarget);
            break;
        }
        case ConditionComparisonType::GreaterThan:
        {
            ctx->getEmitter()->emitJG(trueTarget);
            break;
        }
        case ConditionComparisonType::GreaterThanOrEqual:
        {
            ctx->getEmitter()->emitJGE(trueTarget);
            break;
        }
        case ConditionComparisonType::LessThan:
        {
            ctx->getEmitter()->emitJL(trueTarget);
            break;
        }
        case ConditionComparisonType::LessThanOrEqual:
        {
            ctx->getEmitter()->emitJLE(trueTarget);
            break;
        }
        case ConditionComparisonType::NotEqual:
        {
            ctx->getEmitter()->emitJNE(trueTarget);
            break;
        }
        default:
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Unknown ConditionComparisonType for condition node",
                                                 "ConditionLowerer");
            return false;
        }
    }

    // Emit the jump to the false target (fallthrough).
    ctx->getEmitter()->emitJMP(falseTarget);

    return true;
}
