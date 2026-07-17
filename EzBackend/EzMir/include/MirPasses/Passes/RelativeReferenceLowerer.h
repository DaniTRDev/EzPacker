#ifndef EZPACKER_RELATIVEREFERENCELOWERER_H
#define EZPACKER_RELATIVEREFERENCELOWERER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilderContext.h"
#include "MirPasses/IMirTransformPass.h"
#include "Operand/MirOperandBuilder.h"
#include "Printer/MirPrinter.h"

/**
 * This pass takes instructions with REFERENCE operands that can be lowered as RELATIVE-TO-POINTER memory operands
 * using pointer arithmetic.
 *
 * This is the case of:
 *  - ClassField (base: classPtr, off: fieldOffset)
 *  - ClassMethod (base: classPtr, off: methodOffset)
 *  - ConstantArrayElement (base, arrayPtr, off: elemIndex * elemSize)
 *
 * Theses cases CAN'T be lowered by this pass because they require information about the final code format:
 *  - Block (needs the address of the block)
 *  - GlobalArrayElem (needs the address of the global variable)
 *  - GlobalVar (needs the address of the global variable)
 *  - Function (needs the address of the function)
 */
class RelativeReferenceLowerer : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     */
    RelativeReferenceLowerer(MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "RelativeReferenceLowerer".
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
    MirPassResult run(std::pmr::list<MirInstruction *> &instrList,
                      std::pmr::list<MirInstruction *>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * As the results are printed as TRACE, this does not print anything.
     */
    void printResult() const override;

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_RELATIVEREFERENCELOWERER_H