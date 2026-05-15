#ifndef EZPACKER_LIVENESSANALYSIS_H
#define EZPACKER_LIVENESSANALYSIS_H

#include "EzMirCommon.h"
#include "MirPass/IMirAnalysisPass.h"
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

struct LivenessResult
{
    std::unordered_map<size_t, std::unordered_set<MirRegister>> m_liveIn;
    std::unordered_map<size_t, std::unordered_set<MirRegister>> m_liveOut;
    std::unordered_map<size_t, std::unordered_set<MirRegister>> m_def;
    std::unordered_map<size_t, std::unordered_set<MirRegister>> m_use;
};

class LivenessAnalysis : public IMirAnalysisPass
{
  public:
    ~LivenessAnalysis() override = default;

    /**
     * Creates the liveness analyzer with the given emitter.
     * @param emitter
     */
    LivenessAnalysis(MirEmitter *emitter);

    /**
     * Executes the liveness analysis of the given function.
     * @param func
     * @param pm
     * @return
     */
    bool run(TypedPoolLinkedList<class MirBlock> *blockList,
             TypedPoolLinkedList<class MirBlock>::Iterator it,
             class MirPassManager *passManager) override;

    /**
     * Returns the result of the analysis.
     * @return
     */
    const LivenessResult &getResult() const;

    /**
     * Returns the iteration place. Depending on the place, one callback or the other will be called.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

  private:
    /**
     * Computes the local liveness.
     * @param func
     */
    void computeLocalLiveness(TypedPoolLinkedList<class MirBlock> *blockList,
                              TypedPoolLinkedList<class MirBlock>::Iterator it);

    /**
     * Computes the global liveness.
     * @param func
     * @param cfg
     */
    void computeGlobalLiveness(TypedPoolLinkedList<class MirBlock> *blockList,
                               TypedPoolLinkedList<class MirBlock>::Iterator it,
                               const ControlFlowResult &cfg);

    /**
     * Does instruction write into the operand?
     * @param meta
     * @param operandIndex
     * @return
     */
    bool isOperandDef(const class MirInstructionMetadata &meta, size_t operandIndex) const;

  private:
    LivenessResult m_result;
    MirEmitter *m_emitter;
};

#endif // EZPACKER_LIVENESSANALYSIS_H
