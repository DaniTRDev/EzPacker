#ifndef EZPACKER_REGISTERALLOCATORPASS_H
#define EZPACKER_REGISTERALLOCATORPASS_H

#include "EzTripleCommon.h"
#include "MirPasses/IMirTransformPass.h"

/**
 * Result data container holding per-function allocator contexts and resolved status.
 */
struct MirRegisterAllocatorPassResult
{
    std::pmr::unordered_map<class MirFunction *, class RegisterAllocatorCtx *>
            m_contexts;                                               ///< Per-function allocator state.
    std::pmr::unordered_set<class MirFunction *> m_resolvedFunctions; ///< Functions that finished allocation.

    MirRegisterAllocatorPassResult(std::pmr::memory_resource *alloc) : m_contexts(alloc), m_resolvedFunctions(alloc) {}
};

/**
 * Transformation pass driving graph-coloring register allocation for every function in the module.
 */
class MirRegisterAllocatorPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass linked to the given builder context and target descriptor.
     */
    MirRegisterAllocatorPass(class MirBuilderContext *ctx, class TargetDesc *targetDesc);

    /**
     * Returns "RegisterAllocatorPass".
     */
    const char *getName() const override;

    /**
     * Returns the iteration place for this pass (Function).
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass on every function. It will start traversing blocks and instructions to properly allocate registers
     * and spills.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Returns the result structure of this pass.
     */
    const MirRegisterAllocatorPassResult &getResult() const;

    /**
     * Prints the pass result to the diag collector. In this case, it just prints the modified funcs.
     */
    void printResult() override;

    /**
     * Clears the resolved function list and the allocator context.
     */
    void reset() override;

    /**
     * This pass depends on instruction selection and LivenessAnalysisPass.
     */
    std::vector<std::type_index> getDependencies() const override;

  private:
    class MirBuilderContext *m_ctx;             ///< Shared builder context used to emit spill/reload code.
    class MirRegisterAllocator *m_regAllocator; ///< Concrete target allocator implementing the coloring algorithm.
    MirRegisterAllocatorPassResult m_result;    ///< Accumulated per-function allocation results.
    class TargetDesc *m_targetDesc;             ///< Target supplying the allocator and register classes.
};

#endif // EZPACKER_REGISTERALLOCATORPASS_H
