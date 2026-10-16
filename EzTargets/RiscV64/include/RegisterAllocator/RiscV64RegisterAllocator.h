#ifndef EZTARGETS_RISCV64_REGISTER_ALLOCATOR_H
#define EZTARGETS_RISCV64_REGISTER_ALLOCATOR_H

#include "EzTargetsRiscV64Common.h"
#include "HelperClasses/IntrusiveLinkedList.h"
#include "RegisterAllocator/MirRegisterAllocator.h"

class MirInstruction;
class MirRegister;
class MirBlock;
class SourceReference;
class StackFrameObject;
struct RegisterAllocatorCtx;

namespace EzTargets::RiscV64
{

/**
 * RISC-V 64 register allocator implementing graph coloring, spill generation,
 * and rematerialization.
 */
class RiscV64RegisterAllocator : public MirRegisterAllocator
{
  public:
    RiscV64RegisterAllocator() = default;
    ~RiscV64RegisterAllocator() override = default;

  protected:
    /// Returns true when the instruction is a DALLOC that pins the frame pointer.
    bool isInstructionDAlloc(MirInstruction *instr) override;

    /// Returns true when the value produced by vreg can be recomputed instead of spilled.
    bool isRematerializable(MirRegister *vreg, MirInstruction *definingInst) override;

    /// Emits a LOAD restoring dstReg from the spill slot.
    MirInstruction *emitReload(RegisterAllocatorCtx *ctx,
                               MirBlock *block,
                               IntrusiveLinkedList<MirInstruction>::iterator it,
                               SourceReference *srcRef,
                               MirRegister *dstReg,
                               StackFrameObject *spillSlot) override;

    /// Emits a STORE writing srcReg into the spill slot.
    MirInstruction *emitSpill(RegisterAllocatorCtx *ctx,
                              MirBlock *block,
                              IntrusiveLinkedList<MirInstruction>::iterator it,
                              SourceReference *srcRef,
                              StackFrameObject *spillSlot,
                              MirRegister *srcReg) override;

    /// Re-emits the defining instruction of a rematerializable value at the given insertion point.
    MirInstruction *reMaterialize(RegisterAllocatorCtx *ctx,
                                  MirBlock *block,
                                  IntrusiveLinkedList<MirInstruction>::iterator it,
                                  SourceReference *srcRef,
                                  MirRegister *dstReg,
                                  MirInstruction *defInst) override;
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_REGISTER_ALLOCATOR_H
