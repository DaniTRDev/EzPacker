#ifndef EZPACKER_CALLABILOWERERVERIFIER_H
#define EZPACKER_CALLABILOWERERVERIFIER_H

#include "AbiLowerer/AbiLowerer.h"
#include "../../../EzMirTestSuite/include/EzMirTestSuite.h"

class CallAbiLowererVerifier : public MirPassVerifier<FunctionAbiLowererPass, CallAbiLowererVerifier>
{
  public:
    CallAbiLowererVerifier(MirBuilderContext *ctx, FunctionAbiLowererPass *pass);

    /**
     * Verifies that the PUSH_ARG instructions and CALL site within targetBlock have been correctly
     * lowered into physical registers, memory loads, or stack parameter stores according to the ABI.
     *
     * @param targetBlock The block containing the CALL instruction.
     * @param origPushArgs The original argument values passed into PUSH_ARG before lowering.
     * @return Reference to self for method chaining.
     */
    CallAbiLowererVerifier &verifyLoweredCall(MirBlock *targetBlock, const std::vector<MirOperand *> &origPushArgs);

    /**
     * Verifies that after the call of the given target block, there is a POP_RET instructions that retrieves the return
     * value of the called function.
     * @param targetBlock
     * @param origRet
     */
    CallAbiLowererVerifier &verifyLoweredCallReturn(MirBlock *targetBlock, MirOperand *origRet);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_CALLABILOWERERVERIFIER_H