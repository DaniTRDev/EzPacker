/**
 * @file WhileLowerer.h
 * @brief Lowerer for `while` loops — creates condition-check, body, and exit blocks.
 *
 * The WhileLowerer creates three basic blocks:
 *   1. Condition-check — re-evaluated every iteration; a conditional jump
 *      enters the body or falls through to the exit.
 *   2. Loop-body       — the statements inside the while braces.
 *   3. Exit            — the block after the loop.
 *
 * A LoopContext is pushed so that any break/continue statements inside the
 * body know which blocks to target.  After the body is lowered an
 * unconditional JMP returns control to the condition-check block.
 */
#ifndef EZPACKER_WHILELOWERER_H
#define EZPACKER_WHILELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"
#include "IfLowerer.h"

class WhileLowerer : public GenericLowerer
{
  public:
    /**
     * Lowers a WhileAstNode: creates condition-check, loop-body, and exit
     * blocks, pushes a LoopContext, lowers the condition and body, then
     * emits a back-edge JMP to the condition-check block.
     * @param node The WhileAstNode to lower.
     * @param ctx  The shared lowering context.
     * @return true on success, false on error.
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;
};

#endif // EZPACKER_WHILELOWERER_H
