#ifndef EZPACKER_EZTESTTRIPLEREGISTERALLOCATOR_H
#define EZPACKER_EZTESTTRIPLEREGISTERALLOCATOR_H

#include "EzTestTripleCommon.h"
#include "InstructionSelector/EzTestTripleInstructionSet.h"

class EzTestTripleRegisterAllocator : public MirRegisterAllocator
{
  public:
  private:
    /**
     * Returns true if the instruction is a DALLOC.
     */
    bool isInstructionDAlloc(MirInstruction *instr) override;

    /**
     * Returns true if the given virtual register's value can be rematerialized without using the virtual register at
     * all.
     *
     * This is the case if the defining instruction is:
     */
    bool isRematerializable(MirRegister *vreg, MirInstruction *definingInst) override;

    /**
     * Emits a target-specific instruction to reload a register from a stack slot.
     */
    MirInstruction *emitReload(RegisterAllocatorCtx *ctx,
                               MirBlock *block,
                               std::pmr::list<MirInstruction *>::iterator it,
                               SourceReference *srcRef,
                               MirRegister *dstReg,
                               StackFrameObject *spillSlot) override;

    /**
     * Emits a target-specific instruction to spill a register value to a stack slot.
     */
    MirInstruction *emitSpill(RegisterAllocatorCtx *ctx,
                              MirBlock *block,
                              std::pmr::list<MirInstruction *>::iterator it,
                              SourceReference *srcRef,
                              StackFrameObject *spillSlot,
                              MirRegister *srcReg) override;

    /**
     * Re-emits the defining instruction into the specified insertion point.
     */
    MirInstruction *reMaterialize(RegisterAllocatorCtx *ctx,
                                  MirBlock *block,
                                  std::pmr::list<MirInstruction *>::iterator it,
                                  SourceReference *srcRef,
                                  MirRegister *dstReg,
                                  MirInstruction *defInst) override;
};

#endif // EZPACKER_EZTESTTRIPLEREGISTERALLOCATOR_H