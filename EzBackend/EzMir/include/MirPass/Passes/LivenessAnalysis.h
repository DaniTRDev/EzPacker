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
        std::size_t h1 = std::hash<size_t>{}(reg.m_id);
        std::size_t h2 = std::hash<bool>{}(reg.m_virtual);
        return h1 ^ (h2 << 1);
    }
};

struct LivenessResult
{
    std::unordered_map<MirBlock *, std::unordered_set<MirRegister>> m_liveIn;
    std::unordered_map<MirBlock *, std::unordered_set<MirRegister>> m_liveOut;
    std::unordered_map<MirBlock *, std::unordered_set<MirRegister>> m_def;
    std::unordered_map<MirBlock *, std::unordered_set<MirRegister>> m_use;
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
    bool run(MirFunction *func, MirPassManager *pm) override;

    /**
     * Returns the result of the analysis.
     * @return
     */
    const LivenessResult &getResult() const;

  private:
    /**
     * Computes the local liveness.
     * @param func
     */
    void computeLocalLiveness(MirFunction *func);

    /**
     * Computes the global liveness.
     * @param func
     * @param cfg
     */
    void computeGlobalLiveness(MirFunction *func, const ControlFlowResult &cfg);

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
