#ifndef EZMIR_MIR_VERIFIER_PASS_H
#define EZMIR_MIR_VERIFIER_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirAnalysisPass.h"
#include <cstddef>

/**
 * Result metrics for the MIR verification analysis pass.
 */
struct MirVerifierPassResult
{
    size_t m_instructionCount{ 0 }; ///< Total instructions inspected during verification.
    size_t m_errorCount{ 0 };       ///< Number of invariant / specification violations encountered.
    size_t m_warningCount{ 0 };     ///< Number of non-fatal warnings encountered.

    /**
     * Returns true if no invariant violations or verification errors were recorded.
     */
    [[nodiscard]] bool isValid() const { return m_errorCount == 0; }

    /**
     * Resets verification result counters.
     */
    void reset()
    {
        m_instructionCount = 0;
        m_errorCount = 0;
        m_warningCount = 0;
    }
};

/**
 * Read-only analysis pass that inspects functions, blocks, and instructions to ensure input MIR
 * strictly adheres to opcode metadata specifications and semantic flags prior to middle-end passes.
 *
 * Verifies:
 * 1. Opcode validity (non-sentinel, recognized opcode).
 * 2. Operand count (exact match for fixed-arity instructions, minimum required for variadic).
 * 3. Operand kinds and storage (matching ExpectedOperandType bitmasks, DEF operands are writable registers).
 * 4. Semantic size flags:
 *    - SizeMatch: All value operands share identical bit-width (or compared lhs/rhs for relational comparisons).
 *    - DestLarger: Destination bit-width strictly exceeds source bit-width (ZEXT, SEXT, FPEXT).
 *    - DestSmaller: Destination bit-width is strictly smaller than source bit-width (TRUNC, FPTRUNC).
 * 5. Type consistency:
 *    - TreatAsSigned requires integer scalar types.
 *    - Arithmetic and bitwise operations adhere to integer vs floating-point rules.
 * 6. Basic block structure:
 *    - Terminator instructions (IsTerminator) must be the last instruction of a basic block.
 */
class MirVerifierPass : public IMirAnalysisPass
{
  public:
    /**
     * Constructs a MIR verification pass bound to the compilation builder context.
     */
    explicit MirVerifierPass(class MirBuilderContext *ctx);
    ~MirVerifierPass() override = default;

    /**
     * Returns "MirVerifierPass".
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Executes verification analysis over the targeted function.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Formats and logs pass verification metrics to the diagnostic collector.
     */
    void printResult() override;

    /**
     * Resets internal verification result counters.
     */
    void reset() override;

    /**
     * Returns the verification metrics and validity status from the last run.
     */
    [[nodiscard]] const MirVerifierPassResult &getResult() const { return m_result; }

  private:
    bool verifyFunction(class MirFunction *func);
    bool verifyBlock(class MirBlock *block, class MirFunction *func);
    bool verifyInstruction(class MirInstruction *inst, class MirBlock *block, class MirFunction *func);
    bool verifyOperandCount(class MirInstruction *inst, const struct MirInstructionMetadata &meta);
    bool verifyOperandKinds(class MirInstruction *inst, const struct MirInstructionMetadata &meta);
    bool verifyFlagsAndSizes(class MirInstruction *inst, const struct MirInstructionMetadata &meta);
    bool verifyTypeConsistency(class MirInstruction *inst, const struct MirInstructionMetadata &meta);
    bool verifyTerminatorPlacement(class MirInstruction *inst, class MirBlock *block);

  private:
    class MirBuilderContext *m_ctx;
    MirVerifierPassResult m_result;
};

#endif // EZMIR_MIR_VERIFIER_PASS_H
