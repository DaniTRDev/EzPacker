#include "Targets/X86_64/X86_64FrameLowerer.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Function/CallingConvDesc.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"
#include "FlexNumber/FlexInt.h"
#include "x86_64TargetInstructionTable.h"

namespace EzTriple
{

/**
 * Emits the x86-64 prologue at the top of the entry block: optional push rbp/mov rbp,rsp,
 * sub rsp,frameSize, then pushes of the used callee-saved registers.
 */
void X86_64FrameLowerer::insertPrologue(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    if (!func)
    {
        return;
    }
    CallingConvDesc *cc = func->getCallingConv();
    if (!cc)
    {
        return;
    }

    MirBlock *entryBlock = func->getBlocks().empty() ? nullptr : func->getBlocks().front();
    if (!entryBlock)
    {
        return;
    }

    MirFunctionAnalysisData *analysisData = func->getAnalysisData();
    MirOperandBuilder opBuilder(ctx.m_ctx);
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getPtr(ctx.m_ctx->getTypeTable()->_void());
    MirType *i64 = ctx.m_ctx->getTypeTable()->i64();
    SourceReference *srcRef = func->getSourceRef();

    bool hasFP = cc->hasFramePointer(func);
    MirRegisterRef fpRef = cc->getFramePointerReg();
    MirRegisterRef spRef = cc->getStackPointerReg();
    MirRegister *fpReg = opBuilder.buildPhysReg(ptrType, fpRef.getId(), "rbp", fpRef.getClass());
    MirRegister *spReg = opBuilder.buildPhysReg(ptrType, spRef.getId(), "rsp", spRef.getClass());

    const auto *descPUSH64r = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::PUSH64r);
    const auto *descMOV64rr = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::MOV64rr);
    const auto *descSUB64ri = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::SUB64ri);

    auto &instrList = entryBlock->getInstructions();
    auto it = instrList.begin();
    MirInstructionBuilder builder(ctx.m_ctx,
                                  entryBlock,
                                  it != instrList.end() ? InsertionType::InsertBefore : InsertionType::Append,
                                  it);

    // 1. Set up standard stack frame pointer if required:
    //    pushq %rbp
    //    movq %rsp, %rbp
    if (hasFP)
    {
        builder.buildTarget(descPUSH64r, srcRef, { fpReg });
        builder.buildTarget(descMOV64rr, srcRef, { fpReg, spReg });
    }

    // 2. Allocate total frame stack space:
    //    subq $frameSize, %rsp
    if (analysisData && analysisData->m_totalFrameSize > 0)
    {
        MirInteger *sizeImm = opBuilder.buildInt(i64, FlexInt(static_cast<int64_t>(analysisData->m_totalFrameSize)));
        builder.buildTarget(descSUB64ri, srcRef, { spReg, spReg, sizeImm });
    }

    // 3. Push callee-saved registers:
    const auto &usedCallees = func->getUsedCalleeSavedRegs();
    for (const MirRegisterRef &regRef : usedCallees)
    {
        if (hasFP && regRef == fpRef)
        {
            continue;
        }
        MirRegister *calleeReg = opBuilder.buildPhysReg(ptrType, regRef.getId(), "", regRef.getClass());
        builder.buildTarget(descPUSH64r, srcRef, { calleeReg });
    }
}

/**
 * Emits the x86-64 epilogue before every return: pops callee-saved registers in reverse, then
 * restores rsp/rbp (or adjusts rsp for frame-less functions).
 */
void X86_64FrameLowerer::insertEpilogue(FrameLowererCtx &ctx)
{
    MirFunction *func = ctx.m_targetFunc;
    if (!func)
    {
        return;
    }
    CallingConvDesc *cc = func->getCallingConv();
    if (!cc)
    {
        return;
    }

    MirFunctionAnalysisData *analysisData = func->getAnalysisData();
    MirOperandBuilder opBuilder(ctx.m_ctx);
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getPtr(ctx.m_ctx->getTypeTable()->_void());
    MirType *i64 = ctx.m_ctx->getTypeTable()->i64();

    bool hasFP = cc->hasFramePointer(func);
    MirRegisterRef fpRef = cc->getFramePointerReg();
    MirRegisterRef spRef = cc->getStackPointerReg();
    MirRegister *fpReg = opBuilder.buildPhysReg(ptrType, fpRef.getId(), "rbp", fpRef.getClass());
    MirRegister *spReg = opBuilder.buildPhysReg(ptrType, spRef.getId(), "rsp", spRef.getClass());

    const auto *descPOP64r = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::POP64r);
    const auto *descMOV64rr = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::MOV64rr);
    const auto *descADD64ri = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::ADD64ri);

    const auto &usedCallees = func->getUsedCalleeSavedRegs();

    for (MirBlock *block : func->getBlocks())
    {
        for (auto it = block->getInstructions().begin(); it != block->getInstructions().end(); ++it)
        {
            MirInstruction *inst = *it;
            if (!inst)
            {
                continue;
            }

            bool isRet = (inst->getOpCode() == MirInstructionOpCode::RET);
            if (!isRet && inst->getTargetDesc())
            {
                isRet = (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsReturn) != 0;
            }

            if (!isRet)
            {
                continue;
            }

            MirInstructionBuilder epilogueBuilder(ctx.m_ctx, inst, InsertionType::InsertBefore);
            SourceReference *srcRef = inst->getSourceRef();

            // 1. Pop callee-saved registers in reverse order:
            for (auto rIt = usedCallees.rbegin(); rIt != usedCallees.rend(); ++rIt)
            {
                if (hasFP && *rIt == fpRef)
                {
                    continue;
                }
                MirRegister *calleeReg = opBuilder.buildPhysReg(ptrType, rIt->getId(), "", rIt->getClass());
                epilogueBuilder.buildTarget(descPOP64r, srcRef, { calleeReg });
            }

            // 2. Restore stack pointer and frame pointer:
            if (hasFP)
            {
                epilogueBuilder.buildTarget(descMOV64rr, srcRef, { spReg, fpReg });
                epilogueBuilder.buildTarget(descPOP64r, srcRef, { fpReg });
            }
            else if (analysisData && analysisData->m_totalFrameSize > 0)
            {
                MirInteger *sizeImm =
                        opBuilder.buildInt(i64, FlexInt(static_cast<int64_t>(analysisData->m_totalFrameSize)));
                epilogueBuilder.buildTarget(descADD64ri, srcRef, { spReg, spReg, sizeImm });
            }
        }
    }
}

/**
 * Converts a static ALLOC in place into a LEA64r that computes the address of a newly created
 * static stack object, so later frame layout can assign its final offset.
 */
bool X86_64FrameLowerer::lowerAlloc(FrameLowererCtx &ctx)
{
    MirInstruction *allocInst = *ctx.m_allocIt;
    if (!allocInst || allocInst->getOperandCount() < 1)
    {
        return false;
    }

    MirRegister *dst = allocInst->getOpAs<MirRegister>(0);
    if (!dst)
    {
        return false;
    }

    MirType *allocType = nullptr;
    if (dst->getMirType() && dst->getMirType()->getKind() == MirTypeKind::Pointer)
    {
        allocType = dst->getMirType()->getPointedType();
    }
    if (!allocType || allocType->getKind() == MirTypeKind::Void)
    {
        allocType = ctx.m_ctx->getTypeTable()->i64();
    }

    StackFrameObject *stackObj = ctx.m_targetFunc->getStackFrame()->createStaticStackObj(allocType);
    MirOperandBuilder opBuilder(ctx.m_ctx);
    MirReference *slotRef = opBuilder.buildRef(stackObj, allocInst->getSourceRef());

    const auto *descLEA64r = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::LEA64r);

    allocInst->setOpcode(MirInstructionOpCode::TARGET_INST);
    allocInst->setTargetDesc(descLEA64r);

    MirInstructionBuilder ib(ctx.m_ctx, allocInst, InsertionType::InsertBefore);
    ib.clearOperands(allocInst);
    ib.addOperand(allocInst, dst);
    ib.addOperand(allocInst, slotRef);

    return true;
}

/**
 * Lowers a dynamic DALLOC by first decrementing rsp by the requested size and then turning the
 * DALLOC in place into a MOV64rr that captures the resulting stack pointer into the destination.
 */
bool X86_64FrameLowerer::lowerDAlloc(FrameLowererCtx &ctx)
{
    MirInstruction *dallocInst = *ctx.m_allocIt;
    if (!dallocInst || dallocInst->getOperandCount() < 2)
    {
        return false;
    }

    MirRegister *dst = dallocInst->getOpAs<MirRegister>(0);
    MirRegister *src = dallocInst->getOpAs<MirRegister>(1);
    if (!dst || !src)
    {
        return false;
    }

    CallingConvDesc *cc = ctx.m_targetFunc->getCallingConv();
    if (!cc)
    {
        return false;
    }

    MirOperandBuilder opBuilder(ctx.m_ctx);
    MirType *ptrType = ctx.m_ctx->getTypeTable()->getPtr(ctx.m_ctx->getTypeTable()->_void());
    MirRegisterRef spRef = cc->getStackPointerReg();
    MirRegister *spReg = opBuilder.buildPhysReg(ptrType, spRef.getId(), "rsp", spRef.getClass());

    const auto *descSUB64rr = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::SUB64rr);
    const auto *descMOV64rr = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::MOV64rr);

    MirInstructionBuilder ib(ctx.m_ctx, dallocInst, InsertionType::InsertBefore);
    // 1. subq %src, %rsp
    ib.buildTarget(descSUB64rr, dallocInst->getSourceRef(), { spReg, spReg, src });

    // 2. Transform dalloc in place: movq %rsp, %dst
    dallocInst->setOpcode(MirInstructionOpCode::TARGET_INST);
    dallocInst->setTargetDesc(descMOV64rr);
    ib.clearOperands(dallocInst);
    ib.addOperand(dallocInst, dst);
    ib.addOperand(dallocInst, spReg);

    return true;
}

} // namespace EzTriple
