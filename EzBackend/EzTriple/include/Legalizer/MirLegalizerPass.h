#ifndef EZPACKER_MIRLEGALIZERPASS_H
#define EZPACKER_MIRLEGALIZERPASS_H

#include "EzTripleCommon.h"
#include "MirLegalizer.h"

class MirLegalizerPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass linked to the given legalizer and builder context.
     * @param ctx
     * @param legalizer
     */
    MirLegalizerPass(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * Returns "MirLegalizerPass".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns the iteration place for this pass (Block).
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass on every instruction inside the given block. It will check if the given pair of instruction and
     * operands throw a match against the legalizer rule table.
     * @param blockList
     * @param it
     * @param passManager
     */
    MirPassResult run(std::pmr::list<class MirBlock *> &blockList,
                      std::pmr::list<class MirBlock *>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Prints the pass result to the diag collector. In this case, it just prints the modified blocks.
     */
    void printResult() const override;

  private:
    MirBuilderContext *m_ctx;
    MirLegalizer *m_legalizer;
    std::list<MirBlock *> m_modifiedBlocks;
};

#endif // EZPACKER_MIRLEGALIZERPASS_H
