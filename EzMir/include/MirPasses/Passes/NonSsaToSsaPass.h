#ifndef EZMIR_NON_SSA_TO_SSA_PASS_H
#define EZMIR_NON_SSA_TO_SSA_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirTransformPass.h"

/**
 * Intermediate data structures computed during SSA reconstruction (dominance tree, dominance frontiers, def sites).
 */
struct NonSsaToSsaPassResult
{
    std::pmr::unordered_map<MirId, MirId> m_immDomTree;
    std::pmr::unordered_map<MirId, size_t> m_postOrderIndexes;

    // Virtual Register ID, Set of blocks that write to it (DEFs).
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirId>> m_defSites;
    std::pmr::unordered_map<MirId, std::pmr::unordered_set<MirId>> m_domFrontier;
    std::pmr::vector<MirId> m_postOrderNodes;

    NonSsaToSsaPassResult(std::pmr::memory_resource *alloc) :
        m_immDomTree(alloc), m_postOrderIndexes(alloc), m_defSites(alloc), m_domFrontier(alloc), m_postOrderNodes(alloc)
    {
    }
};

/**
 * Transformation pass converting non-SSA intermediate representation into Static Single Assignment (SSA) form.
 * Implements the classic Cytron et al. algorithm:
 * 1. Computes reverse post-order and immediate dominator tree (Cooper-Harvey-Kennedy).
 * 2. Computes dominance frontiers for all basic blocks.
 * 3. Places iterated phi-nodes at dominance frontiers for variables with multiple definition sites.
 * 4. Renames variable definitions and uses via recursive dominator tree traversal.
 */
class NonSsaToSsaPass : public IMirTransformPass
{
  public:
    /**
     * Constructs the SSA construction pass bound to the compilation context.
     */
    NonSsaToSsaPass(class MirBuilderContext *ctx);
    ~NonSsaToSsaPass() override = default;

    /**
     * Returns "NonSsaToSsaPass".
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Function.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Executes the SSA transformation pipeline over the targeted function.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction>::const_iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Returns the computed SSA pass metrics and dominator tree data.
     */
    NonSsaToSsaPassResult *getResult();

    /**
     * Prints dominance frontiers and dominator tree information to diagnostics.
     */
    void printResult() override;

    /**
     * Clears internal dominator tree and renaming maps for reuse.
     */
    void reset() override;

  private:
    /**
     * Instantiates and inserts a phi-node instruction for the specified register.
     */
    MirInstruction *createPhiInstruction(MirInstructionBuilder *iBuilder, MirId regId, size_t numPredecessors);

    /**
     * Scans the function to collect all basic blocks containing definitions for each virtual register.
     */
    void buildVirtualRegDefPlaces(MirFunction *func);

    /**
     * Computes dominance frontiers where control flow paths merge.
     */
    void buildDominanceFrontier(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager);

    /**
     * Computes the immediate dominator tree using the Cooper, Harvey, and Kennedy algorithm.
     */
    void buildImmDomTree(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager);

    /**
     * Computes post-order traversal ordering and dense index mapping over CFG blocks.
     */
    void buildPostOrderIndexList(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager);

    /**
     * Places phi-nodes at dominance frontiers of definition sites.
     */
    void insertPhiNodes(CodeFlowResult *cfg, MirFunction *func);

    /**
     * Renames variable uses and definitions via dominator tree search stack.
     */
    void renameVariables(CodeFlowResult *cfg, MirFunction *func);

  private:
    class MirBuilderContext *m_ctx;
    NonSsaToSsaPassResult m_result;
    std::pmr::memory_resource *m_resc;
};

#endif // EZMIR_NON_SSA_TO_SSA_PASS_H
