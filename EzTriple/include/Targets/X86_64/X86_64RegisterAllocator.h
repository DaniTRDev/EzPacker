#ifndef EZTRIPLE_X86_64_REGISTER_ALLOCATOR_H
#define EZTRIPLE_X86_64_REGISTER_ALLOCATOR_H

#include "EzTripleCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"
#include "RegisterAllocator/MirRegisterAllocator.h"

class MirInstruction;
class MirRegister;
class MirBlock;
class SourceReference;
class StackFrameObject;
struct RegisterAllocatorCtx;

namespace EzTriple
{

/**
 * Standard x86-64 register allocator implementing Chaitin-Briggs graph coloring,
 * spill generation, and constant rematerialization.
 */
class X86_64RegisterAllocator : public MirRegisterAllocator
{
  public:
    X86_64RegisterAllocator() = default;
    ~X86_64RegisterAllocator() override = default;

  protected:
    /// Returns true when the instruction is a DALLOC that pins the frame pointer.
    bool isInstructionDAlloc(MirInstruction *instr) override;

    /// Returns true when the value produced by vreg can be recomputed instead of spilled.
    bool isRematerializable(MirRegister *vreg, MirInstruction *definingInst) override;

    /// Emits a MOV loading dstReg from the spill slot at the given insertion point.
    MirInstruction *emitReload(RegisterAllocatorCtx *ctx,
                               MirBlock *block,
                               IntrusiveLinkedList<MirInstruction>::iterator it,
                               SourceReference *srcRef,
                               MirRegister *dstReg,
                               StackFrameObject *spillSlot) override;

    /// Emits a MOV storing srcReg into the spill slot at the given insertion point.
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

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_REGISTER_ALLOCATOR_H
