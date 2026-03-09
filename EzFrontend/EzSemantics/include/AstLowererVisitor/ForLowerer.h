#ifndef EZPACKER_FORLOWERER_H
#define EZPACKER_FORLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"

class ForLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given ForAstNode node with the given lowering context. It will emit an initialization block,
     * a condition check block, a next iteration block and the body's block.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_FORLOWERER_H
