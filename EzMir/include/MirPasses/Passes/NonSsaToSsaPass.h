#ifndef EZMIR_NON_SSA_TO_SSA_PASS_H
#define EZMIR_NON_SSA_TO_SSA_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirTransformPass.h"

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
 * This pass runs the algorithm "Cytron et al." to transform a non-SSA input IR into an SSA-compliant IR. For details,
 * look at the headers of private sub phases and in the code of each of the methods as this is rather complex to
 * explain.
 */
class NonSsaToSsaPass : public IMirTransformPass
{
  public:
    /**
     * Creates the analyzer with the given context.
     */
    NonSsaToSsaPass(class MirBuilderContext *ctx);
    ~NonSsaToSsaPass() override = default;

    /**
     * Returns "NonSsaToSsaPass".
     */
    const char *getName() const override;

    /**
     * Returns the iteration place for this pass (MirPassIterationPlace::Function).
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass and builds a n SSA IR out of a non-SSA IR.
     */
    MirPassResult run(IntrusiveLinkedList<class MirFunction> &funcList,
                      IntrusiveLinkedList<class MirFunction>::iterator it,
                      class MirPassManager *passManager) override;

    /**
     * Returns the result of the pass.
     */
    NonSsaToSsaPassResult *getResult();

    /**
     * For every block processed in after calling run, it prints its predecessors and successors.
     */
    void printResult() override;

    /**
     * Resets the result of the pass.
     */
    void reset() override;

  private:
    /**
     * Creates a PHI instruction with the given register IDs in the place that is currently bound in iBuilder.
     */
    MirInstruction *createPhiInstruction(MirInstructionBuilder *iBuilder, MirId regId, size_t numPredecessors);

    /**
     * Builds a map that contains the places in which a virtual register is defined (written).
     */
    void buildVirtualRegDefPlaces(MirFunction *func);

    /**
     * This function computes the dominance frontier. The Dominance Frontier of a block B is the set of blocks where
     * B's dominance "ends". This is exactly where control flow merges, and therefore exactly where PHI nodes need to be
     * inserted.
     */
    void buildDominanceFrontier(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager);

    /**
     * Builds the immediate dominator tree using Cooper, Harvey, and Kennedy algorithm.
     */
    void buildImmDomTree(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager);

    /**
     * Traverses the blocks of the function in post-order and assings the position they are saved in the final postOrder
     * list to the index list.
     *
     * Ex: post order results: { Block1, Block3, Block4 }
     * The post order index list would contain: list[Block1] = 0, list[Block3] = 1, list[Block4] = 2
     */
    void buildPostOrderIndexList(CodeFlowResult *cfg, MirFunction *func, MirPassManager *passManager);

    /**
     * Almost the last step of the alogorithm, it inserts PHI nodes at dominance frontiers. The nodes are in the form
     * of: PHI origReg, origReg, ... (up to the number of predecessors that write to the value).
     */
    void insertPhiNodes(CodeFlowResult *cfg, MirFunction *func);

    /**
     * Rename variables that were affected by PHI nodes. This is the last step and completes the SSA.
     */
    void renameVariables(CodeFlowResult *cfg, MirFunction *func);

  private:
    class MirBuilderContext *m_ctx;
    NonSsaToSsaPassResult m_result;
    std::pmr::memory_resource *m_resc;
};

#endif // EZMIR_NON_SSA_TO_SSA_PASS_H
