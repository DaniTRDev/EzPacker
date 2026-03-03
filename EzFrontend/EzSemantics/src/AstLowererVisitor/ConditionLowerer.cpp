#include "AstLowererVisitor/ConditionLowerer.h"
#include "AstLowererVisitor/AstLowererVisitor.h"

bool ConditionLowerer::lower(AstNode *node, LoweringContext *ctx)
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

    MirOperand right = ctx->popOperand();

    if (!ctx->hasBlocks())
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "There is no 'true-branch' block to jump to",
                                             "ConditionLowerer");
        return false;
    }

    MirOperand left = ctx->popOperand();

    // Emit CMP into the CURRENT block
    ctx->getEmitter()->emitCMP(left, right);

    // Grab jump targets provided by the caller. FIFO order (false is first and true is after).
    MirId falseTarget = ctx->popBlock()->getId();
    MirId trueTarget = ctx->popBlock()->getId();

    // Emit the Conditional Jump to the True Block
    switch (cond->getComparisonType())
    {
        case ConditionComparisonType::Equal:
        {
            ctx->getEmitter()->emitJE(MirReference{ trueTarget });
            break;
        }
        case ConditionComparisonType::GreaterThan:
        {
            ctx->getEmitter()->emitJG(MirReference{ trueTarget });
            break;
        }
        case ConditionComparisonType::GreaterThanOrEqual:
        {
            ctx->getEmitter()->emitJGE(MirReference{ trueTarget });
            break;
        }
        case ConditionComparisonType::LessThan:
        {
            ctx->getEmitter()->emitJL(MirReference{ trueTarget });
            break;
        }
        case ConditionComparisonType::LessThanOrEqual:
        {
            ctx->getEmitter()->emitJLE(MirReference{ trueTarget });
            break;
        }
        case ConditionComparisonType::NotEqual:
        {
            ctx->getEmitter()->emitJNE(MirReference{ trueTarget });
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
    ctx->getEmitter()->emitJMP(MirReference{ falseTarget });

    return true;
}
