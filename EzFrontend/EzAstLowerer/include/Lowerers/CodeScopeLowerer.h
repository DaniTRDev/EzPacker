/**
 * @file CodeScopeLowerer.h
 * @brief Lowerer for brace-delimited code scopes (`{ … }`).
 *
 * Iterates through every child expression in the CodeScope and delegates
 * each one to the AstLowererVisitor, preserving statement order.
 */
#ifndef EZPACKER_CODESCOPELOWERER_H
#define EZPACKER_CODESCOPELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericAstLowerer.h"
#include "VariableLowerer.h"

class CodeScopeLowerer : public GenericAstLowerer
{
  public:
    /**
     * Tries to lower the given CodeScope node with the given lowering context. It will lower both instructions
     * and expressions.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, AstLoweringContext *ctx) override;

  private:
};

#endif // EZPACKER_CODESCOPELOWERER_H
