#ifndef EZPACKER_LEGALIZERETURNACTIONVERIFIER_H
#define EZPACKER_LEGALIZERETURNACTIONVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class LegalizeReturnActionVerifier : public MirPassVerifier<MirBlockLegalizerPass, LegalizeReturnActionVerifier>
{
  public:
    /**
     * Creates the verifier with the given builder ctx and pass.
     * @param ctx
     * @param pass
     */
    LegalizeReturnActionVerifier(MirBuilderContext *ctx, MirBlockLegalizerPass *pass);

    /**
     * Verifies that the return operands are correctly legalized for the given instruction.
     * Account for direct returns, indirect (SRET) returns, and void returns.
     * @param retStartIt Iterator pointing to the start of the legalized sequence (STORE/PUSH_RET or RET)
     * @param origOperand The original return operand passed into RET before legalization
     * @return
     */
    LegalizeReturnActionVerifier &verifyRetPush(std::pmr::list<MirInstruction *>::iterator retStartIt,
                                                MirOperand *origOperand);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_LEGALIZERETURNACTIONVERIFIER_H