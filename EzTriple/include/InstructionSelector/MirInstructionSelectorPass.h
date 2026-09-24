#ifndef EZTRIPLE_MIR_INSTRUCTION_SELECTOR_PASS_H
#define EZTRIPLE_MIR_INSTRUCTION_SELECTOR_PASS_H

#include "EzTripleCommon.h"
#include "MirPasses/IMirTransformPass.h"

class MirBuilderContext;
class TargetDesc;

/**
 * Driver pass executing target instruction selection across all functions in the module.
 */
class MirInstructionSelectorPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass bound to the active builder context and the target being compiled for.
     */
    MirInstructionSelectorPass(MirBuilderContext *ctx, TargetDesc *targetDesc);
    ~MirInstructionSelectorPass() override = default;

    /// Returns the stable pass name used by the pass manager for scheduling and diagnostics.
    const char *getName() const override;

    /// Declares where in the pass pipeline this pass should execute.
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Selects target instructions for every function referenced by the iterator.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

  private:
    MirBuilderContext *m_ctx; ///< Shared builder context used to create and mutate instructions.
    TargetDesc *m_targetDesc; ///< Target supplying the instruction selector implementation.
};

#endif // EZTRIPLE_MIR_INSTRUCTION_SELECTOR_PASS_H
