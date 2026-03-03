#ifndef EZPACKER_CONTINUELOWERER_H
#define EZPACKER_CONTINUELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"

class ContinueLowerer : public GenericLowerer
{
  public:
    /**
     * Lowers a continue statement by replacing it with a jump to the appropriate loop continuation point.
     * @param node
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_CONTINUELOWERER_H
