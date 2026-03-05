/**
 * @file GenericLowerer.h
 * @brief Abstract interface for a single-node AST-to-MIR lowerer.
 *
 * Every language construct that needs MIR translation (module, label,
 * instruction, variable, immediate, memory, condition, if, while, break,
 * continue, code-scope) implements GenericLowerer::lower().  The
 * AstLowererVisitor dispatches each visited node to the appropriate
 * concrete lowerer.
 */
#ifndef EZPACKER_GENERICLOWERER_H
#define EZPACKER_GENERICLOWERER_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "LoweringContext.h"
#include "SemanticAnnotations/DataTypeAnnotation.h"
#include "SemanticAnnotations/ScopeAnnotation.h"
#include "SemanticAnnotations/ScopedSymbolAnnotation.h"
#include "SemanticAnnotations/SymbolAnnotation.h"
#include "SemanticAnnotations/TypeCastAnnotation.h"

/**
 * Interface used to define "something" from the AST that can be lowered into MIR.
 */
class GenericLowerer
{
  public:
    virtual ~GenericLowerer() = default;

    /**
     * Tries to lower the given AST node with the given lowering context.
     * @param ctx
     * @return bool
     */
    virtual bool lower(AstNode *node, LoweringContext *ctx) = 0;
};

#endif // EZPACKER_GENERICLOWERER_H
