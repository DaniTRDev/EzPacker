#ifndef EZPACKER_RETURNABILOWERERPASS_H
#define EZPACKER_RETURNABILOWERERPASS_H

#include "EzTripleCommon.h"

/**
 * This pass lowers the return chain (PUSH_RET/RET) into phyiscal places using target's ABI and calling convention.
 */
class ReturnAbiLowerer : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     */
    ReturnAbiLowerer(MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "ReturnAbiLowererPass".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Function.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass in the given function and returns the result.
     * @param funcList
     * @param it
     * @param passManager
     * @return
     */
    MirPassResult run(std::pmr::list<MirFunction *> &funcList,
                      std::pmr::list<MirFunction *>::iterator it,
                      MirPassManager *passManager) override;

    /**
     * Prints nothing, as the results are shown as traces.
     */
    void printResult() const override;

  private:
    /**
     * Process the given block of PUSH_RET instructions and modifies it to follow CallingConvention's guidelines.
     * @param cc
     * @param targetBlock
     * @param func
     * @param retType
     * @param it
     * @param retBlock
     */
    bool processReturnBlock(CallingConvDesc *cc,
                            MirBlock *targetBlock,
                            MirFunction *func,
                            MirType *retType,
                            std::pmr::list<MirInstruction *>::iterator it,
                            std::pmr::vector<MirInstruction *> &retBlock);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_RETURNABILOWERERPASS_H
