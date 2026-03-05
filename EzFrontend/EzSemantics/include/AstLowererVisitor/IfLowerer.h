/**
 * @file IfLowerer.h
 * @brief Lowerer for `if / else if / else` control-flow blocks.
 *
 * Creates true-branch, false-branch, and merge blocks.  The condition is
 * lowered in the current block (via ConditionLowerer); then both branches
 * are lowered into their respective blocks.  All paths converge at the
 * merge block which becomes the new "current" block after lowering.
 */
#ifndef EZPACKER_IFLOWERER_H
#define EZPACKER_IFLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"

class IfLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given IfAstNode node with the given lowering context. It will emit the condition code
     * in the CURRENT block. It will also create 2 blocks: true branch and false branch. If no false branch is given,
     * current block is used as fallthrough.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_IFLOWERER_H
