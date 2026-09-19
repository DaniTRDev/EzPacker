#ifndef EZMIR_LIVENESS_ANALYSIS_H
#define EZMIR_LIVENESS_ANALYSIS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirAnalysisPass.h"
#include "Operand/MirRegisterReference.h"
#include "HelperClasses/DenseBitSet.h"

/**
 * Dataflow liveness analysis container tracking live-in, live-out, def, and use register sets per basic block.
 */
struct LivenessResult
{
    // Block Id, <MirRegisterRef>.
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>>
            m_liveIn; // Registers live on entry to each block.
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>>
            m_liveOut; // Registers live on exit from each block.
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>>
            m_def; // Registers defined within each block.
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirRegisterRef>>
            m_use; // Registers used before being defined in each block.

    /**
     * Allocates all four per-block sets from the given arena.
     */
    LivenessResult(std::pmr::memory_resource *arena) : m_liveIn(arena), m_liveOut(arena), m_def(arena), m_use(arena) {}
};

/**
 * Backward dataflow analysis pass computing live register ranges across the CFG.
 * Determines variable live-in and live-out sets for register allocation and dead code elimination.
 */
class LivenessAnalysisPass : public IMirAnalysisPass
{
  public:
    ~LivenessAnalysisPass() override = default;

    /**
     * Constructs a liveness analysis pass with arena storage.
     */
    LivenessAnalysisPass(class MirBuilderContext *ctx);

    /**
     * Returns "LivenessAnalysisPass".
     */
    const char *getName() const override;

    /**
     * Returns the computed liveness analysis data (liveIn, liveOut, def, use sets).
     */
    LivenessResult *getResult();

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Executes local and global backward dataflow analysis over the function.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Formats and prints def/use and live-in/live-out sets to diagnostics.
     */
    void printResult() override;

    /**
     * Clears internal liveness sets for reuse.
     */
    void reset() override;

  private:
    /**
     * Solves backward dataflow equations using sparse bitsets for cross-block registers:
     * LiveIn[B] = Use[B] U (LiveOut[B] - Def[B])
     * LiveOut[B] = U { LiveIn[S] for S in Successors(B) }
     */
    void computeGlobalLiveness(class MirFunction *func, class CodeFlowResult *cfg);

  private:
    LivenessResult m_result;            // Last computed liveness sets.
    class MirBuilderContext *m_ctx;     // Context whose functions are inspected.
    std::pmr::memory_resource *m_arena; // Arena backing the result containers.
};

#endif // EZMIR_LIVENESS_ANALYSIS_H
