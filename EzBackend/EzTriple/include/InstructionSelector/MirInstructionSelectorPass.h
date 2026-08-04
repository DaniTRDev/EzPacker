#ifndef EZPACKER_MIRINSTRUCTIONSELECTORPASS_H
#define EZPACKER_MIRINSTRUCTIONSELECTORPASS_H

#include "EzTripleCommon.h"
#include "MirInstructionSelector.h"
#include "AbiLowerer/FunctionAbiLowererPass.h"
#include "Legalizer/MirBlockLegalizerPass.h"
#include "Legalizer/MirFunctionSignatureLegalizerPass.h"

class MirInstructionSelectorPass : public IMirTransformPass
{
  public:
    /**
     * Creates the selector pass with the given context selector.
     * @param ctx
     * @param selector
     */
    MirInstructionSelectorPass(MirBuilderContext *ctx, MirInstructionSelector *selector);

    /**
     * Returns "MirInstructionSelectorPass".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns the iteration place for this pass (Instruction).
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass on every instruction inside the given block. It will check if the given pair of instruction and
     * operands throw a match against the selector's rule table.
     * @param blockList
     * @param it
     * @param passManager
     */
    MirPassResult run(std::pmr::list<MirBlock *> &blockList,
                      std::pmr::list<MirBlock *>::iterator it,
                      MirPassManager *passManager) override;

    /**
     * This pass depends on: MirBlockLegalizerPass, MirFunctionSignatureLegalizerPass and FunctionAbiLowererPass.
     */
    std::vector<std::type_index> getDependencies() const override;

  private:
    MirBuilderContext *m_ctx;
    MirInstructionSelector *m_selector;
};

#endif // EZPACKER_MIRINSTRUCTIONSELECTORPASS_H
