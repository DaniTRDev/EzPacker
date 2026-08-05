#ifndef EZPACKER_FUNCSIGNATUREPASSVERIFIER_H
#define EZPACKER_FUNCSIGNATUREPASSVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class FuncSignaturePassVerifier : public MirPassVerifier<MirFunctionSignatureLegalizerPass, FuncSignaturePassVerifier>
{
  public:
    /**
     * Creates the verifier with the given builder ctx and pass.
     * @param ctx
     * @param pass
     */
    FuncSignaturePassVerifier(MirBuilderContext *ctx, MirFunctionSignatureLegalizerPass *pass);

    /**
     * Sets the target func to the one given, any subsequent call to the verify methods will use this func.
     * @param func
     * @return
     */
    FuncSignaturePassVerifier &beginFunc(MirFunction *func);

    /**
     * Verifies that the given args were moved away from the function parameter list and were introduced in the
     * entry point as POP_ARG instructions.
     * @param originalArgs
     * @return
     */
    FuncSignaturePassVerifier &verifyArgs(std::vector<MirRegister *> originalArgs);

  private:
    MirFunction *m_targetFunc;
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_FUNCSIGNATUREPASSVERIFIER_H
