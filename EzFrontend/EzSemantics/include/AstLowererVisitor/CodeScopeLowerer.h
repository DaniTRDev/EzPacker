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
#include "GenericLowerer.h"
#include "VariableLowerer.h"

class CodeScopeLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given CodeScope node with the given lowering context. It will lower both instructions
     * and expressions.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;

  private:
};

#endif // EZPACKER_CODESCOPELOWERER_H
