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
