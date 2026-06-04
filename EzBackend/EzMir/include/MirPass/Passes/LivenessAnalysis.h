#ifndef EZPACKER_LIVENESSANALYSIS_H
#define EZPACKER_LIVENESSANALYSIS_H

#include "EzMirCommon.h"
#include "CodeFlowAnalysis.h"

template <> struct std::hash<MirRegister>
{
    std::size_t operator()(const MirRegister &reg) const noexcept
    {
        // Combine id and virtual flag.
        std::size_t h1 = std::hash<size_t>{}(reg.getRegId());
        std::size_t h2 = std::hash<bool>{}(reg.isVirtual());
        return h1 ^ (h2 << 1);
    }
};

/**
 * @brief PMR-backed storage tracking variable lifespans across basic blocks.
 */
struct LivenessResult
{
    std::pmr::unordered_map<MirBlock *, std::pmr::unordered_set<MirRegister *>> m_liveIn;
    std::pmr::unordered_map<MirBlock *, std::pmr::unordered_set<MirRegister *>> m_liveOut;
    std::pmr::unordered_map<MirBlock *, std::pmr::unordered_set<MirRegister *>> m_def;
    std::pmr::unordered_map<MirBlock *, std::pmr::unordered_set<MirRegister *>> m_use;

    LivenessResult(std::pmr::memory_resource *arena) : m_liveIn(arena), m_liveOut(arena), m_def(arena), m_use(arena) {}
};

class LivenessAnalysis : public IMirAnalysisPass
{
  public:
    ~LivenessAnalysis() override = default;

    /**
     * @brief Allocates the liveness analyzer maps on the global compilation arena.
     */
    LivenessAnalysis(std::pmr::memory_resource *globalArena);

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
     * @brief Explicitly establishes dependency mapping rules.
     * Guaranteed to compile and run CodeFlowAnalysis prior to executing liveness.
     */
    std::vector<std::type_index> getDependencies() const override;

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

    // Helpers to extract read/written registers out of generic instructions
    void extractRegistersFromInstruction(MirInstruction *instr,
                                         std::pmr::unordered_set<MirRegister *> &defs,
                                         std::pmr::unordered_set<MirRegister *> &uses);

  private:
    LivenessResult m_result;
    std::pmr::memory_resource *m_arena;
};

#endif // EZPACKER_LIVENESSANALYSIS_H
