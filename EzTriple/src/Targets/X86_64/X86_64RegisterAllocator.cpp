#include "Targets/X86_64/X86_64RegisterAllocator.h"
#include "Builder/MirBuilderContext.h"
#include "Block/MirBlock.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"

namespace EzTriple
{

bool X86_64RegisterAllocator::isInstructionDAlloc(MirInstruction *instr)
{
    return instr && instr->getOpCode() == MirInstructionOpCode::DALLOC;
}

bool X86_64RegisterAllocator::isRematerializable(MirRegister *vreg, MirInstruction *definingInst)
{
    if (!definingInst)
    {
        return false;
    }

    if (definingInst->getOpCode() == MirInstructionOpCode::MOV && definingInst->getOperands().size() >= 2)
    {
        auto *srcOp = definingInst->getOperands()[1];
        return srcOp && (srcOp->getType() == MirOperandType::Integer ||
                         srcOp->getType() == MirOperandType::FloatingPoint);
    }
    return false;
}

MirInstruction *X86_64RegisterAllocator::emitReload(RegisterAllocatorCtx *ctx,
                                                    MirBlock *block,
                                                    IntrusiveLinkedList<MirInstruction>::iterator it,
                                                    SourceReference *srcRef,
                                                    MirRegister *dstReg,
                                                    StackFrameObject *spillSlot)
{
    MirOperandBuilder opBuilder(ctx->m_ctx);
    MirInstructionBuilder iBuilder(ctx->m_ctx, block, InsertionType::InsertBefore, it);
    MirReference *slotRef = opBuilder.buildRef(spillSlot, srcRef);
    return iBuilder.LOAD(srcRef, dstReg, slotRef);
}

MirInstruction *X86_64RegisterAllocator::emitSpill(RegisterAllocatorCtx *ctx,
                                                   MirBlock *block,
                                                   IntrusiveLinkedList<MirInstruction>::iterator it,
                                                   SourceReference *srcRef,
                                                   StackFrameObject *spillSlot,
                                                   MirRegister *srcReg)
{
    MirOperandBuilder opBuilder(ctx->m_ctx);
    MirInstructionBuilder iBuilder(ctx->m_ctx, block, InsertionType::InsertAfter, it);
    MirReference *slotRef = opBuilder.buildRef(spillSlot, srcRef);
    return iBuilder.STORE(srcRef, slotRef, srcReg);
}

MirInstruction *X86_64RegisterAllocator::reMaterialize(RegisterAllocatorCtx *ctx,
                                                       MirBlock *block,
                                                       IntrusiveLinkedList<MirInstruction>::iterator it,
                                                       SourceReference *srcRef,
                                                       MirRegister *dstReg,
                                                       MirInstruction *defInst)
{
    MirInstructionBuilder iBuilder(ctx->m_ctx, block, InsertionType::InsertBefore, it);
    std::pmr::vector<MirOperand *> ops(ctx->m_allocator);
    ops.push_back(dstReg);
    for (size_t i = 1; i < defInst->getOperands().size(); ++i)
    {
        ops.push_back(defInst->getOperands()[i]);
    }
    return iBuilder.build(defInst->getOpCode(), srcRef, ops);
}

} // namespace EzTriple
