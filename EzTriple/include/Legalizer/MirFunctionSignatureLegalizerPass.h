#ifndef EZTRIPLE_MIR_FUNCTION_SIGNATURE_LEGALIZER_PASS_H
#define EZTRIPLE_MIR_FUNCTION_SIGNATURE_LEGALIZER_PASS_H

#include "EzTripleCommon.h"
#include "MirPasses/IMirTransformPass.h"

class MirBuilderContext;
class TargetDesc;

/**
 * Transformation pass that legalizes function parameter lists and entry signatures before ABI lowering.
 * Prepend hidden SRET pointers for structs that cannot be returned in registers, and synthesizes
 * token-bound POP_ARG and END_ARG sequences at the function entry point.
 */
class MirFunctionSignatureLegalizerPass : public IMirTransformPass
{
  public:
    MirFunctionSignatureLegalizerPass(MirBuilderContext *ctx, TargetDesc *targetDesc);
    ~MirFunctionSignatureLegalizerPass() override = default;

    const char *getName() const override;
    MirPassIterationPlace getIterationPlace() const override;

    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

  private:
    MirBuilderContext *m_ctx; ///< Shared builder context used to synthesize the prologue.
};

#endif // EZTRIPLE_MIR_FUNCTION_SIGNATURE_LEGALIZER_PASS_H
