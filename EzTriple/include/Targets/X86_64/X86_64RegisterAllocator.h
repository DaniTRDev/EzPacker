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
    bool isInstructionDAlloc(MirInstruction *instr) override;
    bool isRematerializable(MirRegister *vreg, MirInstruction *definingInst) override;

    MirInstruction *emitReload(RegisterAllocatorCtx *ctx,
                               MirBlock *block,
                               IntrusiveLinkedList<MirInstruction>::iterator it,
                               SourceReference *srcRef,
                               MirRegister *dstReg,
                               StackFrameObject *spillSlot) override;

    MirInstruction *emitSpill(RegisterAllocatorCtx *ctx,
                              MirBlock *block,
                              IntrusiveLinkedList<MirInstruction>::iterator it,
                              SourceReference *srcRef,
                              StackFrameObject *spillSlot,
                              MirRegister *srcReg) override;

    MirInstruction *reMaterialize(RegisterAllocatorCtx *ctx,
                                  MirBlock *block,
                                  IntrusiveLinkedList<MirInstruction>::iterator it,
                                  SourceReference *srcRef,
                                  MirRegister *dstReg,
                                  MirInstruction *defInst) override;
};

} // namespace EzTriple

#endif // EZTRIPLE_X86_64_REGISTER_ALLOCATOR_H
