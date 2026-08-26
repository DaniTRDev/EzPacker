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
    MirInstructionSelectorPass(MirBuilderContext *ctx, TargetDesc *targetDesc);
    ~MirInstructionSelectorPass() override = default;

    const char *getName() const override;
    MirPassIterationPlace getIterationPlace() const override;

    MirPassResult run(IntrusiveLinkedList<class MirFunction> &funcList,
                      IntrusiveLinkedList<class MirFunction>::iterator it,
                      class MirPassManager *passManager) override;

  private:
    MirBuilderContext *m_ctx;
    TargetDesc *m_targetDesc;
};

#endif // EZTRIPLE_MIR_INSTRUCTION_SELECTOR_PASS_H
