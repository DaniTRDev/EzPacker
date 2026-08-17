#ifndef EZMIR_MIR_FRAME_LOWERER_PASS_H
#define EZMIR_MIR_FRAME_LOWERER_PASS_H

#include "EzTripleCommon.h"
#include "MirPasses/IMirTransformPass.h"

class MirFrameLowererPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context and target desc.
     */
    MirFrameLowererPass(class MirBuilderContext *ctx, class TargetDesc *targetDesc);

    /**
     * Returns "MirFrameLowererPass".
     */
    const char *getName() const override;

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
     * Prints each lowered function using MirPrinter.
     */
    void printResult() override;

    /**
     * Clears m_lowredFunction list.
     */
    void reset() override;

    /**
     * This pass depends on MirRegisterAllocator.
     */
    std::vector<std::type_index> getDependencies() const override;

  private:
    class MirBuilderContext *m_ctx;
    class TargetDesc *m_targetDesc;
    std::pmr::list<class MirFunction *> m_loweredFunctions;
};

#endif // EZMIR_MIR_FRAME_LOWERER_PASS_H
