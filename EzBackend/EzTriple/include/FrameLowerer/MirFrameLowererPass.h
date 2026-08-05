#ifndef EZPACKER_MIRFRAMELOWERERPASS_H
#define EZPACKER_MIRFRAMELOWERERPASS_H

#include "EzTripleCommon.h"
#include "MirFrameLowerer.h"
#include "RegisterAllocator/MirRegisterAllocatorPass.h"

struct MirFrameLowererPassResult
{
    std::pmr::map<MirFunction *, FrameLayout> m_layouts;

    MirFrameLowererPassResult(std::pmr::memory_resource *allocator) : m_layouts(allocator) {}
};

class MirFrameLowererPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context and target desc.
     */
    MirFrameLowererPass(MirBuilderContext *ctx, TargetDesc *targetDesc);

    /**
     * Returns "MirFrameLowererPass".
     */
    const char *getName() const override;

    /**
     * Returns the result of this pass, if any.
     */
    const MirFrameLowererPassResult &getResult();

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass on the given function. This pass will:
     *  - Compute the stack frame layout (total reserve size for the stack, including saved callee registers). As well
     *  as the offsets of each stack frame object of the function.
     *  - Insert prologue
     *  - Insert epilogue
     *  - Rewrite instructions that use a MirReference(StackFrameObject) into a MirMemory(FP/SP + offset).
     */
    MirPassResult run(std::pmr::list<class MirFunction *> &funcList,
                      std::pmr::list<class MirFunction *>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Prints the content of each lowered function.
     */
    void printResult() const override;

    /**
     * Clears the result of this pass.
     */
    void reset() override;

    /**
     * This pass depends on MirRegisterAllocator.
     */
    std::vector<std::type_index> getDependencies() const override;

  private:
    MirBuilderContext *m_ctx;
    MirFrameLowererPassResult m_result;
    TargetDesc *m_targetDesc;
};

#endif // EZPACKER_MIRFRAMELOWERERPASS_H
