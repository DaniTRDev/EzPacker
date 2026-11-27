#ifndef EZTRIPLE_MIR_REGISTER_ALLOCATOR_H
#define EZTRIPLE_MIR_REGISTER_ALLOCATOR_H

#include "EzTripleCommon.h"
#include "HelperClasses/DenseBitSet.h"
#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterReference.h"
#include <algorithm>
#include <limits>

/**
 * Working context for graph-coloring register allocation on a single function,
 * containing the dense-indexed interference graph, degree tables, select stack, and spill tracking.
 */
struct RegisterAllocatorCtx
{
    static constexpr uint32_t InvalidNode = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t InfiniteDegree = std::numeric_limits<uint32_t>::max();

    class MirBuilderContext *m_ctx;      ///< Shared builder context used when rewriting registers.
    class MirFunction *m_targetFunction; ///< Function whose registers are being allocated.
    class TargetDesc *m_targetDesc;      ///< Target supplying register classes and spill hooks.

    // Explicit PMR allocator handle for context containers
    std::pmr::memory_resource *m_allocator;

    // A set of registers that are reserved and CAN'T be used during allocation.
    std::pmr::unordered_set<MirRegisterRef> m_reservedRegs;

    // First = Virtual Register. Second = Assigned Physical Register.
    std::pmr::unordered_map<MirRegisterRef, MirRegisterRef> m_allocatedRegs;

    // First = Register. Second = Assigned stack slot if spilled.
    std::pmr::unordered_map<MirRegisterRef, class StackFrameObject *> m_spilledRegs;

    // Set of registers that can't be optimistically spilled.
    std::pmr::unordered_set<MirRegisterRef> m_unspillableRegs;

    // Cached per-register spill costs, rebuilt once per interference-graph construction.
    std::pmr::unordered_map<MirRegisterRef, double> m_spillCosts;
    bool m_spillCostsValid{ false };

    // Controls whether conservative coalescing is executed during allocation.
    bool m_coalescingEnabled{ true };

    // -------------------------------------------------------------------------
    // Dense-indexed interference graph & per-iteration allocation state
    // -------------------------------------------------------------------------
    size_t m_adjCapacity{ 0 };
    std::pmr::vector<MirRegisterRef> m_idToReg;
    std::pmr::unordered_map<MirRegisterRef, uint32_t> m_regToId;
    std::pmr::vector<DenseBitSet> m_adjBits;
    std::pmr::vector<std::pmr::vector<uint32_t>> m_adjList;
    std::pmr::vector<uint32_t> m_degree;
    std::pmr::vector<uint32_t> m_leader;
    DenseBitSet m_removed;
    DenseBitSet m_coalescedAway;
    DenseBitSet m_reservedDense;
    DenseBitSet m_unspillableDense;
    std::pmr::vector<uint32_t> m_selectStack;
    std::pmr::vector<std::pmr::vector<uint32_t>> m_affinity;

    // Cached per-register-class color limits for this function.
    std::pmr::unordered_map<MirRegisterClass *, uint32_t> m_classToIdx;
    std::pmr::vector<size_t> m_classColorLimits;
    std::pmr::vector<uint32_t> m_nodeClassIdx;

    /**
     * Clears all per-iteration graph, coalescing, and coloring state while preserving
     * cross-iteration spill tracking (m_spilledRegs, m_unspillableRegs).
     */
    void resetIterationState()
    {
        m_adjCapacity = 0;
        m_idToReg.clear();
        m_regToId.clear();
        m_adjBits.clear();
        m_adjList.clear();
        m_degree.clear();
        m_leader.clear();
        m_removed.reset();
        m_coalescedAway.reset();
        m_reservedDense.reset();
        m_unspillableDense.reset();
        m_selectStack.clear();
        m_affinity.clear();
        m_allocatedRegs.clear();
        m_classToIdx.clear();
        m_classColorLimits.clear();
        m_nodeClassIdx.clear();
        m_spillCostsValid = false;
    }

    /**
     * Ensures capacity for at least targetCap dense nodes across all bitsets.
     */
    void ensureNodeCapacity(size_t targetCap)
    {
        if (targetCap <= m_adjCapacity)
            return;

        size_t newCap = std::max<size_t>(64, m_adjCapacity * 2);
        while (newCap < targetCap)
            newCap *= 2;

        m_adjCapacity = newCap;
        m_removed.resize(newCap);
        m_coalescedAway.resize(newCap);
        m_reservedDense.resize(newCap);
        m_unspillableDense.resize(newCap);
        for (auto &bits : m_adjBits)
            bits.resize(newCap);
    }

    /**
     * Returns the dense node ID for reg, inserting it into the graph if not yet present.
     */
    uint32_t getOrAddNode(MirRegisterRef reg)
    {
        auto [it, inserted] = m_regToId.try_emplace(reg, static_cast<uint32_t>(m_idToReg.size()));
        if (!inserted)
            return it->second;

        const uint32_t id = it->second;
        ensureNodeCapacity(id + 1);

        m_idToReg.push_back(reg);
        m_adjBits.emplace_back(m_adjCapacity);
        m_adjList.emplace_back();
        m_degree.push_back(0);
        m_leader.push_back(id);
        m_affinity.emplace_back();
        m_nodeClassIdx.push_back(getOrAddClassIdx(reg.getClass()));

        if (m_reservedRegs.contains(reg))
            m_reservedDense.set(id);
        if (m_unspillableRegs.contains(reg))
            m_unspillableDense.set(id);

        return id;
    }

    /**
     * Returns the dense node ID for reg, or InvalidNode if absent.
     */
    [[nodiscard]] uint32_t findNode(MirRegisterRef reg) const
    {
        auto it = m_regToId.find(reg);
        return it != m_regToId.end() ? it->second : InvalidNode;
    }

    [[nodiscard]] bool hasNode(MirRegisterRef reg) const { return findNode(reg) != InvalidNode; }

    [[nodiscard]] bool hasEdge(MirRegisterRef a, MirRegisterRef b) const
    {
        uint32_t idA = findNode(a);
        uint32_t idB = findNode(b);
        if (idA == InvalidNode || idB == InvalidNode)
            return false;
        return m_adjBits[idA].test(idB);
    }

    /**
     * Adds an undirected interference edge (a, b), enforcing symmetry, no self-edges, and no duplicates.
     */
    void addEdge(uint32_t a, uint32_t b)
    {
        if (a == b || m_adjBits[a].test(b))
            return;
        m_adjBits[a].set(b);
        m_adjBits[b].set(a);
        m_adjList[a].push_back(b);
        m_adjList[b].push_back(a);
    }

    /**
     * Removes an undirected interference edge (a, b) symmetrically.
     */
    void removeEdge(uint32_t a, uint32_t b)
    {
        if (!m_adjBits[a].test(b))
            return;
        m_adjBits[a].clearBit(b);
        m_adjBits[b].clearBit(a);
        auto swapErase = [](std::pmr::vector<uint32_t> &vec, uint32_t target)
        {
            for (size_t i = 0; i < vec.size(); ++i)
            {
                if (vec[i] == target)
                {
                    vec[i] = vec.back();
                    vec.pop_back();
                    return;
                }
            }
        };
        swapErase(m_adjList[a], b);
        swapErase(m_adjList[b], a);
    }

    [[nodiscard]] size_t getNeighborCount(MirRegisterRef reg) const
    {
        uint32_t id = findNode(reg);
        return id != InvalidNode ? m_adjList[id].size() : 0;
    }

    [[nodiscard]] size_t getDegree(MirRegisterRef reg) const
    {
        uint32_t id = findNode(reg);
        return id != InvalidNode ? m_degree[id] : 0;
    }

    void addAffinity(MirRegisterRef a, MirRegisterRef b)
    {
        uint32_t idA = getOrAddNode(a);
        uint32_t idB = getOrAddNode(b);
        if (idA == idB)
            return;
        auto &affA = m_affinity[idA];
        if (std::find(affA.begin(), affA.end(), idB) == affA.end())
            affA.push_back(idB);
        auto &affB = m_affinity[idB];
        if (std::find(affB.begin(), affB.end(), idA) == affB.end())
            affB.push_back(idA);
    }

    uint32_t getOrAddClassIdx(MirRegisterClass *cls)
    {
        auto [it, inserted] = m_classToIdx.try_emplace(cls, static_cast<uint32_t>(m_classColorLimits.size()));
        if (!inserted)
            return it->second;

        size_t usableCount = 0;
        if (cls)
        {
            for (const auto &[name, regDesc] : cls->getRegs())
            {
                MirRegisterRef physReg = MirRegisterRef::preg(regDesc);
                if (!m_reservedRegs.contains(physReg))
                    usableCount++;
            }
        }
        m_classColorLimits.push_back(usableCount);
        return it->second;
    }

    [[nodiscard]] size_t getColorLimit(MirRegisterClass *cls)
    {
        uint32_t idx = getOrAddClassIdx(cls);
        return m_classColorLimits[idx];
    }

    [[nodiscard]] uint32_t getLeaderId(uint32_t id) const
    {
        while (id < m_leader.size() && m_leader[id] != id)
            id = m_leader[id];
        return id;
    }

    [[nodiscard]] MirRegisterRef getCoalescedLeader(MirRegisterRef reg) const
    {
        uint32_t id = findNode(reg);
        if (id == InvalidNode)
            return reg;
        return m_idToReg[getLeaderId(id)];
    }

    explicit RegisterAllocatorCtx(class MirBuilderContext *ctx,
                                  class MirFunction *targetFunction,
                                  class TargetDesc *targetDesc,
                                  std::pmr::memory_resource *alloc) :
        m_ctx(ctx), m_targetFunction(targetFunction), m_targetDesc(targetDesc), m_allocator(alloc),
        m_reservedRegs(alloc), m_allocatedRegs(alloc), m_spilledRegs(alloc), m_unspillableRegs(alloc),
        m_spillCosts(alloc), m_idToReg(alloc), m_regToId(alloc), m_adjBits(alloc), m_adjList(alloc), m_degree(alloc),
        m_leader(alloc), m_removed(alloc), m_coalescedAway(alloc), m_reservedDense(alloc), m_unspillableDense(alloc),
        m_selectStack(alloc), m_affinity(alloc), m_classToIdx(alloc), m_classColorLimits(alloc),
        m_nodeClassIdx(alloc)
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
    virtual ~MirRegisterAllocator() = default;

    /**
     * Builds the interference graph from the given context and liveness analysis.
     */
    bool buildInterferenceGraph(class LivenessResult *liveness, class RegisterAllocatorCtx *ctx);

    /**
     * Conservatively coalesces copy-related and two-address virtual/physical registers
     * using Briggs and George heuristics. Returns true if any registers were coalesced.
     */
    bool coalesce(class RegisterAllocatorCtx *ctx);

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

    /**
     * Erases redundant identity copy instructions (MOV r, r) produced by register coalescing and allocation.
     */
    void eliminateRedundantCopies(class RegisterAllocatorCtx *ctx);

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
     * Adds a node to the dense interference graph if it doesn't already exist and returns its dense ID.
     */
    uint32_t addNode(const MirRegisterRef &v, class RegisterAllocatorCtx *ctx);

    /**
     * Rewrites instructions so spilled registers are replaced by LOAD/STORE instructions.
     */
    void rewriteSpilledRegisters(const std::pmr::unordered_set<MirRegisterRef> &spilledNodes,
                                 class RegisterAllocatorCtx *ctx);
};

#endif // EZTRIPLE_MIR_REGISTER_ALLOCATOR_H