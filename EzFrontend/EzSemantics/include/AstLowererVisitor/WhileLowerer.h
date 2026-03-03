#ifndef EZPACKER_WHILELOWERER_H
#define EZPACKER_WHILELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"
#include "IfLowerer.h"

class WhileLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given WhileAstNode node with the given lowering context. It will emit the condition code
     * in the CURRENT block. It will also create 2 blocks: true branch and false branch. If no false branch is given,
     * current block is used as fallthrough.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_WHILELOWERER_H
