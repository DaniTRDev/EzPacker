#ifndef EZMIR_RELATIVE_REFERENCE_LOWERER_H
#define EZMIR_RELATIVE_REFERENCE_LOWERER_H

#include "EzMirCommon.h"
#include "MirPasses/IMirTransformPass.h"

/**
 * This pass takes instructions with REFERENCE operands and lowers them into flat MEMORY
 * operands (MirMemory) using pointer arithmetic.
 *
 * It handles the following structural accesses:
 *
 *  - ClassField  (base: classPtr, displacement: fieldOffset)
 *      Resolved at compile-time. Becomes [base + constant offset].
 *
 *  - ClassMethod (base: classPtr, displacement: methodOffset)
 *      Resolved at compile-time. Becomes [base + constant offset].
 */
class RelativeReferenceLowererPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     */
    RelativeReferenceLowererPass(class MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "RelativeReferenceLowererPass".
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Instruction.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass in the given instruction and returns the result.
     */
    MirPassResult run(std::pmr::list<class MirInstruction *> &instrList,
                      std::pmr::list<class MirInstruction *>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * As the results are printed as TRACE, this does not print anything.
     */
    void printResult() override;

  private:
    class MirBuilderContext *m_ctx;
};

#endif // EZMIR_RELATIVE_REFERENCE_LOWERER_H