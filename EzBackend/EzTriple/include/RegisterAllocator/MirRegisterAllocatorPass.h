#ifndef EZPACKER_REGISTERALLOCATORPASS_H
#define EZPACKER_REGISTERALLOCATORPASS_H

#include "EzTripleCommon.h"
#include "MirRegisterAllocator.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"

class MirRegisterAllocatorPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass linked to the given builder context, register allocator, and target descriptor.
     */
    MirRegisterAllocatorPass(MirBuilderContext *ctx, MirRegisterAllocator *regAllocator, TargetDesc *targetDesc);

    /**
     * Returns "RegisterAllocatorPass".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns the iteration place for this pass (Function).
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass on every function. It will start traversing blocks and instructions to properly allocate registers
     * and spills.
     * @param funcList
     * @param it
     * @param passManager
     */
    MirPassResult run(std::pmr::list<class MirFunction *> &funcList,
                      std::pmr::list<class MirFunction *>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Prints the pass result to the diag collector. In this case, it just prints the modified funcs.
     */
    void printResult() const override;

    /**
     * This pass depends on instruction selection and LivenessAnalysisPass.
     */
    std::vector<std::type_index> getDependencies() const override;

  private:
    MirBuilderContext *m_ctx;
    MirRegisterAllocator *m_regAllocator;
    TargetDesc *m_targetDesc;
    std::pmr::vector<MirFunction *> m_resolvedFunctions;
};

#endif // EZPACKER_REGISTERALLOCATORPASS_H
