#ifndef EZMIR_RELATIVE_REFERENCE_LOWERER_H
#define EZMIR_RELATIVE_REFERENCE_LOWERER_H

#include "EzMirCommon.h"
#include "MirPasses/IMirTransformPass.h"

/**
 * This pass takes instructions with REFERENCE operands that can be lowered as RELATIVE-TO-POINTER memory operands
 * using pointer arithmetic.
 *
 * This is the case of:
 *  - ClassField (base: classPtr, off: fieldOffset)
 *  - ClassMethod (base: classPtr, off: methodOffset)
 *  - ConstantArrayElement (base, arrayPtr, off: elemIndex * elemSize)
 *
 * Theses cases CAN'T be lowered by this pass because they require information about the final code format and will
 * result in RELOCATIONS.
 *  - Block (needs the address of the block)
 *  - GlobalArrayElem (needs the address of the global variable)
 *  - GlobalVar (needs the address of the global variable)
 *  - Function (needs the address of the function)
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
     * @return
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Instruction.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass in the given instruction and returns the result.
     * @param instrList
     * @param it
     * @param passManager
     * @return
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