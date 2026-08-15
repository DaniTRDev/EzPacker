#include "FrameLowerer/EzTestTripleFrameLowerer.h"

using namespace EzTestTriple;

int64_t EzTestTripleFrameLowerer::calculateStaticFrameAdjustment(const MirFunctionAnalysisData *analysisData,
                                                                 size_t stackAlign) const
{
    if (!analysisData)
        return 0;

    int64_t rawPayload = static_cast<int64_t>(analysisData->m_totalFrameSize - analysisData->m_calleeSavedAreaSize);
    if (rawPayload <= 0)
        return 0;

    int64_t mask = static_cast<int64_t>(stackAlign - 1);
    return (rawPayload + mask) & ~mask;
}

void EzTestTripleFrameLowerer::insertPrologue(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    TargetDesc *targetDesc = ctx.m_targetDesc;
    CallingConvDesc *cc = func->getCallingConv();
    MirBlock *entryBlock = func->getEntryPoint();
    MirFunctionAnalysisData *analysisData = func->getAnalysisData();

    if (!entryBlock || entryBlock->getInstructions().empty())
        return;

    auto &instructions = entryBlock->getInstructions();
    MirInstructionBuilder iBuilder(ctx.m_ctx, entryBlock, InsertionType::InsertBefore, instructions.begin());
    MirOperandBuilder oBuilder(ctx.m_ctx);

    SourceReference *srcRef = entryBlock->getSourceRef();

    MirType *i64Type = ctx.m_ctx->getTypeTable()->i64();
    MirRegisterClass *gpr64 = Banks::GPR->getClass("GPR64");

    RegisterRef fpRef = cc->getFramePointerReg();
    RegisterRef spRef = cc->getStackPointerReg();

    MirRegister *fpReg = oBuilder.buildPhysReg(i64Type, fpRef.getId(), "rfp", gpr64);
    MirRegister *spReg = oBuilder.buildPhysReg(i64Type, spRef.getId(), "rsp", gpr64);

    const bool useFramePointer = cc->hasFramePointer(func);

    // 1. Frame Pointer Setup: PUSH RFP; MOV RFP, RSP
    if (useFramePointer)
    {
        func->addCalleeSavedRegUse(fpRef);
        iBuilder.buildTarget(TargetInst::PUSH64r, srcRef, { fpReg });
        iBuilder.buildTarget(TargetInst::MOV64rr, srcRef, { fpReg, spReg });
    }

    // 2. Preserve Callee-Saved Registers: PUSH Reg
    const auto &usedCalleeSavedRegs = func->getUsedCalleeSavedRegs();
    for (const RegisterRef &physRegRef : usedCalleeSavedRegs)
    {
        if (useFramePointer && physRegRef == fpRef)
            continue;

        MirRegister *savedReg = oBuilder.buildPhysReg(i64Type, physRegRef.getId());
        savedReg->setClass(gpr64);
        iBuilder.buildTarget(TargetInst::PUSH64r, srcRef, { savedReg });
    }

    // 3. Allocate Stack Frame Payload: SUB RSP, stackAllocSize
    int64_t stackAllocSize = calculateStaticFrameAdjustment(analysisData, cc->getStackAlignment());
    if (stackAllocSize > 0)
    {
        MirOperand *immOp = oBuilder.buildInt(i64Type, FlexInt(stackAllocSize, 64));
        iBuilder.buildTarget(TargetInst::SUB64rr, srcRef, { spReg, immOp });
    }
}

void EzTestTripleFrameLowerer::insertEpilogue(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    TargetDesc *targetDesc = ctx.m_targetDesc;
    if (!func || !targetDesc)
        return;

    CallingConvDesc *cc = func->getCallingConv();
    if (!cc)
        return;

    MirOperandBuilder oBuilder(ctx.m_ctx);
    MirFunctionAnalysisData *analysisData = func->getAnalysisData();

    MirType *i64Type = ctx.m_ctx->getTypeTable()->i64();
    MirRegisterBank *gprBank = targetDesc->getAvailableRegisterBanks()[0];
    MirRegisterClass *gpr64 = gprBank->getClass("GPR64");

    RegisterRef fpRef = cc->getFramePointerReg();
    RegisterRef spRef = cc->getStackPointerReg();

    MirRegister *fpReg = oBuilder.buildPhysReg(i64Type, fpRef.getId(), "rfp", gpr64);
    MirRegister *spReg = oBuilder.buildPhysReg(i64Type, spRef.getId(), "rsp", gpr64);

    const auto &usedCalleeSavedRegs = func->getUsedCalleeSavedRegs();
    const bool useFramePointer = cc->hasFramePointer(func) || (analysisData && analysisData->m_hasDynamicAllocs);
    const int64_t stackAllocSize = calculateStaticFrameAdjustment(analysisData, cc->getStackAlignment());

    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        for (auto it = instructions.begin(); it != instructions.end(); ++it)
        {
            MirInstruction *inst = *it;
            const bool isRet =
                    inst->getFlags() & MirInstructionFlags::IsReturn || inst->getTargetDesc() == TargetInst::RET;

            if (!isRet)
                continue;

            MirInstructionBuilder iBuilder(ctx.m_ctx, block, InsertionType::InsertBefore, it);
            SourceReference *srcRef = inst->getSourceRef();

            // 1. Reclaim Stack Space
            if (useFramePointer)
            {
                int64_t calleeSaveOffset = analysisData ? static_cast<int64_t>(analysisData->m_calleeSavedAreaSize) : 0;
                if (calleeSaveOffset > 0)
                {
                    // LEA / MOV64rm: RSP = RFP - calleeSaveOffset
                    MirOperand *memOp = oBuilder.buildMem(i64Type, fpReg, FlexInt(-calleeSaveOffset, 64));
                    iBuilder.buildTarget(TargetInst::MOV64rm, srcRef, { spReg, memOp });
                }
                else
                {
                    // MOV RSP, RFP
                    iBuilder.buildTarget(TargetInst::MOV64rr, srcRef, { spReg, fpReg });
                }

                // 2. Restore Callee-Saved Registers in Reverse Order
                for (auto regIt = usedCalleeSavedRegs.rbegin(); regIt != usedCalleeSavedRegs.rend(); ++regIt)
                {
                    if (*regIt == fpRef)
                        continue;

                    MirRegister *savedReg = oBuilder.buildPhysReg(i64Type, regIt->getId());
                    savedReg->setClass(gpr64);
                    iBuilder.buildTarget(TargetInst::POP64r, srcRef, { savedReg });
                }

                // POP RFP
                iBuilder.buildTarget(TargetInst::POP64r, srcRef, { fpReg });
            }
            else
            {
                if (stackAllocSize > 0)
                {
                    MirOperand *immOp = oBuilder.buildInt(i64Type, FlexInt(stackAllocSize, 64));
                    iBuilder.buildTarget(TargetInst::ADD64rr, srcRef, { spReg, immOp });
                }

                // Restore Callee-Saved Registers in Reverse Order
                for (auto regIt = usedCalleeSavedRegs.rbegin(); regIt != usedCalleeSavedRegs.rend(); ++regIt)
                {
                    MirRegister *savedReg = oBuilder.buildPhysReg(i64Type, regIt->getId());
                    savedReg->setClass(gpr64);
                    iBuilder.buildTarget(TargetInst::POP64r, srcRef, { savedReg });
                }
            }
        }
    }
}

bool EzTestTripleFrameLowerer::lowerAlloc(FrameLowererCtx &ctx)
{
    MirInstruction *instr = *ctx.m_allocIt;
    if (!instr || instr->getTargetDesc() != TargetInst::ALLOC)
    {
        return false;
    }

    MirFunction *func = ctx.m_targetFunc;
    MirOperand *dst = instr->getOperands()[0];
    MirType *allocTypePtr = dst->getMirType();
    MirType *allocType = allocTypePtr->getPointedType();

    StackFrameObject *obj = func->getStackFrame()->createStaticStackObj(allocType);
    MirInstructionBuilder iBuilder(ctx.m_ctx,
                                   instr->getOwner(),
                                   InsertionType::InsertBefore,
                                   instr->getOwner()->getInstructions().begin());
    MirOperandBuilder oBuilder(ctx.m_ctx);

    MirInstruction *newInstr = iBuilder.LEA(instr->getSourceRef(), dst, oBuilder.buildRef(obj, instr->getSourceRef()));

    auto log = ctx.m_ctx->getDiagCollector()->builder(Diag_Trace, "MirFrameLowerer");
    log << "Lowered ALLOC in function" << func->getSourceRef();
    log.appendNote(
            std::format("Original instr: {}", MirPrinter::printToString(instr, MirPrinterDetail::Detailed)).c_str(),
            instr->getSourceRef());
    log.appendNote(std::format("Assigned stack obj: {}", MirPrinter::printToString(obj)).c_str(),
                   instr->getSourceRef());
    log.appendNote(
            std::format("New instr: {}", MirPrinter::printToString(newInstr, MirPrinterDetail::Detailed)).c_str(),
            newInstr->getSourceRef());

    instr->getOwner()->getInstructions().erase(ctx.m_allocIt);
    return true;
}

bool EzTestTripleFrameLowerer::lowerDAlloc(FrameLowererCtx &ctx)
{
    MirInstruction *instr = *ctx.m_allocIt;
    if (!instr || instr->getTargetDesc() != TargetInst::DYNAMIC_ALLOC)
    {
        return false;
    }

    MirFunction *func = ctx.m_targetFunc;
    TargetDesc *targetDesc = ctx.m_targetDesc;
    CallingConvDesc *cc = func->getCallingConv();

    // Mark dynamic allocation in analysis data
    MirFunctionAnalysisData *analysisData = func->getAnalysisData();
    if (analysisData)
    {
        analysisData->m_hasDynamicAllocs = true;
    }

    MirOperand *dstOp = instr->getOperands()[0];
    MirOperand *sizeOp = instr->getOperands()[1];

    MirBlock *block = instr->getOwner();
    SourceReference *srcRef = instr->getSourceRef();

    MirInstructionBuilder iBuilder(ctx.m_ctx, block, InsertionType::InsertBefore, ctx.m_allocIt);
    MirOperandBuilder oBuilder(ctx.m_ctx);

    MirType *i64Type = ctx.m_ctx->getTypeTable()->i64();
    MirRegisterBank *gprBank = targetDesc->getAvailableRegisterBanks()[0];
    MirRegisterClass *gpr64 = gprBank->getClass("GPR64");

    RegisterRef spRef = cc->getStackPointerReg();
    MirRegister *spReg = oBuilder.buildPhysReg(i64Type, spRef.getId(), "rsp");
    spReg->setClass(gpr64);

    const size_t stackAlign = cc->getStackAlignment();
    uint64_t maskValue = ~static_cast<uint64_t>(stackAlign - 1);

    MirOperand *alignPadding = oBuilder.buildInt(i64Type, FlexInt(static_cast<int64_t>(stackAlign - 1), 64));
    MirOperand *alignMask = oBuilder.buildInt(i64Type, FlexInt(static_cast<int64_t>(maskValue), 64));

    // Align runtime allocation size: (size + (align - 1)) & ~(align - 1)
    iBuilder.buildTarget(TargetInst::ADD64rr, srcRef, { sizeOp, alignPadding });
    iBuilder.buildTarget(TargetInst::AND64rr, srcRef, { sizeOp, alignMask });

    // Decrement SP by the aligned size
    iBuilder.buildTarget(TargetInst::SUB64rr, srcRef, { spReg, sizeOp });

    // Copy new SP into destination pointer register
    iBuilder.buildTarget(TargetInst::MOV64rr, srcRef, { dstOp, spReg });

    ctx.m_allocIt = block->getInstructions().erase(ctx.m_allocIt);
    return true;
}