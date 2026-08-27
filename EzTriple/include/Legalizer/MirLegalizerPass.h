#ifndef EZTRIPLE_MIR_LEGALIZER_PASS_H
#define EZTRIPLE_MIR_LEGALIZER_PASS_H

#include "EzTripleCommon.h"
#include "MirPasses/IMirTransformPass.h"

class MirBuilderContext;
class TargetDesc;

/**
 * Driver pass executing function signature legalization, call/return legalization,
 * and generic instruction legalization across all functions in the module.
 */
class MirLegalizerPass : public IMirTransformPass
{
  public:
    MirLegalizerPass(MirBuilderContext *ctx, TargetDesc *targetDesc);
    ~MirLegalizerPass() override = default;

    const char *getName() const override;
    MirPassIterationPlace getIterationPlace() const override;

    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

  private:
    MirBuilderContext *m_ctx;
    TargetDesc *m_targetDesc;
};

#endif // EZTRIPLE_MIR_LEGALIZER_PASS_H
