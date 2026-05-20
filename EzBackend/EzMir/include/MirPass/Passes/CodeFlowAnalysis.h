#ifndef EZPACKER_CODEFLOWANALYSIS_H
#define EZPACKER_CODEFLOWANALYSIS_H

#include "EzMirCommon.h"
#include "MirPass/IMirAnalysisPass.h"
#include "MirBlock.h"
#include "Emitter/MirEmitter.h"

struct ControlFlowResult
{
    std::unordered_map<MirBlock *, std::vector<MirBlock *>> m_successors;
    std::unordered_map<MirBlock *, std::vector<MirBlock *>> m_predecessors;
};

class CodeFlowAnalysis : public IMirAnalysisPass
{
  public:
    virtual ~CodeFlowAnalysis() override = default;

    /**
     * Creates the analyzer with the given emitter.
     * @param emitter
     */
    CodeFlowAnalysis(MirEmitter *emitter);

    /**
     * Performs the CodeFlow analysis and builds the code-flow-graph.
     * @param func
     * @param pm
     * @return
     */
    bool run(TypedPoolLinkedList<class MirBlock> *blockList,
             TypedPoolLinkedList<class MirBlock>::Iterator it,
             class MirPassManager *passManager) override;

    const char *getName() const override;
    
    /**
     * Returns the result of the analysis.
     * @return
     */
    const ControlFlowResult &getResult() const;

    /**
     * Returns the iteration place. Depending on the place, one callback or the other will be called.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;
    
  private:
    /**
     * Adds an edge to the flow graph.
     * @param from
     * @param to
     */
    void addEdge(MirBlock *from, MirBlock *to);

    /**
     * Returns the destination block of a jump instruction.
     * @param inst
     * @return
     */
    MirBlock *getTargetJumpBlock(const MirInstruction *inst) const;

  private:
    ControlFlowResult m_result;
    MirEmitter *m_emitter;
};
#endif // EZPACKER_CODEFLOWANALYSIS_H
