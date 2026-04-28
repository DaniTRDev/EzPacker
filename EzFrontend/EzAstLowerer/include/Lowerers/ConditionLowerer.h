/**
 * @file ConditionLowerer.h
 * @brief Lowerer for binary comparison conditions (`%a EQ %b`, etc.).
 *
 * Emits a CMP instruction for the two operands, followed by a conditional
 * jump to the true-branch block and an unconditional JMP to the false-branch
 * block.  Expects two target blocks on the AstLoweringContext block stack
 * (pushed by the caller, typically IfLowerer or WhileLowerer).
 */
#ifndef EZPACKER_CONDITIONLOWERER_H
#define EZPACKER_CONDITIONLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericAstLowerer.h"

class ConditionLowerer : public GenericAstLowerer
{
  public:
    /**
     * Tries to lower the given ConditionAstNode, emitting the appropriate compare, jump instruction to enter (or not)
     * the "true-branch" block and a JMP to the "false-branch block". This requires that ctx has at least 2 blocks
     * available. If there are not enough blocks, false is returned.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, AstLoweringContext *ctx) override;
};

#endif // EZPACKER_CONDITIONLOWERER_H
