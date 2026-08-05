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

    // Target native pointer size (e.g., i64 for 64-bit target, i32 for 32-bit target)
    const size_t slotSize = targetDesc->getStackSlotSize();
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getIntegerTypeBySize(slotSize);

    RegisterRef fpRegRef = cc->getFramePointerReg(), spRegRef = cc->getStackPointerReg();
    MirRegister *fpReg = oBuilder.buildPhysReg(ptrType, fpRegRef.getId(), "fp"),
                *spReg = oBuilder.buildPhysReg(ptrType, spRegRef.getId(), "sp");

    // Frame Pointer Setup: PUSH FP; MOV FP, SP
    if (cc->hasFramePointer(func))
    {
        iBuilder.PUSH(srcRef, fpReg);
        iBuilder.MOV(srcRef, fpReg, spReg);
    }

    // Save Callee-Saved Physical Registers
    const auto &usedCalleeSavedRegs = func->getUsedCalleeSavedRegs();
    for (const RegisterRef &physRegRef : usedCalleeSavedRegs)
    {
        iBuilder.PUSH(srcRef, oBuilder.buildPhysReg(ptrType, physRegRef.getId()));
    }

    // Allocate Stack Frame Objects (Locals, Spills & Shadow Space)
    int64_t stackAllocSize = static_cast<int64_t>(layout.totalFrameSize - layout.calleeSavedAreaSize);
    if (stackAllocSize > 0)
    {
        MirOperand *immOp = oBuilder.buildInt(ptrType, FlexInt(int32_t(stackAllocSize), 32));
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

    // Epilogue must be inserted at every return instruction across all blocks
    for (MirBlock *block : func->getBlocks())
    {
        auto &instructions = block->getInstructions();
        for (auto it = instructions.begin(); it != instructions.end(); ++it)
        {
            MirInstruction *inst = *it;
            if (!(inst->getFlags() & MirInstructionFlags::IsReturn))
                continue;

            // Insert epilogue instructions right BEFORE the return instruction
            MirInstructionBuilder iBuilder(ctx.m_ctx, block, InsertionType::InsertBefore, it);
            SourceReference *srcRef = inst->getSourceRef();

            // Deallocate Local Stack Payload
            if (stackAllocSize > 0)
            {
                MirOperand *immOp = oBuilder.buildInt(ptrType, FlexInt(int32_t(stackAllocSize), 32));
                iBuilder.ADD(srcRef, spReg, immOp);
            }

            // Restore Callee-Saved Registers in REVERSE order of PUSH
            for (auto regIt = usedCalleeSavedRegs.rbegin(); regIt != usedCalleeSavedRegs.rend(); ++regIt)
            {
                iBuilder.POP(srcRef, oBuilder.buildPhysReg(ptrType, regIt->getId()));
            }

            // Restore Frame Pointer: POP FP
            if (cc->hasFramePointer(func))
            {
                iBuilder.POP(srcRef, fpReg);
            }
        }
    }
}
