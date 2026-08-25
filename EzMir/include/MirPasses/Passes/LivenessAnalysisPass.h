#ifndef EZMIR_LIVENESS_ANALYSIS_H
#define EZMIR_LIVENESS_ANALYSIS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirAnalysisPass.h"
#include "Operand/MirRegisterReference.h"
#include "HelperClasses/DenseBitSet.h"

struct LivenessResult
{
    // Block Id, <MirRegisterRef>.
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>> m_liveIn;
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>> m_liveOut;
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>> m_def;
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>> m_use;

    LivenessResult(std::pmr::memory_resource *arena) : m_liveIn(arena), m_liveOut(arena), m_def(arena), m_use(arena) {}
};

class LivenessAnalysisPass : public IMirAnalysisPass
{
  public:
    ~LivenessAnalysisPass() override = default;

    /**
     * Allocates the liveness analyzer maps on the global compilation arena.
     */
    LivenessAnalysisPass(class MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "LivenessAnalysisPass"
     */
    const char *getName() const override;

    /**
     * Returns the result of the pass. The result contains a live interval of the variables of a function.
     */
    LivenessResult *getResult();

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass and builds a the live in-out intervals of the variables used in a function.
     */
    MirPassResult run(std::pmr::list<class MirFunction *> &funcList,
                      std::pmr::list<class MirFunction *>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Prints the result of the liveness analysis. It shows the def/use set of variables of each block and the global
     * live in/out graph.
     */
    void printResult() override;

    /**
     * Resets the result of the pass.
     */
    void reset() override;

  private:
    /**
     * Computes the gloval live-in/live-out set of a function.
     */
    void computeGlobalLiveness(class MirFunction *func, class CodeFlowResult *cfg);

    /**
     * Computes the local def/use of the blocks inside the function.
     */
    void computeLocalLiveness(class MirFunction *func);

  private:
    LivenessResult m_result;
    class MirBuilderContext *m_ctx;
    std::pmr::memory_resource *m_arena;
};

#endif // EZMIR_LIVENESS_ANALYSIS_H
