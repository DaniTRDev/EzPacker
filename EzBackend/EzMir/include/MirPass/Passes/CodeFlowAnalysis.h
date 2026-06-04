#ifndef EZPACKER_CODEFLOWANALYSIS_H
#define EZPACKER_CODEFLOWANALYSIS_H

#include "EzMirCommon.h"
#include "Builder/MirBuilderContext.h"
#include "MirPass/IMirAnalysisPass.h"
#include "MirPass/MirPassManager.h"

/**
 * @brief High-performance PMR-backed snapshot of a function's control flow graph.
 */
struct ControlFlowResult
{
    std::pmr::unordered_map<MirBlock *, std::pmr::vector<MirBlock *>> m_successors;
    std::pmr::unordered_map<MirBlock *, std::pmr::vector<MirBlock *>> m_predecessors;

    ControlFlowResult(std::pmr::memory_resource *arena) : m_successors(arena), m_predecessors(arena) {}
};

class CodeFlowAnalysis : public IMirAnalysisPass
{
  public:
    virtual ~CodeFlowAnalysis() override = default;

    /**
     * Creates the analyzer with the given context.
     * @param ctx
     */
    CodeFlowAnalysis(MirBuilderContext *ctx);

    /**
     * Returns "CodeFlowAnalysisPass".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns the result of the pass, if populated. If run was not called, an empty result is returned.
     * @return
     */
    const ControlFlowResult &getResult() const;

    /**
     * Returns the iteration place for this pass (Function).
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass and builds a Code Flow Graph out of the given function iterator.
     * @param funcList
     * @param it
     * @param passManager
     * @return
     */
    MirPassResult run(std::pmr::list<MirFunction *> &funcList,
                      std::pmr::list<MirFunction *>::iterator it,
                      class MirPassManager *passManager) override;

  private:
    void addEdge(MirBlock *from, MirBlock *to);
    MirBlock *getTargetJumpBlock(const MirInstruction *inst) const;

  private:
    ControlFlowResult m_result;
    MirBuilderContext *m_ctx;
    std::pmr::memory_resource *m_arena;
};

#endif // EZPACKER_CODEFLOWANALYSIS_H