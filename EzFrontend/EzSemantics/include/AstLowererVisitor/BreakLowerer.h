#ifndef EZPACKER_BREAKLOWERER_H
#define EZPACKER_BREAKLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"

class BreakLowerer : public GenericLowerer
{
  public:
    /**
     * Lowers a break statement by replacing it with a jump to the appropriate loop exit point.
     * @param node
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_BREAKLOWERER_H
