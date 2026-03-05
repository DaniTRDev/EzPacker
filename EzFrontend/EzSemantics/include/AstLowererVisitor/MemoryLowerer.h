/**
 * @file MemoryLowerer.h
 * @brief Lowerer for memory-addressing operands (base+disp, index*scale, etc.).
 *
 * Translates each MemoryOperandAstNode variant into a MirMemory operand
 * by resolving the base/index variables to their MIR register IDs,
 * reading the displacement and scale factor, and pushing the resulting
 * MirOperand onto the operand stack.  Each addressing mode
 * (BaseDisplacement, IndexScale, BaseIndexScaleDisplacement) has a
 * dedicated private helper.
 */
#ifndef EZPACKER_MEMORYLOWERER_H
#define EZPACKER_MEMORYLOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericLowerer.h"
#include "VariableLowerer.h"

class MemoryLowerer : public GenericLowerer
{
  public:
    /**
     * Tries to lower the given MemoryOperand AST node with the given lowering context.
     * @param ctx
     * @return bool
     */
    bool lower(AstNode *node, LoweringContext *ctx) override;

  private:
    /**
     * Lowers the given BaseDisplacementMemory AST node with the given lowering context. This method is responsible for
     * translating the BaseDisplacementMemory node into the appropriate MIR representation, handling any necessary
     * calculations for the base register and displacement value.
     * @param node
     * @param ctx
     * @return bool
     */
    bool lowerBaseDisplacement(BaseDisplacementMemory *node, LoweringContext *ctx);

    /**
     * Lowers the given BaseIndexScaleDisplacementMemory AST node with the given lowering context. This method is
     * responsible for translating the BaseIndexScaleDisplacementMemory node into the appropriate MIR representation,
     * handling any necessary calculations for the base register, index register, scale factor, and displacement value.
     * @param node
     * @param ctx
     * @return
     */
    bool lowerBaseIndexScaleDisplacement(BaseIndexScaleDisplacementMemory *node, LoweringContext *ctx);

    /**
     * Lowers the given IndexScaleMemory AST node with the given lowering context. This method is responsible for
     * translating the IndexScaleMemory node into the appropriate MIR representation, handling any necessary
     * calculations for the index register and scale factor.
     * @param node
     * @param ctx
     * @return
     */
    bool lowerIndexScale(IndexScaleMemory *node, LoweringContext *ctx);

    /**
     * Lowers the given DirectMemory AST node with the given lowering context. This method is responsible for
     * translating the DirectMemory node into the appropriate MIR representation, handling any necessary
     * calculations for the memory address.
     * @param node
     * @param ctx
     * @return bool
     */
    bool lowerDirect(DirectMemory *node, LoweringContext *ctx);
};

#endif // EZPACKER_MEMORYLOWERER_H
