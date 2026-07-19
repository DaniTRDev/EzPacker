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

    /**
     * Verifies that an indirect Struct Return (SRET) transformation was correctly legalized.
     * Asserts that:
     *  1. An ALLOC instruction initializes the stack space destination variable.
     *  2. A PUSH_ARG sequence pushes the implicit allocation pointer address as the first argument.
     *  3. The normal high-level user arguments follow sequentially.
     *  4. The truncated CALL instruction binds onto a proper call token.
     *
     * @param startIt          Iterator pointing to the beginning of the expected call sequence layout blocks.
     * @param origOperands     The original pre-legalized call vector slice elements: [DestReg, Callee, UserArgs...]
     * @return Reference to self for method-chaining verification blocks.
     */
    LegalizeCallActionVerifier &verifySretCallSequence(std::pmr::list<MirInstruction *>::iterator startIt,
                                                       const std::vector<MirOperand *> &origOperands);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_LEGALIZECALLACTIONVERIFIER_H
