#ifndef EZPACKER_REGISTERALLOCATORPASS_H
#define EZPACKER_REGISTERALLOCATORPASS_H

#include "EzTargetCommon.h"
#include "RegisterAllocatorContext.h"

struct IGNode
{
    MirRegister *m_reg;
    std::unordered_set<MirRegister *> m_neighbors;

    // -1 means uncolored. >= 0 represents the physical register ID.
    int m_color{ -1 };
    bool m_isSpilled{ false };
};

class InterferenceGraph
{
  public:
    void addNode(MirRegister *reg);

    void addEdge(MirRegister *a, MirRegister *b);

    IGNode &getNode(MirRegister *reg);
    std::unordered_map<MirRegister *, IGNode> &getNodes();

    void clear();

  private:
    std::unordered_map<MirRegister *, IGNode> m_nodes;
};

/**
 * Allocates physical register to the virtual registers in the given mir. If there aren't any available registers, a
 * spill is made, which is moving a variable to stack.
 */
class RegisterAllocatorPass : public IMirTransformPass
{
  public:
    RegisterAllocatorPass(RegisterAllocatorContext *ctx);

    bool run(TypedPoolLinkedList<class MirFunction> *funcList,
             TypedPoolLinkedList<class MirFunction>::Iterator it,
             class MirPassManager *passManager) override;

    const char *getName() const override;

    MirPassIterationPlace getIterationPlace() const override;

  private:
    void buildGraph(class MirFunction *func, const LivenessResult &liveness);
    void simplifyAndSelect();
    bool rewriteProgram(class MirFunction *func);

    size_t getKForType(MirType *type);

    // Helpers to extract defined/used registers from an instruction
    std::vector<MirRegister *> getDefs(class MirInstruction *instr);
    std::vector<MirRegister *> getUses(class MirInstruction *instr);

  private:
    InterferenceGraph m_graph;
    MirEmitter *m_emitter;
    MirEmitterContext *m_emitterCtx;
    RegisterAllocatorContext *m_ctx;
    TargetDesc *m_targetDesc;

    // The stack used to hold nodes removed during the Simplify phase
    std::stack<MirRegister *> m_selectStack;

    size_t m_k; // Number of available physical registers
    std::vector<PhysicalRegId> m_allocatableRegs;
    std::unordered_set<MirRegister*> m_spillExempt;
};

#endif // EZPACKER_REGISTERALLOCATORPASS_H
