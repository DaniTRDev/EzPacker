#ifndef EZPACKER_IMMEDIATELOWERER_H
#define EZPACKER_IMMEDIATELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"

class ImmediateLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given ImmediateOperand node with the given lowering context. It will emit the immediate data
     * or a reference to it depending on the type. Strings and BigInts will be emitted as a GLOBAL variables and a
     * reference will be pushed to the operand stack.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_IMMEDIATELOWERER_H
