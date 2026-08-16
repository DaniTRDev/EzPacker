#ifndef EZMIR_CODE_FLOW_ANALYSIS_PASS_H
#define EZMIR_CODE_FLOW_ANALYSIS_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirAnalysisPass.h"

/**
 * Resulting structure that contains the resulting Code Flow Graph.
 */
struct ControlFlowResult
{
    std::pmr::unordered_map<MirId, std::pmr::set<MirId>> m_successors;
    std::pmr::unordered_map<MirId, std::pmr::set<MirId>> m_predecessors;

    ControlFlowResult(std::pmr::memory_resource *arena) : m_successors(arena), m_predecessors(arena) {}
};

/**
 * This pass executes a CFG search and saves the result in a ControlFlowResult structure.
 */
class CodeFlowAnalysisPass : public IMirAnalysisPass
{
  public:
    virtual ~CodeFlowAnalysisPass() override = default;

    /**
     * Creates the analyzer with the given context.
     */
    CodeFlowAnalysisPass(class MirBuilderContext *ctx);

    /**
     * Returns "CodeFlowAnalysisPass".
     */
    const char *getName() const override;

    /**
     * Returns the result of the pass, if populated. If run was not called, an empty result is returned.
     */
    ControlFlowResult *getResult();

    /**
     * Returns the iteration place for this pass (Function).
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass and builds a Code Flow Graph out of the given function iterator.
     */
    MirPassResult run(std::pmr::list<class MirFunction *> &funcList,
                      std::pmr::list<class MirFunction *>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * For every block processed in after calling run, it prints its predecessors and successors.
     */
    void printResult() override;

    /**
     * Resets the result of the pass.
     */
    void reset() override;

  private:
    void addEdge(class MirBlock *from, class MirBlock *to);
    MirBlock *getTargetJumpBlock(const class MirInstruction *inst) const;

  private:
    ControlFlowResult m_result;
    class MirBuilderContext *m_ctx;
    std::pmr::memory_resource *m_arena;
};

#endif // EZPACKER_CODEFLOWANALYSIS_H