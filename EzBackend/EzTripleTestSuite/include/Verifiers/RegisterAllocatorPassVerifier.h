#ifndef EZPACKER_REGISTERALLOCATORPASSVERIFIER_H
#define EZPACKER_REGISTERALLOCATORPASSVERIFIER_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"

class RegisterAllocatorPassVerifier : public MirPassVerifier<MirRegisterAllocatorPass, RegisterAllocatorPassVerifier>
{
  public:
    RegisterAllocatorPassVerifier(MirBuilderContext *ctx, MirRegisterAllocatorPass *pass);

    /**
     * Asserts that no virtual register operands remain in any instruction after allocation.
     */
    RegisterAllocatorPassVerifier &verifyNoVirtualRegistersRemain(const std::pmr::list<MirBlock *> &blockList);

    /**
     * Asserts that every virtual register in the interference graph was either assigned a physical
     * register or recorded in m_spilledRegs.
     */
    RegisterAllocatorPassVerifier &verifyAllocationMappingComplete(const RegisterAllocatorCtx &allocCtx);

    /**
     * Asserts that no two interfering nodes in the graph share the same physical register color.
     */
    RegisterAllocatorPassVerifier &verifyNoInterferenceConflicts(const RegisterAllocatorCtx &allocCtx);

    /**
     * Asserts that spilled registers have valid stack slots and corresponding spill code (LOAD/STORE).
     */
    RegisterAllocatorPassVerifier &verifySpillingCorrectness(const RegisterAllocatorCtx &allocCtx,
                                                             const std::pmr::list<MirBlock *> &blockList);

    /**
     * Asserts that no virtual register was allocated to any physical register marked in m_reservedRegs.
     */
    RegisterAllocatorPassVerifier &verifyReservedRegistersNotAssigned(const RegisterAllocatorCtx &allocCtx);

    /**
     * Asserts that if m_needsFramePointer is active, the calling convention's FP register is present in m_reservedRegs.
     */
    RegisterAllocatorPassVerifier &verifyFramePointerReservedOnDAlloc(const RegisterAllocatorCtx &allocCtx);

  private:
    MirBuilderContext *m_ctx;
};

#endif // EZPACKER_REGISTERALLOCATORPASSVERIFIER_H
