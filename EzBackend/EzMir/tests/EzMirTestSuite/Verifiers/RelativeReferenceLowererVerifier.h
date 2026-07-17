#ifndef EZPACKER_RELATIVEREFERENCEVERIFIER_H
#define EZPACKER_RELATIVEREFERENCEVERIFIER_H

#include "MirCoreVerifiers.h"
#include "MirPasses/Passes/RelativeReferenceLowerer.h"

class RelativeReferenceLowererVerifier
    : public MirPassVerifier<RelativeReferenceLowerer, RelativeReferenceLowererVerifier>
{
  public:
    RelativeReferenceLowererVerifier(RelativeReferenceLowerer *pass, MirBuilderContext *ctx) :
        MirPassVerifier<RelativeReferenceLowerer, RelativeReferenceLowererVerifier>(pass), m_ctx(ctx)
    {
    }

    /**
     * Inspects a specific instruction in a function block and passes it to an evaluation callback block.
     * @param func
     * @param blockId
     * @param instrIndex
     */
    RelativeReferenceLowererVerifier &verifyInstruction(MirFunction *func,
                                                        size_t blockId,
                                                        size_t instrIndex,
                                                        std::function<void(MirInstructionVerifier &)> callback);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_RELATIVEREFERENCEVERIFIER_H