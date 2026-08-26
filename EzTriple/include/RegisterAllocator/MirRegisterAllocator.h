#ifndef EZTRIPLE_MIR_REGISTER_ALLOCATOR_H
#define EZTRIPLE_MIR_REGISTER_ALLOCATOR_H

#include "EzTripleCommon.h"
#include "Operand/MirRegisterReference.h"

/**
 * Working context for graph-coloring register allocation on a single function,
 * containing the interference graph, degree tables, select stack, and spill tracking.
 */
struct RegisterAllocatorCtx
{
    class MirBuilderContext *m_ctx;
    class MirFunction *m_targetFunction;
    class TargetDesc *m_targetDesc;

    // Explicit PMR allocator handle for context containers
    std::pmr::memory_resource *m_allocator;

    /**
     * Stack of nodes removed during simplify, popped in reverse during select phase.
     */
    std::pmr::vector<MirRegisterRef> m_selectStack;

    /**
     * Nodes currently removed from the active graph during simplification.
     */
    std::pmr::unordered_set<MirRegisterRef> m_removedNodes;

    // A set of registers that are reserved and CAN'T be used during allocation.
    std::pmr::unordered_set<MirRegisterRef> m_reservedRegs;

    // First = Virtual Register. Second = Assigned Physical Register.
    std::pmr::unordered_map<MirRegisterRef, MirRegisterRef> m_allocatedRegs;

    // First = Register. Second = Number of active interference neighbors.
    std::pmr::unordered_map<MirRegisterRef, size_t> m_degree;

    // First = Register. Second = Assigned stack slot if spilled.
    std::pmr::unordered_map<MirRegisterRef, class StackFrameObject *> m_spilledRegs;

    /**
     * Interference graph.
     *  - First = Node V
     *  - Second = Set of interfering neighbor registers.
     */
    std::pmr::unordered_map<MirRegisterRef, std::pmr::set<MirRegisterRef>> m_iGraph;

    // Set of registers that can't be optimistically spilled.
    std::pmr::unordered_set<MirRegisterRef> m_unspillableRegs;

    explicit RegisterAllocatorCtx(class MirBuilderContext *ctx,
                                  class MirFunction *targetFunction,
                                  class TargetDesc *targetDesc,
                                  std::pmr::memory_resource *alloc) :
        m_ctx(ctx), m_targetFunction(targetFunction), m_targetDesc(targetDesc), m_allocator(alloc),
        m_selectStack(alloc), m_removedNodes(alloc), m_reservedRegs(alloc), m_allocatedRegs(alloc), m_degree(alloc),
        m_spilledRegs(alloc), m_iGraph(alloc), m_unspillableRegs(alloc)
    {
    }
};

/**
 * Graph-coloring register allocator implementing Chaitin-Briggs style simplification,
 * coloring selection, spill cost estimation, and register rewriting.
 */
class MirRegisterAllocator
{
  public:
    /**
     * Builds the interference graph from the given context and liveness analysis.
     */
    bool buildInterferenceGraph(class LivenessResult *liveness, class RegisterAllocatorCtx *ctx);

    /**
     * Simplifies the graph by removing nodes whose degree < K_class and pushing onto selectStack.
     */
    bool simplify(class RegisterAllocatorCtx *ctx);

    /**
     * Assigns physical registers to nodes on the selectStack.
     * Returns false if an unresolvable spill occurred.
     */
    bool selectColors(class RegisterAllocatorCtx *ctx);

    /**
     * Evaluates initial node degrees and locks physical nodes to degree infinity.
     */
    void evaluateInterferenceGraphDegree(class RegisterAllocatorCtx *ctx);

    /**
     * Rewrites virtual registers to the assigned colors (physical registers).
     */
    void rewriteColors(class RegisterAllocatorCtx *ctx);

  protected:
    /**
     * Returns true if the instruction is a DALLOC.
     */
    virtual bool isInstructionDAlloc(class MirInstruction *instr) = 0;

    /**
     * Returns true if the given virtual register's value can be rematerialized without using the virtual register at
     * all.
     */
    virtual bool isRematerializable(class MirRegister *vreg, class MirInstruction *definingInst) = 0;

    /**
     * Calculates the spill cost of a given node.
     */
    double calculateSpillCost(MirRegisterRef node, class RegisterAllocatorCtx *ctx);

    /**
     * Emits a target-specific instruction to reload a register from a stack slot.
     */
    virtual MirInstruction *emitReload(class RegisterAllocatorCtx *ctx,
                                       class MirBlock *block,
                                       IntrusiveLinkedList<class MirInstruction>::iterator it,
                                       class SourceReference *srcRef,
                                       class MirRegister *dstReg,
                                       class StackFrameObject *spillSlot) = 0;

    /**
     * Emits a target-specific instruction to spill a register value to a stack slot.
     */
    virtual MirInstruction *emitSpill(class RegisterAllocatorCtx *ctx,
                                      class MirBlock *block,
                                      IntrusiveLinkedList<class MirInstruction>::iterator it,
                                      class SourceReference *srcRef,
                                      class StackFrameObject *spillSlot,
                                      class MirRegister *srcReg) = 0;

    /**
     * Re-emits the defining instruction into the specified insertion point.
     */
    virtual class MirInstruction *reMaterialize(class RegisterAllocatorCtx *ctx,
                                                class MirBlock *block,
                                                IntrusiveLinkedList<class MirInstruction>::iterator it,
                                                class SourceReference *srcRef,
                                                class MirRegister *dstReg,
                                                class MirInstruction *defInst) = 0;

    /**
     * Adds an undirected edge between U and V.
     */
    void addEdge(const MirRegisterRef &u, const MirRegisterRef &v, class RegisterAllocatorCtx *ctx);

    /**
     * Adds a node to the graph if it doesn't already exist.
     */
    void addNode(const MirRegisterRef &v, class RegisterAllocatorCtx *ctx);

    /**
     * Rewrites instructions so spilled registers are replaced by LOAD/STORE instructions.
     */
    void rewriteSpilledRegisters(const std::pmr::unordered_set<MirRegisterRef> &spilledNodes,
                                 class RegisterAllocatorCtx *ctx);
};

#endif // EZTRIPLE_MIR_REGISTER_ALLOCATOR_H