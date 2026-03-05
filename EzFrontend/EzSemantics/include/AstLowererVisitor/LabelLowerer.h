/**
 * @file LabelLowerer.h
 * @brief Lowerer for named labels — creates a new basic block and links
 *        the label's symbol to its ID.
 *
 * After creating and binding the new block, the label's body scope is
 * lowered into it by delegating to the AstLowererVisitor.
 */
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
