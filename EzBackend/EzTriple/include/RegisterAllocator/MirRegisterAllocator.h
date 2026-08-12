#ifndef EZPACKER_REGISTERALLOCATOR_H
#define EZPACKER_REGISTERALLOCATOR_H

#include "EzTripleCommon.h"
#include "Descriptors/TargetDesc.h"

struct RegisterAllocatorCtx
{
    bool m_needsFramePointer; // Does the function needs a mandatory FP (dynamic alloc found)?
    MirBuilderContext *m_ctx;
    MirFunction *m_targetFunction;
    TargetDesc *m_targetDesc;

    // Explicit PMR allocator handle for context containers
    std::pmr::memory_resource *m_allocator;

    /**
     * Stack of nodes removed during simplify, popped in reverse during select phase.
     */
    std::pmr::vector<RegisterRef> m_selectStack;

    /**
     * Nodes currently removed from the active graph during simplification.
     */
    std::pmr::unordered_set<RegisterRef> m_removedNodes;

    // A set of registers that are reserved and CAN'T be used during allocation.
    std::pmr::unordered_set<RegisterRef> m_reservedRegs;

    // First = Virtual Register. Second = Assigned Physical Register.
    std::pmr::unordered_map<RegisterRef, RegisterRef> m_allocatedRegs;

    // First = Register. Second = Number of active interference neighbors.
    std::pmr::unordered_map<RegisterRef, size_t> m_degree;

    // First = Register. Second = Assigned stack slot if spilled.
    std::pmr::unordered_map<RegisterRef, StackFrameObject *> m_spilledRegs;

    /**
     * Interference graph.
     *  - First = Node V
     *  - Second = Set of interfering neighbor registers.
     */
    std::pmr::unordered_map<RegisterRef, std::pmr::set<RegisterRef>> m_iGraph;

    // Set of registers that can't be optimistically spilled.
    std::pmr::unordered_set<RegisterRef> m_unspillableRegs;

    explicit RegisterAllocatorCtx(MirBuilderContext *ctx,
                                  MirFunction *targetFunction,
                                  TargetDesc *targetDesc,
                                  std::pmr::memory_resource *alloc) :
        m_needsFramePointer(false), m_ctx(ctx), m_targetFunction(targetFunction), m_targetDesc(targetDesc),
        m_allocator(alloc), m_selectStack(alloc), m_removedNodes(alloc), m_reservedRegs(alloc), m_allocatedRegs(alloc),
        m_degree(alloc), m_spilledRegs(alloc), m_iGraph(alloc), m_unspillableRegs(alloc)
    {
    }
};

class MirRegisterAllocator
{
  public:
    /**
     * Builds the interference graph from the given context and liveness analysis.
     */
    bool buildInterferenceGraph(LivenessResult *liveness, RegisterAllocatorCtx &ctx);

    /**
     * Simplifies the graph by removing nodes whose degree < K_class and pushing onto selectStack.
     */
    bool simplify(RegisterAllocatorCtx &ctx);

    /**
     * Assigns physical registers to nodes on the selectStack.
     * Returns false if an unresolvable spill occurred.
     */
    bool selectColors(RegisterAllocatorCtx &ctx);

    /**
     * Evaluates initial node degrees and locks physical nodes to degree infinity.
     */
    void evaluateInterferenceGraphDegree(RegisterAllocatorCtx &ctx);

    /**
     * Rewrites virtual registers to the assigned colors (physical registers).
     */
    void rewriteColors(RegisterAllocatorCtx &ctx);

  private:
    /**
     * Returns true if the given virtual register's value can be rematerialized without using the virtual register at
     * all.
     */
    bool isRematerializable(MirRegister *vreg, MirInstruction *definingInst);

    /**
     * Calculates the spill cost of a given node.
     */
    double calculateSpillCost(RegisterRef node, RegisterAllocatorCtx &ctx);

    /**
     * Adds an undirected edge between U and V.
     */
    void addEdge(const RegisterRef &u, const RegisterRef &v, RegisterAllocatorCtx &ctx);

    /**
     * Adds a node to the graph if it doesn't already exist.
     */
    void addNode(const RegisterRef &v, RegisterAllocatorCtx &ctx);

    /**
     * Rewrites instructions so spilled registers are replaced by LOAD/STORE instructions.
     */
    void rewriteSpilledRegisters(const std::pmr::unordered_set<RegisterRef> &spilledNodes, RegisterAllocatorCtx &ctx);
};

#endif // EZPACKER_REGISTERALLOCATOR_H