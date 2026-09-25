#ifndef EZTRIPLE_MIR_TARGET_PEEPHOLE_PASS_H
#define EZTRIPLE_MIR_TARGET_PEEPHOLE_PASS_H

#include "EzTripleCommon.h"
#include "MirPasses/IMirTransformPass.h"
#include <cstddef>
#include <vector>

/**
 * Statistics collected during target machine peephole optimization.
 */
struct MirTargetPeepholeMetrics
{
    size_t m_selfMovesEliminated{ 0 };
    size_t m_reciprocalMovesEliminated{ 0 };
    size_t m_redundantJumpsEliminated{ 0 };
    size_t m_zeroIdentitiesEliminated{ 0 };
    size_t m_spillReloadsForwarded{ 0 };
    size_t m_redundantLoadsEliminated{ 0 };
    size_t m_deadStoresEliminated{ 0 };

    [[nodiscard]] size_t totalEliminated() const
    {
        return m_selfMovesEliminated + m_reciprocalMovesEliminated + m_redundantJumpsEliminated +
               m_zeroIdentitiesEliminated + m_spillReloadsForwarded + m_redundantLoadsEliminated +
               m_deadStoresEliminated;
    }
};

/**
 * Target-level peephole optimization pass executing machine-specific and target-agnostic
 * simplifications after register allocation and frame lowering:
 *  - Machine self-move elimination (MOV %r, %r)
 *  - Reciprocal move elimination (MOV %r1, %r2; MOV %r2, %r1)
 *  - Redundant adjacent fall-through branch/jump elimination
 *  - Zero-identity arithmetic elimination (ADD %r, %r, 0; SUB %r, %r, 0)
 *  - Spill/reload forwarding and redundant memory access elimination
 */
class MirTargetPeepholePass : public IMirTransformPass
{
  public:
    MirTargetPeepholePass(class MirBuilderContext *ctx, class TargetDesc *targetDesc);

    const char *getName() const override;

    MirPassIterationPlace getIterationPlace() const override;

    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

    void printResult() override;

    void reset() override;

    std::vector<std::type_index> getDependencies() const override;

    [[nodiscard]] const MirTargetPeepholeMetrics &getMetrics() const { return m_metrics; }

  private:
    bool optimizeBlock(class MirBlock *block);
    bool tryEliminateSelfMove(class MirInstruction *inst, class MirBlock *block);
    bool tryEliminateReciprocalMove(class MirInstruction *inst, class MirBlock *block);
    bool tryEliminateAdjacentJump(class MirInstruction *inst, class MirBlock *block);
    bool tryEliminateZeroIdentity(class MirInstruction *inst, class MirBlock *block);
    bool tryOptimizeMemoryAccesses(class MirInstruction *inst, class MirBlock *block);

    class MirBuilderContext *m_ctx;
    class TargetDesc *m_targetDesc;
    MirTargetPeepholeMetrics m_metrics;
};

#endif // EZTRIPLE_MIR_TARGET_PEEPHOLE_PASS_H
