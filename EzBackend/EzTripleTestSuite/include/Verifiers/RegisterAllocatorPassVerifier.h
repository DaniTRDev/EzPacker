#ifndef EZPACKER_REGISTERALLOCATORPASSVERIFIER_H
#define EZPACKER_REGISTERALLOCATORPASSVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class RegisterAllocatorPassVerifier : public MirPassVerifier<MirRegisterAllocatorPass, RegisterAllocatorPassVerifier>
{
  public:
    RegisterAllocatorPassVerifier(MirBuilderContext *ctx, MirRegisterAllocatorPass *pass);

    /**
     * Verifies that after the register allocation pass runs, no virtual register operands
     * remain across any instructions in the provided blocks.
     *
     * @param blockList The blocks to scan.
     * @return Reference to self for method chaining.
     */
    RegisterAllocatorPassVerifier &verifyNoVirtualRegistersRemain(const std::pmr::list<MirBlock *> &blockList);

    /**
     * Verifies that all virtual register nodes in the context were properly mapped to valid
     * physical registers or assigned a StackFrameObject if spilled.
     *
     * @param allocCtx The context generated during register allocation.
     * @return Reference to self for method chaining.
     */
    RegisterAllocatorPassVerifier &verifyAllocationMappingComplete(const RegisterAllocatorCtx &allocCtx);

    /**
     * Verifies that no two interfering virtual registers (sharing an edge in the interference graph)
     * were assigned the same physical register color.
     *
     * @param allocCtx The context generated during register allocation.
     * @return Reference to self for method chaining.
     */
    RegisterAllocatorPassVerifier &verifyNoInterferenceConflicts(const RegisterAllocatorCtx &allocCtx);

    /**
     * Verifies that spilled virtual registers have non-null StackFrameObject slots and that corresponding
     * LOAD (reload) or STORE (spill) instructions exist in the target blocks.
     *
     * @param allocCtx The context generated during register allocation.
     * @param blockList The blocks containing the lowered spill code.
     * @return Reference to self for method chaining.
     */
    RegisterAllocatorPassVerifier &verifySpillingCorrectness(const RegisterAllocatorCtx &allocCtx,
                                                             const std::pmr::list<MirBlock *> &blockList);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_REGISTERALLOCATORPASSVERIFIER_H
