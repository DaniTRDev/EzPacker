#ifndef EZPACKER_CONDITIONLOWERER_H
#define EZPACKER_CONDITIONLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"

class ConditionLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given ConditionAstNode, emitting the appropriate compare, jump instruction to enter (or not)
     * the "true-branch" block and a JMP to the "false-branch block". This requires that ctx has at least 2 blocks
     * available. If there are not enough blocks, false is returned.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_CONDITIONLOWERER_H
