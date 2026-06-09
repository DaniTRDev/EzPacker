#ifndef EZPACKER_CODEFLOWPASSVERIFIER_H
#define EZPACKER_CODEFLOWPASSVERIFIER_H

#include "MirCoreVerifiers.h"

/**
 * This class is used when testing the results of the CodeFlowAnalysis pass. It provides a high-level API used to
 * quick-test the pass.
 */
class CodeFlowAnalysisVerifier : public MirPassVerifier<CodeFlowAnalysis, CodeFlowAnalysisVerifier>
{
  public:
    /**
     * Creates the flow analysis verifier and attach it to an object. At the time of calling internal verifiers, ensure
     * the pass has results.
     * @param analysis
     * @param ctx
     */
    CodeFlowAnalysisVerifier(CodeFlowAnalysis *analysis, MirBuilderContext *ctx);

    /**
     * Checks if the given block exits the flow (return, which causes 0 successors).
     * @param blockId
     * @return
     */
    CodeFlowAnalysisVerifier &exitBlock(size_t blockId);

    /**
     * Checks if the block with 'fromId' has a predecessor 'toId' (same as checking if 'toId' has a successor 'fromId').
     * @param to
     * @param from
     * @return
     */
    CodeFlowAnalysisVerifier &predecessor(size_t toId, size_t fromId);

    /**
     * Checks if predecessor count of the target block matches the given count.
     * @param blockId
     * @param count
     * @return
     */
    CodeFlowAnalysisVerifier &predecessorCount(size_t blockId, size_t count);

    /**
     * Checks if there's a path between 'start' and 'end'.
     * @param blockId
     * @param count
     * @return
     */
    CodeFlowAnalysisVerifier &reachable(size_t start, size_t end);

    /**
     * Checks if the block with 'fromId' has a successor 'toId'.
     * @param from
     * @param to
     * @return
     */
    CodeFlowAnalysisVerifier &successor(size_t fromId, size_t toId);

    /**
     * Checks if successor count of the target block matches the given count.
     * @param blockId
     * @param count
     * @return
     */
    CodeFlowAnalysisVerifier &successorCount(size_t blockId, size_t count);

    /**
     * Asserts that a block is dead / completely stranded from the flow.
     * @param start
     * @param end
     * @return
     */
    CodeFlowAnalysisVerifier &unreachable(size_t start, size_t end);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_CODEFLOWPASSVERIFIER_H
