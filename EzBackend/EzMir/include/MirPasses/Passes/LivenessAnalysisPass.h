#ifndef EZPACKER_LIVENESSANALYSIS_H
#define EZPACKER_LIVENESSANALYSIS_H

#include "EzMirCommon.h"
#include "CodeFlowAnalysisPass.h"

/**
 * @brief PMR-backed storage tracking variable lifespans across basic blocks.
 */
struct LivenessResult
{
    // Block Id, <RegisterRef>.
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<RegisterRef>> m_liveIn;
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<RegisterRef>> m_liveOut;
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<RegisterRef>> m_def;
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<RegisterRef>> m_use;

    LivenessResult(std::pmr::memory_resource *arena) : m_liveIn(arena), m_liveOut(arena), m_def(arena), m_use(arena) {}
};

class LivenessAnalysisPass : public IMirAnalysisPass
{
  public:
    ~LivenessAnalysisPass() override = default;

    /**
     * @brief Allocates the liveness analyzer maps on the global compilation arena.
     */
    LivenessAnalysisPass(MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "LivenessAnalysisPass".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns the result of the pass. The result contains a live interval of the variables of a function.
     * @return
     */
    const LivenessResult &getResult() const;

    /**
     * Returns MirPassIterationPlace::Function.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass and builds a the live in-out intervals of the variables used in a function.
     * @param funcList
     * @param it
     * @param passManager
     * @return
     */
    MirPassResult run(std::pmr::list<MirFunction *> &funcList,
                      std::pmr::list<MirFunction *>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Prints the result of the liveness analysis. It shows the def/use set of variables of each block and the global
     * live in/out graph.
     */
    void printResult() const override;

    /**
     * Resets the result of the pass.
     */
    void reset() override;
private:
    /**
     * Computes the gloval live-in/live-out set of a function.
     * @param func
     * @param cfg
     */
    void computeGlobalLiveness(MirFunction *func, const ControlFlowResult &cfg);

    /**
     * Computes the local def/use of the blocks inside the function.
     * @param func
     * @param collector
     */
    void computeLocalLiveness(MirFunction *func, const std::shared_ptr<DiagnosticCollector> &collector);

  private:
    LivenessResult m_result;
    MirBuilderContext *m_ctx;
    std::pmr::memory_resource *m_arena;
};

#endif // EZPACKER_LIVENESSANALYSIS_H
