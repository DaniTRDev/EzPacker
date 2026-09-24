#ifndef EZMIR_CODE_FLOW_ANALYSIS_PASS_H
#define EZMIR_CODE_FLOW_ANALYSIS_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirAnalysisPass.h"
#include <span>

/**
 * Control Flow Graph (CFG) representation for the basic blocks of the analysed functions.
 *
 * Adjacency is stored in dense per-block slot vectors instead of ordered MirId->set maps: every
 * block is assigned a compact slot on first sight and its successor/predecessor lists are stored
 * contiguously. Each adjacency list is kept sorted by MirId, so iteration order (and therefore PHI
 * predecessor numbering) stays deterministic. The result accumulates across the functions the pass
 * visits; the pass manager's single reset() clears it.
 */
struct CodeFlowResult
{
    /**
     * Allocates the adjacency storage from the given arena.
     */
    explicit CodeFlowResult(std::pmr::memory_resource *arena);

    /**
     * Ensures a slot exists for blockId and returns its index.
     */
    size_t ensureBlock(MirId blockId);

    /**
     * Records a directed edge from -> to in both the successor and predecessor lists, ignoring
     * null-like duplicate edges and keeping every list sorted.
     */
    void addEdge(MirId from, MirId to);

    /**
     * Returns true when a slot was assigned to blockId.
     */
    [[nodiscard]] bool contains(MirId blockId) const;

    /**
     * Returns the number of blocks tracked in the graph.
     */
    [[nodiscard]] size_t getBlockCount() const;

    /**
     * Returns the sorted successor MirIds of blockId, or an empty span when the block is unknown.
     */
    [[nodiscard]] std::span<const MirId> getSuccessors(MirId blockId) const;

    /**
     * Returns the sorted predecessor MirIds of blockId, or an empty span when the block is unknown.
     */
    [[nodiscard]] std::span<const MirId> getPredecessors(MirId blockId) const;

    /**
     * Returns the block MirIds in slot order (used for deterministic reporting).
     */
    [[nodiscard]] const std::pmr::vector<MirId> &getBlockIds() const;

    /**
     * Clears every block slot and edge, ready for another run.
     */
    void reset();

  private:
    /**
     * Returns the slot assigned to blockId or InvalidSlot when it was never assigned.
     */
    [[nodiscard]] size_t slotOf(MirId blockId) const;

    static constexpr size_t InvalidSlot = static_cast<size_t>(-1);

    std::pmr::memory_resource *m_arena;                       // Arena backing every adjacency vector.
    std::pmr::vector<MirId> m_blockIds;                       // slot -> block MirId.
    std::pmr::vector<std::pmr::vector<MirId>> m_successors;   // slot -> sorted successor MirIds.
    std::pmr::vector<std::pmr::vector<MirId>> m_predecessors; // slot -> sorted predecessor MirIds.
    std::pmr::unordered_map<MirId, size_t> m_slotById;        // block MirId -> slot.
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
    CodeFlowResult m_result;            // Accumulated CFG adjacency mappings.
    class MirBuilderContext *m_ctx;     // Context whose functions are inspected.
    std::pmr::memory_resource *m_arena; // Arena backing the result containers.
};

#endif // EZPACKER_CODEFLOWANALYSIS_H
