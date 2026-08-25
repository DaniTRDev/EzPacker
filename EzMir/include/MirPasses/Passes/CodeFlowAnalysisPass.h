#ifndef EZMIR_CODE_FLOW_ANALYSIS_PASS_H
#define EZMIR_CODE_FLOW_ANALYSIS_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirAnalysisPass.h"

/**
 * Resulting structure that contains the resulting Code Flow Graph.
 */
struct CodeFlowResult
{
    std::pmr::map<MirId, std::pmr::set<MirId>> m_successors;
    std::pmr::map<MirId, std::pmr::set<MirId>> m_predecessors;

    CodeFlowResult(std::pmr::memory_resource *arena) : m_successors(arena), m_predecessors(arena) {}
};

/**
 * This pass executes a CFG search and saves the result in a CodeFlowResult structure.
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
    CodeFlowResult *getResult();

    /**
     * Returns the iteration place for this pass (Function).
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass and builds a Code Flow Graph out of the given function iterator.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction> &funcList,
                      IntrusiveLinkedList<class MirFunction>::iterator it,
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

  private:
    CodeFlowResult m_result;
    class MirBuilderContext *m_ctx;
    std::pmr::memory_resource *m_arena;
};

#endif // EZPACKER_CODEFLOWANALYSIS_H