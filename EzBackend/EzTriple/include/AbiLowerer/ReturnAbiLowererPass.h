#ifndef EZPACKER_RETURNABILOWERERPASS_H
#define EZPACKER_RETURNABILOWERERPASS_H

#include "EzTripleCommon.h"
#include "Descriptors/ABIDesc.h"

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
     * Returns MirPassIterationPlace::Block.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass in the given block and returns the result.
     * @param instrList
     * @param it
     * @param passManager
     * @return
     */
    MirPassResult run(std::pmr::list<MirBlock *> &funcList,
                      std::pmr::list<MirBlock *>::iterator it,
                      MirPassManager *passManager) override;

    /**
     * Prints nothing, as the results are shown as traces.
     */
    void printResult() const override;

  private:
};

#endif // EZPACKER_RETURNABILOWERERPASS_H
