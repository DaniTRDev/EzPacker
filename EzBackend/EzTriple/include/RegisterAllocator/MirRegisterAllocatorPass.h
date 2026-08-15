#ifndef EZPACKER_REGISTERALLOCATORPASS_H
#define EZPACKER_REGISTERALLOCATORPASS_H

#include "EzTripleCommon.h"
#include "MirRegisterAllocator.h"
#include "InstructionSelector/MirInstructionSelectorPass.h"

struct MirRegisterAllocatorPassResult
{
    std::pmr::unordered_map<MirFunction *, RegisterAllocatorCtx *> m_contexts;
    std::pmr::unordered_set<MirFunction *> m_resolvedFunctions;

    MirRegisterAllocatorPassResult(std::pmr::memory_resource *alloc) : m_contexts(alloc), m_resolvedFunctions(alloc) {}
};

class MirRegisterAllocatorPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass linked to the given builder context and target descriptor.
     */
    MirRegisterAllocatorPass(MirBuilderContext *ctx, TargetDesc *targetDesc);

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
     * Returns the result structure of this pass.
     */
    const MirRegisterAllocatorPassResult &getResult() const;

    /**
     * Prints the pass result to the diag collector. In this case, it just prints the modified funcs.
     */
    void printResult() const override;

    /**
     * Clears the resolved function list and the allocator context.
     */
    void reset() override;

    /**
     * This pass depends on instruction selection and LivenessAnalysisPass.
     */
    std::vector<std::type_index> getDependencies() const override;

  private:
    MirBuilderContext *m_ctx;
    MirRegisterAllocator *m_regAllocator;
    MirRegisterAllocatorPassResult m_result;
    TargetDesc *m_targetDesc;
};

#endif // EZPACKER_REGISTERALLOCATORPASS_H
