#ifndef EZPACKER_LABELLOWERER_H
#define EZPACKER_LABELLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"

class LabelLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given LabelAstNode node with the given lowering context. It will create a block and lower
     * its sub-expressions.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_LABELLOWERER_H
