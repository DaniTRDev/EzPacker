/**
 * @file BreakLowerer.h
 * @brief Lowerer for the `break` statement — emits a JMP to the enclosing
 *        loop's exit block.
 *
 * After the JMP, a dead-code block is created and bound so that any
 * unreachable statements following the break do not corrupt the terminated
 * basic block.
 */
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
