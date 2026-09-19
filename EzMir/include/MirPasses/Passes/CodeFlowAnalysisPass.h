#ifndef EZMIR_CODE_FLOW_ANALYSIS_PASS_H
#define EZMIR_CODE_FLOW_ANALYSIS_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirAnalysisPass.h"

/**
 * Control Flow Graph (CFG) representation storing predecessor and successor adjacency sets for each basic block.
 */
struct CodeFlowResult
{
    std::pmr::map<MirId, std::pmr::set<MirId>> m_successors;   // Block ID -> successor block IDs.
    std::pmr::map<MirId, std::pmr::set<MirId>> m_predecessors; // Block ID -> predecessor block IDs.

    /**
     * Allocates both adjacency maps from the given arena.
     */
    CodeFlowResult(std::pmr::memory_resource *arena) : m_successors(arena), m_predecessors(arena) {}
};

/**
 * Analysis pass constructing the explicit Control Flow Graph (CFG) for a function.
 * Inspects branch and jump instructions to compute predecessor and successor mappings for all basic blocks.
 */
class CodeFlowAnalysisPass : public IMirAnalysisPass
{
  public:
    virtual ~CodeFlowAnalysisPass() override = default;

    /**
     * Constructs a CFG analysis pass bound to the compilation context.
     */
    CodeFlowAnalysisPass(class MirBuilderContext *ctx);

    /**
     * Returns "CodeFlowAnalysisPass".
     */
    const char *getName() const override;

    /**
     * Returns the computed CFG predecessor/successor adjacency mappings.
     */
    CodeFlowResult *getResult();

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Executes CFG construction over the targeted function.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Prints predecessor and successor sets for each basic block in the function.
     */
    void printResult() override;

    /**
     * Clears internal CFG mappings for reuse across functions.
     */
    void reset() override;

  private:
    /**
     * Adds a directed control flow edge from the source basic block to the destination basic block.
     */
    void addEdge(const class MirBlock *from, const class MirBlock *to);

  private:
    CodeFlowResult m_result;            // Last computed CFG adjacency mappings.
    class MirBuilderContext *m_ctx;     // Context whose functions are inspected.
    std::pmr::memory_resource *m_arena; // Arena backing the result containers.
};

#endif // EZPACKER_CODEFLOWANALYSIS_H