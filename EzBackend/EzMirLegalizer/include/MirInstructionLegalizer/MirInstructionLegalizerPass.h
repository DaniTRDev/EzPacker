#ifndef EZPACKER_MIRINSTRUCTIONLEGALIZER_H
#define EZPACKER_MIRINSTRUCTIONLEGALIZER_H

#include "EzMirLegalizerCommon.h"
#include "MirLegalizerContext.h"

class MirInstructionLegalizer : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     * @param ctx
     */
    MirInstructionLegalizer(MirLegalizerContext *ctx);

    /**
     * Executes the type legalization pass on a single function.
     * Returns true if the MIR was modified (requires invalidating analysis passes).
     *
     * This function will iterate the list of blocks owned by the function and will try to legalize given instructions.
     */
    bool run(MirFunction *func, class MirPassManager *passManager) override;

  private:
    /**
     * Runs the pass in the given block.
     * @param block
     * @return
     */
    bool runOnBlock(MirBlock *block);

    /**
     * Runs the legalizer in the given instruction. If it was modified, true will be returned.
     * @param instrIt
     * @param parentBlock
     * @return
     */
    bool runOnInstruction(TypedPoolSlice<MirInstruction>::Iterator instrIt, MirBlock *parentBlock);

  private:
    MirLegalizerContext *m_ctx;
};

#endif // EZPACKER_MIRINSTRUCTIONLEGALIZER_H
