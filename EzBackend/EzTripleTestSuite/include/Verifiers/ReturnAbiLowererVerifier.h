#ifndef EZPACKER_RETURNABILOWERERVERIFIER_H
#define EZPACKER_RETURNABILOWERERVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class ReturnAbiLowererVerifier : public MirPassVerifier<FunctionAbiLowererPass, ReturnAbiLowererVerifier>
{
  public:
    /**
     * Creates the verifier with the given builder context and pass pointer.
     * @param ctx
     * @param pass
     */
    ReturnAbiLowererVerifier(MirBuilderContext *ctx, FunctionAbiLowererPass *pass);

    /**
     * Verifies that the return path at retIt was correctly lowered into ABI-compliant physical boundaries.
     * @param targetBlock The block containing the RET instruction being verified.
     * @param origValues The original return values expected to be lowered (empty for void).
     * @return
     */
    ReturnAbiLowererVerifier &verifyLoweredReturn(MirBlock *targetBlock, const std::vector<MirOperand *> &origValues);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_RETURNABILOWERERVERIFIER_H