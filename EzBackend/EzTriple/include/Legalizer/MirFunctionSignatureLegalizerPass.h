#ifndef EZPACKER_MIRFUNCTIONSIGNATURELEGALIZERPASS_H
#define EZPACKER_MIRFUNCTIONSIGNATURELEGALIZERPASS_H

#include "EzTripleCommon.h"
#include "MirLegalizer.h"

/**
 * This pass will perform 2 actions:
 * - It will convert a function's parameters list into a set of POP_ARG instructions that can be affected by
 * promotion/expansion, ...
 *
 * - It will convert return instructions within the function into a sequence of SET_RET that will be consumed by
 * GET_RET.
 */
class MirFunctionSignatureLegalizerPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass linked to the given builder context and target desc.
     * @param ctx
     * @param legalizer
     */
    MirFunctionSignatureLegalizerPass(MirBuilderContext *ctx, TargetDesc *targetDesc);

    /**
     * Returns "MirFunctionSignatureLegalizerPass".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns the iteration place for this pass (Block).
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass on the given function.
     * @param funcList
     * @param it
     * @param passManager
     */
    MirPassResult run(std::pmr::list<class MirFunction *> &funcList,
                      std::pmr::list<class MirFunction *>::iterator it,
                      class MirPassManager *passManager) override;

  private:
    MirBuilderContext *m_ctx;
    MirLegalizer *m_legalizer;
    std::set<size_t> m_modifiedBlockSet; // Used to push a block exactly ONCE to the list.
    std::list<MirBlock *> m_modifiedBlocks;
};

#endif // EZPACKER_MIRFUNCTIONSIGNATURELEGALIZERPASS_H
