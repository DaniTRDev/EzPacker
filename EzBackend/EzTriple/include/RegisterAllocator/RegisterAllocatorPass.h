#ifndef EZPACKER_REGISTERALLOCATORPASS_H
#define EZPACKER_REGISTERALLOCATORPASS_H

#include "EzTriple.h"

struct RegisterAllocatorResult
{
    // Map that links a virtual register to its assigned physical register.
    std::pmr::map<MirId, MirRegister *> m_allocatedRegisters;

    // Map that assigns a virtual register to its spilled memory location.
    std::pmr::map<MirId, StackFrameObject *> m_spilledRegisters;
};

class RegisterAllocatorPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass linked to the given builder context.
     */
    RegisterAllocatorPass(MirBuilderContext *ctx);

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
     * Prints the pass result to the diag collector. In this case, it just prints the modified blocks.
     */
    void printResult() const override;

    /**
     * This pass depends on instruction selection and LivenessAnalysis.
     */
    std::vector<std::type_index> getDependencies() const override;

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_REGISTERALLOCATORPASS_H
