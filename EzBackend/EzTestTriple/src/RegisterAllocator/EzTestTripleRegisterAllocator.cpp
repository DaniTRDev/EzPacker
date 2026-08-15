#include "RegisterAllocator/EzTestTripleRegisterAllocator.h"

bool EzTestTripleRegisterAllocator::isInstructionDAlloc(MirInstruction *instr)
{
    using namespace EzTestTriple;
    return instr && instr->getTargetDesc() == TargetInst::DYNAMIC_ALLOC;
}

bool EzTestTripleRegisterAllocator::isRematerializable(MirRegister *vreg, MirInstruction *definingInst)
{
    using namespace EzTestTriple;

    if (!vreg || !definingInst)
        return false;

    const MirTargetInstructionDesc *desc = definingInst->getTargetDesc();
    if (!desc)
        return false;

    const auto &operands = definingInst->getOperands();
    if (operands.empty())
        return false;

    // Verify that the register being rematerialized is indeed the destination (Operand 0)
    if (!operands[0]->isOfType<MirRegister>() || operands[0]->get<MirRegister>()->getRef() != vreg->getRef())
    {
        return false;
    }

    // 1. Integer Immediate Moves: MOV reg, imm (MOV8ri, MOV16ri, MOV32ri, MOV64ri)
    if (desc == TargetInst::MOV8ri || desc == TargetInst::MOV16ri || desc == TargetInst::MOV32ri ||
        desc == TargetInst::MOV64ri)
    {
        if (operands.size() >= 2 && operands[1]->isOfType<MirInteger>())
        {
            return true;
        }
    }

    return false;
}

MirInstruction *EzTestTripleRegisterAllocator::emitReload(RegisterAllocatorCtx *ctx,
                                                          MirBlock *block,
                                                          std::pmr::list<MirInstruction *>::iterator it,
                                                          SourceReference *srcRef,
                                                          MirRegister *dstReg,
                                                          StackFrameObject *spillSlot)
{
    using namespace EzTestTriple;

    MirInstructionBuilder builder(ctx->m_ctx, block, InsertionType::InsertBefore, it);
    MirOperandBuilder oBuilder(ctx->m_ctx);
    MirReference *slotRef = oBuilder.buildRef(spillSlot, srcRef);

    MirType *type = dstReg->getMirType();
    MirTargetInstructionDesc *loadDesc = TargetInst::MOV64rm; // Default fallback

    if (type->getKind() == MirTypeKind::Integer || type->getKind() == MirTypeKind::Pointer)
    {
        switch (type->getTotalSizeInBytes())
        {
            case 1:
                loadDesc = TargetInst::MOV8rm;
                break;
            case 2:
                loadDesc = TargetInst::MOV16rm;
                break;
            case 4:
                loadDesc = TargetInst::MOV32rm;
                break;
            case 8:
                loadDesc = TargetInst::MOV64rm;
                break;
        }
    }
    else if (type->getKind() == MirTypeKind::FloatingPoint)
    {
        loadDesc = (type->getTotalSizeInBytes() == 4) ? TargetInst::MOVSSrm : TargetInst::MOVSDrm;
    }
    else
    {
        ctx->m_ctx->getDiagCollector()->builder(Diag_Error, "EzTestTripleRegisterAllocator")
                << "Can't emit reload for given reg type" << srcRef;
        return nullptr;
    }

    return builder.buildTarget(loadDesc, srcRef, { dstReg, slotRef });
}

MirInstruction *EzTestTripleRegisterAllocator::emitSpill(RegisterAllocatorCtx *ctx,
                                                         MirBlock *block,
                                                         std::pmr::list<MirInstruction *>::iterator it,
                                                         SourceReference *srcRef,
                                                         StackFrameObject *spillSlot,
                                                         MirRegister *srcReg)
{
    using namespace EzTestTriple;

    MirInstructionBuilder builder(ctx->m_ctx, block, InsertionType::InsertAfter, it);
    MirOperandBuilder oBuilder(ctx->m_ctx);
    MirReference *slotRef = oBuilder.buildRef(spillSlot, srcRef);

    MirType *type = srcReg->getMirType();
    MirTargetInstructionDesc *storeDesc = TargetInst::MOV64mr; // Default fallback

    if (type->getKind() == MirTypeKind::Integer || type->getKind() == MirTypeKind::Pointer)
    {
        switch (type->getTotalSizeInBytes())
        {
            case 1:
                storeDesc = TargetInst::MOV8mr;
                break;
            case 2:
                storeDesc = TargetInst::MOV16mr;
                break;
            case 4:
                storeDesc = TargetInst::MOV32mr;
                break;
            case 8:
                storeDesc = TargetInst::MOV64mr;
                break;
        }
    }
    else if (type->getKind() == MirTypeKind::FloatingPoint)
    {
        storeDesc = (type->getTotalSizeInBytes() == 4) ? TargetInst::MOVSSmr : TargetInst::MOVSDmr;
    }
    else
    {
        ctx->m_ctx->getDiagCollector()->builder(Diag_Error, "EzTestTripleRegisterAllocator")
                << "Can't emit spill for given reg type" << srcRef;
        return nullptr;
    }

    return builder.buildTarget(storeDesc, srcRef, { slotRef, srcReg });
}

MirInstruction *EzTestTripleRegisterAllocator::reMaterialize(RegisterAllocatorCtx *ctx,
                                                             MirBlock *block,
                                                             std::pmr::list<MirInstruction *>::iterator it,
                                                             SourceReference *srcRef,
                                                             MirRegister *dstReg,
                                                             MirInstruction *defInst)
{
    MirInstructionBuilder builder(ctx->m_ctx, block, InsertionType::InsertBefore, it);
    MirTargetInstructionDesc *desc = defInst->getTargetDesc();
    MirOperand *constVal = defInst->getOperands()[1];

    return builder.buildTarget(desc, srcRef, { dstReg, constVal });
}