/**
 * @file ContinueLowerer.h
 * @brief Lowerer for the `continue` statement — emits a JMP back to the
 *        enclosing loop's condition-check block.
 *
 * After the JMP, a dead-code block is created and bound so that any
 * unreachable statements following the continue do not corrupt the
 * terminated basic block.
 */
#ifndef EZPACKER_CONTINUELOWERER_H
#define EZPACKER_CONTINUELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericAstLowerer.h"

class ContinueLowerer : public GenericAstLowerer
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
