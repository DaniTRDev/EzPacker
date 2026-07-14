#ifndef EZPACKER_LEGALIZECALLACTIONVERIFIER_H
#define EZPACKER_LEGALIZECALLACTIONVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class LegalizeCallActionVerifier : public MirPassVerifier<MirBlockLegalizerPass, LegalizeCallActionVerifier>
{
  public:
    /**
     * Creates the verifier with the given builder ctx and pass.
     * @param ctx
     * @param pass
     */
    LegalizeCallActionVerifier(MirBuilderContext *ctx, MirBlockLegalizerPass *pass);

    /**
     * Verifies that the call arguments are correctly legalized for the given call instruction and that the correct
     * POP_RET is generated and bound particularly to this call.
     * @param argStartIt
     * @param origOperands
     * @param expectedDest
     * @return
     */
    LegalizeCallActionVerifier &verifyCallSequence(std::pmr::list<MirInstruction *>::iterator startIt,
                                                   const std::vector<MirOperand *> &origOperands,
                                                   MirOperand *expectedDest);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_LEGALIZECALLACTIONVERIFIER_H
