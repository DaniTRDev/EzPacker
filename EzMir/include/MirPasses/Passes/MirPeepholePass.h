#ifndef EZMIR_MIR_PEEPHOLE_PASS_H
#define EZMIR_MIR_PEEPHOLE_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirTransformPass.h"

/**
 * Result metrics for the generic MIR peephole optimization pass.
 */
struct MirPeepholePassResult
{
    size_t m_algebraicSimplifications{ 0 }; ///< Number of algebraic identity rewrites applied.
    size_t m_redundantMovesEliminated{ 0 }; ///< Number of redundant self/reciprocal moves erased.
    size_t m_deadInstructionsEliminated{ 0 }; ///< Number of dead/unreachable instructions erased.
    size_t m_branchesSimplified{ 0 }; ///< Number of redundant jump instructions erased.

    void reset()
    {
        m_algebraicSimplifications = 0;
        m_redundantMovesEliminated = 0;
        m_deadInstructionsEliminated = 0;
        m_branchesSimplified = 0;
    }

    [[nodiscard]] size_t totalTransformations() const
    {
        return m_algebraicSimplifications + m_redundantMovesEliminated + m_deadInstructionsEliminated +
                m_branchesSimplified;
    }
};

/**
 * Middle-end transformation pass performing local window peephole optimizations on generic SSA MIR.
 *
 * Traverses basic blocks and applies:
 * 1. Algebraic identity simplifications (e.g. ADD x, 0 -> MOV, SUB x, 0 -> MOV, SUB x, x -> MOV 0,
 *    IMUL x, 1 -> MOV, IMUL x, 0 -> MOV 0, AND x, 0 -> MOV 0, AND x, -1 -> MOV, AND x, x -> MOV,
 *    OR x, 0 -> MOV, OR x, x -> MOV, XOR x, 0 -> MOV, XOR x, x -> MOV 0, SHL/SHR x, 0 -> MOV).
 * 2. Redundant move elimination (self-moves MOV r, r and consecutive reciprocal moves).
 * 3. Control flow simplifications (erasing unconditional jumps to sequential fall-through blocks).
 * 4. Dead instruction elimination (pruning unreachable instructions following block terminators).
 */
class MirPeepholePass : public IMirTransformPass
{
  public:
    /**
     * Constructs the peephole pass bound to the compilation builder context.
     */
    explicit MirPeepholePass(class MirBuilderContext *ctx);
    ~MirPeepholePass() override = default;

    /**
     * Returns "MirPeepholePass".
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Executes peephole optimization over all basic blocks in the specified function.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Clears internal transformation metrics.
     */
    void reset() override;

    /**
     * Logs pass transformation metrics to the diagnostic collector.
     */
    void printResult() override;

    /**
     * Returns the metrics result bundle for this pass.
     */
    [[nodiscard]] const MirPeepholePassResult &getResult() const { return m_result; }

  private:
    /**
     * Applies algebraic identity simplifications to a single instruction if applicable.
     * Returns true if the instruction was mutated or simplified.
     */
    bool trySimplifyAlgebraic(class MirInstruction *inst, class MirBlock *block);

    /**
     * Applies redundant move elimination (self-moves and reciprocal moves).
     * Returns true if an instruction was erased.
     */
    bool trySimplifyMove(class MirInstruction *inst, class MirBlock *block);

    /**
     * Eliminates unreachable instructions following basic block terminators.
     * Returns true if any dead instructions were erased.
     */
    bool eliminateDeadInstructionsAfterTerminators(class MirBlock *block);

    /**
     * Eliminates redundant unconditional jumps that branch to the sequential layout successor.
     * Returns true if a redundant jump was erased.
     */
    bool trySimplifyBranch(class MirInstruction *inst, class MirBlock *block);

    /**
     * Replaces an instruction with a MOV of src to dst (clearing operand 2).
     */
    void rewriteToMov(class MirInstruction *inst, class MirOperand *src);

    /**
     * Replaces an instruction with a MOV of immediate zero to dst.
     */
    void rewriteToZero(class MirInstruction *inst);

  private:
    class MirBuilderContext *m_ctx; ///< Shared builder context.
    MirPeepholePassResult m_result;  ///< Metrics collected during transformation.
};

#endif // EZMIR_MIR_PEEPHOLE_PASS_H
