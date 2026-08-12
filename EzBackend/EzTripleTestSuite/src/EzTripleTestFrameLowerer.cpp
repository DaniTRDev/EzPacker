#include "EzTripleTestFrameLowerer.h"

void EzTripleTestFrameLowerer::insertPrologue(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    TargetDesc *targetDesc = ctx.m_targetDesc;
    CallingConvDesc *cc = func->getCallingConv();
    MirBlock *entryBlock = func->getEntryPoint();

    auto &instructions = entryBlock->getInstructions();
    MirInstructionBuilder iBuilder(ctx.m_ctx, entryBlock, InsertionType::InsertBefore, instructions.begin());
    MirOperandBuilder oBuilder(ctx.m_ctx);

    const FrameLayout &layout = ctx.m_layout;
    SourceReference *srcRef = entryBlock->getSourceRef();

    const size_t slotSize = targetDesc->getStackSlotSize();
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getIntegerTypeBySize(slotSize);

    RegisterRef fpRegRef = cc->getFramePointerReg(), spRegRef = cc->getStackPointerReg();
    MirRegister *fpReg = oBuilder.buildPhysReg(ptrType, fpRegRef.getId(), "fp"),
                *spReg = oBuilder.buildPhysReg(ptrType, spRegRef.getId(), "sp");

    // Enforce FP if ABI mandates it OR if the pass detected dynamic stack allocations on func
    const bool useFramePointer = cc->hasFramePointer(func) || ctx.m_layout.m_hasDynamicAllocs;

    // Frame Pointer Setup: PUSH FP; MOV FP, SP
    if (useFramePointer)
    {
        func->addCalleeSavedRegUse(fpRegRef);
        iBuilder.PUSH(srcRef, fpReg);
        iBuilder.MOV(srcRef, fpReg, spReg);
    }

    // Save Callee-Saved Physical Registers
    const auto &usedCalleeSavedRegs = func->getUsedCalleeSavedRegs();
    for (const RegisterRef &physRegRef : usedCalleeSavedRegs)
    {
        // Skip FP as it was already pushed above
        if (useFramePointer && physRegRef == fpRegRef)
            continue;

        iBuilder.PUSH(srcRef, oBuilder.buildPhysReg(ptrType, physRegRef.getId()));
    }

    // Allocate Static Stack Frame Payload (Locals, Spills & Shadow Space)
    int64_t stackAllocSize = static_cast<int64_t>(layout.totalFrameSize - layout.calleeSavedAreaSize);
    if (stackAllocSize > 0)
    {
        MirOperand *immOp = oBuilder.buildInt(ptrType, FlexInt(static_cast<int32_t>(stackAllocSize), 32));
        iBuilder.SUB(srcRef, spReg, immOp);
    }
}

void EzTripleTestFrameLowerer::insertEpilogue(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    TargetDesc *targetDesc = ctx.m_targetDesc;
    if (!func || !targetDesc)
        return;

    CallingConvDesc *cc = func->getCallingConv();
    if (!cc)
        return;

    MirOperandBuilder oBuilder(ctx.m_ctx);
    const FrameLayout &layout = ctx.m_layout;

    const size_t slotSize = targetDesc->getStackSlotSize();
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getIntegerTypeBySize(slotSize);

    RegisterRef fpRegRef = cc->getFramePointerReg();
    RegisterRef spRegRef = cc->getStackPointerReg();

    MirRegister *fpReg = oBuilder.buildPhysReg(ptrType, fpRegRef.getId(), "fp");
    MirRegister *spReg = oBuilder.buildPhysReg(ptrType, spRegRef.getId(), "sp");

    const auto &usedCalleeSavedRegs = func->getUsedCalleeSavedRegs();
    int64_t stackAllocSize = static_cast<int64_t>(layout.totalFrameSize - layout.calleeSavedAreaSize);

    const bool useFramePointer = cc->hasFramePointer(func) || ctx.m_layout.m_hasDynamicAllocs;

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        for (auto it = instructions.begin(); it != instructions.end(); ++it)
        {
            MirInstruction *inst = *it;
            if (!(inst->getFlags() & MirInstructionFlags::IsReturn))
                continue;

            MirInstructionBuilder iBuilder(ctx.m_ctx, block, InsertionType::InsertBefore, it);
            SourceReference *srcRef = inst->getSourceRef();

            if (useFramePointer)
            {
                int64_t calleeSaveOffset = static_cast<int64_t>(layout.calleeSavedAreaSize);
                if (calleeSaveOffset > 0)
                {
                    iBuilder.LEA(
                            srcRef,
                            spReg,
                            oBuilder.buildMem(ptrType, fpReg, FlexInt(static_cast<int32_t>(calleeSaveOffset), 32)));
                }
                else
                {
                    iBuilder.MOV(srcRef, spReg, fpReg);
                }

                for (auto regIt = usedCalleeSavedRegs.rbegin(); regIt != usedCalleeSavedRegs.rend(); ++regIt)
                {
                    if (*regIt == fpRegRef)
                        continue;

                    iBuilder.POP(srcRef, oBuilder.buildPhysReg(ptrType, regIt->getId()));
                }

                iBuilder.POP(srcRef, fpReg);
            }
            else
            {
                if (stackAllocSize > 0)
                {
                    MirOperand *immOp = oBuilder.buildInt(ptrType, FlexInt(static_cast<int32_t>(stackAllocSize), 32));
                    iBuilder.ADD(srcRef, spReg, immOp);
                }

                for (auto regIt = usedCalleeSavedRegs.rbegin(); regIt != usedCalleeSavedRegs.rend(); ++regIt)
                {
                    iBuilder.POP(srcRef, oBuilder.buildPhysReg(ptrType, regIt->getId()));
                }
            }
        }
    }
}

void EzTripleTestFrameLowerer::lowerDAlloc(FrameLowererCtx &ctx)
{
    MirInstruction *instr = *ctx.m_allocIt;
    if (!instr || instr->getOpCode() != MirInstructionOpCode::DALLOC)
    {
        ctx.m_ctx->getDiagCollector()->builder(Diag_Error, "EzTripleTestFrameLowerer")
                << "Given FrameLowererCtx does not point to a valid DALLOC instruction";
        return;
    }

    MirFunction *func = ctx.m_targetFunc;
    TargetDesc *targetDesc = ctx.m_targetDesc;
    CallingConvDesc *cc = func->getCallingConv();

    ctx.m_layout.m_hasDynamicAllocs = true;

    MirOperand *dstOp = instr->getOperands()[0];
    MirOperand *sizeOp = instr->getOperands()[1];

    MirBlock *block = instr->getOwner();
    SourceReference *srcRef = instr->getSourceRef();

    MirInstructionBuilder iBuilder(ctx.m_ctx, block, InsertionType::InsertBefore, ctx.m_allocIt);
    MirOperandBuilder oBuilder(ctx.m_ctx);

    const size_t slotSize = targetDesc->getStackSlotSize();
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getIntegerTypeBySize(slotSize);

    RegisterRef spRegRef = cc->getStackPointerReg();
    MirRegister *spReg = oBuilder.buildPhysReg(ptrType, spRegRef.getId(), "sp");

    const size_t stackAlign = cc->getStackAlignment();
    uint64_t maskValue = ~static_cast<uint64_t>(stackAlign - 1);

    MirOperand *alignPadding = oBuilder.buildInt(ptrType, FlexInt(static_cast<int64_t>(stackAlign - 1)));
    MirOperand *alignMask = oBuilder.buildInt(ptrType, FlexInt(static_cast<int64_t>(maskValue)));

    iBuilder.ADD(srcRef, sizeOp, alignPadding);
    iBuilder.AND(srcRef, sizeOp, alignMask);

    iBuilder.SUB(srcRef, spReg, sizeOp);
    iBuilder.MOV(srcRef, dstOp, spReg);

    ctx.m_allocIt = block->getInstructions().erase(ctx.m_allocIt);
}