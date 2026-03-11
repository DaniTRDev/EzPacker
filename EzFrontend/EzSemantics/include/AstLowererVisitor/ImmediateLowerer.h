/**
 * @file ImmediateLowerer.h
 * @brief Lowerer for compile-time constant operands (integers, floats, strings).
 *
 * Small integers are emitted inline as MirInteger operands.  Strings and
 * arbitrary-precision big integers are emitted as global data entries
 * (via MirGlobalDataEmitter) and a MirReference to that entry is pushed
 * onto the operand stack.
 */
#ifndef EZPACKER_IMMEDIATELOWERER_H
#define EZPACKER_IMMEDIATELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericAstLowerer.h"

class ImmediateLowerer : public GenericAstLowerer
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
