#ifndef EZPACKER_REGISTERALLOCATOR_H
#define EZPACKER_REGISTERALLOCATOR_H

#include "EzTriple.h"

struct RegisterAllocatorCtx
{
    CallingConvDesc *m_callignConv;
    MirBuilderContext *m_ctx;
    /**
     * Interference graph. For each tuple:
     *  - First = Node V
     *  - Second = Edges. An edge represents that register V and U are alive in the same interval.
     */
    std::pmr::map<RegisterRef, std::pmr::set<RegisterRef>> m_iGraph;

    // First = register. Second = physical register.
    std::pmr::map<RegisterRef, RegisterRef> m_allocatedRegs;

    // First = virtual register. Second = spilled physical place.
    std::pmr::map<RegisterRef, StackFrameObject *> m_spilledRegs;
};

class RegisterAllocator
{
  public:
    /**
     * Builds the inteference graph out of the given blocklist and liveness analysis. Returns true if succeeded, false
     * otherways.
     */
    bool
    buildInterferenceGraph(LivenessResult *liveness, RegisterAllocatorCtx &ctx, std::pmr::list<MirBlock *> &blockList);

  private:
    /**
     * Adds an edge from U to V. This means that when V is alive, U is too.
     */
    void addEdge(const RegisterRef &u, const RegisterRef &v, RegisterAllocatorCtx &ctx);

    /**
     * Adds a node to the graph, if not already added.
     */
    void addNode(const RegisterRef &v, RegisterAllocatorCtx &ctx);
};

#endif // EZPACKER_REGISTERALLOCATOR_H
