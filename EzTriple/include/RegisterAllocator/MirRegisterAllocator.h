#ifndef EZTRIPLE_MIR_REGISTER_ALLOCATOR_H
#define EZTRIPLE_MIR_REGISTER_ALLOCATOR_H

#include "EzTripleCommon.h"
#include "HelperClasses/DenseBitSet.h"
#include "Operand/MirRegisterClass.h"
#include "Operand/MirRegisterReference.h"
#include <algorithm>
#include <limits>
#include <optional>

/**
 * Working context for graph-coloring register allocation on a single function,
 * separating persistent cross-iteration spill state (m_persistentAlloc) from
 * ephemeral per-iteration graph/coalescing/simplification scratch state (m_scratchAlloc).
 */
struct RegisterAllocatorCtx
{
    static constexpr uint32_t InvalidNode = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t InfiniteDegree = std::numeric_limits<uint32_t>::max();

    /**
     * Ephemeral per-iteration dense interference graph and simplification state.
     * Destroyed at the end of every spill iteration so its monotonic scratch arena
     * can be reclaimed immediately without dangling memory_resource pointers.
     */
    struct ScratchState
    {
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
        std::pmr::unordered_map<MirRegisterClass *, uint32_t> m_classToIdx;
        std::pmr::vector<size_t> m_classColorLimits;
        std::pmr::vector<uint32_t> m_nodeClassIdx;
        std::pmr::unordered_map<MirRegisterRef, double> m_spillCosts;
        bool m_spillCostsValid{ false };

        explicit ScratchState(std::pmr::memory_resource *alloc) :
            m_idToReg(alloc), m_regToId(alloc), m_adjBits(alloc), m_adjList(alloc), m_degree(alloc), m_leader(alloc),
            m_removed(alloc), m_coalescedAway(alloc), m_reservedDense(alloc), m_unspillableDense(alloc),
            m_selectStack(alloc), m_affinity(alloc), m_classToIdx(alloc), m_classColorLimits(alloc),
            m_nodeClassIdx(alloc), m_spillCosts(alloc)
        {
        }
    };

    class MirBuilderContext *m_ctx;      ///< Shared builder context used when rewriting registers.
    class MirFunction *m_targetFunction; ///< Function whose registers are being allocated.
    class TargetDesc *m_targetDesc;      ///< Target supplying register classes and spill hooks.

    std::pmr::memory_resource *m_persistentAlloc; ///< Arena for cross-iteration & pass-result state.
    std::pmr::memory_resource *m_scratchAlloc;    ///< Arena for current iteration's ephemeral graph state.

    // Persistent across spill iterations
    std::pmr::unordered_set<MirRegisterRef> m_reservedRegs;
    std::pmr::unordered_map<MirRegisterRef, MirRegisterRef> m_allocatedRegs;
    std::pmr::unordered_map<MirRegisterRef, class StackFrameObject *> m_spilledRegs;
    std::pmr::unordered_set<MirRegisterRef> m_unspillableRegs;
    bool m_coalescingEnabled{ true };

    // Ephemeral per-iteration state (scoped to m_scratchAlloc)
    std::optional<ScratchState> m_scratch;

    /**
     * Begins a new allocation iteration backed by scratchAlloc, clearing m_allocatedRegs.
     */
    void beginIteration(std::pmr::memory_resource *scratchAlloc)
    {
        m_allocatedRegs.clear();
        m_scratch.reset();
        m_scratchAlloc = scratchAlloc ? scratchAlloc : m_persistentAlloc;
        m_scratch.emplace(m_scratchAlloc);
    }

    /**
     * Destroys all ephemeral per-iteration containers before the iteration's scratch arena is released.
     */
    void endIteration()
    {
        m_scratch.reset();
        m_scratchAlloc = m_persistentAlloc;
    }

    /**
     * Resets per-iteration state in-place using the current scratch allocator (for standalone callers).
     */
    void resetIterationState() { beginIteration(m_scratchAlloc); }

    /**
     * Ensures capacity for at least targetCap dense nodes across all bitsets.
     */
    void ensureNodeCapacity(size_t targetCap)
    {
        if (!m_scratch || targetCap <= m_scratch->m_adjCapacity)
            return;

        size_t newCap = std::max<size_t>(64, m_scratch->m_adjCapacity * 2);
        while (newCap < targetCap)
            newCap *= 2;

        m_scratch->m_adjCapacity = newCap;
        m_scratch->m_removed.resize(newCap);
        m_scratch->m_coalescedAway.resize(newCap);
        m_scratch->m_reservedDense.resize(newCap);
        m_scratch->m_unspillableDense.resize(newCap);
        for (auto &bits : m_scratch->m_adjBits)
            bits.resize(newCap);
    }

    /**
     * Returns the dense node ID for reg, inserting it into the graph if not yet present.
     */
    uint32_t getOrAddNode(MirRegisterRef reg)
    {
        auto &s = *m_scratch;
        auto [it, inserted] = s.m_regToId.try_emplace(reg, static_cast<uint32_t>(s.m_idToReg.size()));
        if (!inserted)
            return it->second;

        const uint32_t id = it->second;
        ensureNodeCapacity(id + 1);

        s.m_idToReg.push_back(reg);
        s.m_adjBits.emplace_back(s.m_adjCapacity);
        s.m_adjList.emplace_back();
        s.m_degree.push_back(0);
        s.m_leader.push_back(id);
        s.m_affinity.emplace_back();
        s.m_nodeClassIdx.push_back(getOrAddClassIdx(reg.getClass()));

        if (m_reservedRegs.contains(reg))
            s.m_reservedDense.set(id);
        if (m_unspillableRegs.contains(reg))
            s.m_unspillableDense.set(id);

        return id;
    }

    /**
     * Returns the dense node ID for reg, or InvalidNode if absent.
     */
    [[nodiscard]] uint32_t findNode(MirRegisterRef reg) const
    {
        if (!m_scratch)
            return InvalidNode;
        auto it = m_scratch->m_regToId.find(reg);
        return it != m_scratch->m_regToId.end() ? it->second : InvalidNode;
    }

    [[nodiscard]] bool hasNode(MirRegisterRef reg) const { return findNode(reg) != InvalidNode; }

    [[nodiscard]] bool hasEdge(MirRegisterRef a, MirRegisterRef b) const
    {
        uint32_t idA = findNode(a);
        uint32_t idB = findNode(b);
        if (idA == InvalidNode || idB == InvalidNode)
            return false;
        return m_scratch->m_adjBits[idA].test(idB);
    }

    /**
     * Adds an undirected interference edge (a, b), enforcing symmetry, no self-edges, and no duplicates.
     */
    void addEdge(uint32_t a, uint32_t b)
    {
        auto &s = *m_scratch;
        if (a == b || s.m_adjBits[a].test(b))
            return;
        s.m_adjBits[a].set(b);
        s.m_adjBits[b].set(a);
        s.m_adjList[a].push_back(b);
        s.m_adjList[b].push_back(a);
    }

    /**
     * Removes an undirected interference edge (a, b) symmetrically.
     */
    void removeEdge(uint32_t a, uint32_t b)
    {
        auto &s = *m_scratch;
        if (!s.m_adjBits[a].test(b))
            return;
        s.m_adjBits[a].clearBit(b);
        s.m_adjBits[b].clearBit(a);
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
        swapErase(s.m_adjList[a], b);
        swapErase(s.m_adjList[b], a);
    }

    [[nodiscard]] size_t getNeighborCount(MirRegisterRef reg) const
    {
        uint32_t id = findNode(reg);
        return id != InvalidNode ? m_scratch->m_adjList[id].size() : 0;
    }

    [[nodiscard]] size_t getDegree(MirRegisterRef reg) const
    {
        uint32_t id = findNode(reg);
        return id != InvalidNode ? m_scratch->m_degree[id] : 0;
    }

    void pushSelectStack(uint32_t id) { m_scratch->m_selectStack.push_back(id); }

    [[nodiscard]] size_t getSelectStackSize() const { return m_scratch ? m_scratch->m_selectStack.size() : 0; }

    void addAffinity(MirRegisterRef a, MirRegisterRef b)
    {
        uint32_t idA = getOrAddNode(a);
        uint32_t idB = getOrAddNode(b);
        if (idA == idB)
            return;
        auto &affA = m_scratch->m_affinity[idA];
        if (std::find(affA.begin(), affA.end(), idB) == affA.end())
            affA.push_back(idB);
        auto &affB = m_scratch->m_affinity[idB];
        if (std::find(affB.begin(), affB.end(), idA) == affB.end())
            affB.push_back(idA);
    }

    uint32_t getOrAddClassIdx(MirRegisterClass *cls)
    {
        auto &s = *m_scratch;
        auto [it, inserted] = s.m_classToIdx.try_emplace(cls, static_cast<uint32_t>(s.m_classColorLimits.size()));
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
        s.m_classColorLimits.push_back(usableCount);
        return it->second;
    }

    [[nodiscard]] size_t getColorLimit(MirRegisterClass *cls)
    {
        uint32_t idx = getOrAddClassIdx(cls);
        return m_scratch->m_classColorLimits[idx];
    }

    [[nodiscard]] uint32_t getLeaderId(uint32_t id) const
    {
        if (!m_scratch)
            return id;
        const auto &leader = m_scratch->m_leader;
        while (id < leader.size() && leader[id] != id)
            id = leader[id];
        return id;
    }

    [[nodiscard]] MirRegisterRef getCoalescedLeader(MirRegisterRef reg) const
    {
        uint32_t id = findNode(reg);
        if (id == InvalidNode)
            return reg;
        return m_scratch->m_idToReg[getLeaderId(id)];
    }

    explicit RegisterAllocatorCtx(class MirBuilderContext *ctx,
                                  class MirFunction *targetFunction,
                                  class TargetDesc *targetDesc,
                                  std::pmr::memory_resource *persistentAlloc,
                                  std::pmr::memory_resource *scratchAlloc = nullptr) :
        m_ctx(ctx), m_targetFunction(targetFunction), m_targetDesc(targetDesc), m_persistentAlloc(persistentAlloc),
        m_scratchAlloc(scratchAlloc ? scratchAlloc : persistentAlloc), m_reservedRegs(persistentAlloc),
        m_allocatedRegs(persistentAlloc), m_spilledRegs(persistentAlloc), m_unspillableRegs(persistentAlloc)
    {
        m_scratch.emplace(m_scratchAlloc);
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
     * Populates lightweight MOV affinity hints without modifying the interference graph or coalescing
     * nodes (used at -O0 so selectColors can still bias trivial copies deterministically).
     */
    void populateMoveAffinityHints(class RegisterAllocatorCtx *ctx);

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