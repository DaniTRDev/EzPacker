#ifndef EZPACKER_SWITCHLOWERER_H
#define EZPACKER_SWITCHLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericAstLowerer.h"

class SwitchLowerer : public GenericAstLowerer
{
  public:
    /**
     * Tries to lower the given SwitchAstNode node with the given lowering context. It will emit the checks for
     * every case and link break instructions to the corresponding block..
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, AstLoweringContext *ctx) override;
};

#endif // EZPACKER_SWITCHLOWERER_H
