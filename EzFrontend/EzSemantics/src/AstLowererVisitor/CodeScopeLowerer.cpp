#include "AstLowererVisitor/CodeScopeLowerer.h"
#include "AstLowererVisitor/AstLowererVisitor.h" // Defined here not to create a cyclic dependency.

bool CodeScopeLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    CodeScope *scope = dynamic_cast<CodeScope *>(node);
    for (AstNode *expr : *scope->getExpressions())
    {
        if (!expr->accept(ctx->getOwnerLowererVisitor()))
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Failed to lower code scope",
                                                 "CodeScopeLowerer");
            return false;
        }
    }

    return true;
}
