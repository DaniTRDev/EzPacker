#ifndef EZPACKER_FUNCTIONARGABILOWERERVERIFIER_H
#define EZPACKER_FUNCTIONARGABILOWERERVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class FunctionAbiLowererPass;

class FunctionArgAbiLowererVerifier : public MirPassVerifier<FunctionAbiLowererPass, FunctionArgAbiLowererVerifier>
{
  public:
    FunctionArgAbiLowererVerifier(MirBuilderContext *ctx, FunctionAbiLowererPass *pass);

    /**
     * Verifies that POP_ARG instructions in the entry block have been lowered into
     * physical register reads (MOVs), split memory stores, indirect pointer loads, or
     * stack parameter loads at the function entry point.
     *
     * @param entryBlock The entry block of the function being lowered.
     * @param origPopArgs The original virtual register operands expecting parameter data.
     * @return Reference to self for method chaining.
     */
    FunctionArgAbiLowererVerifier &verifyLoweredFunctionArguments(MirBlock *entryBlock,
                                                                  const std::vector<MirOperand *> &origPopArgs);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_FUNCTIONARGABILOWERERVERIFIER_H