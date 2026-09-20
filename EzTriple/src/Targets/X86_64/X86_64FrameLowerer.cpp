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

namespace
{

/**
 * Shared inputs resolved for both prologue and epilogue emission: the owning function and its
 * calling convention, the frame analysis data, the physical frame/stack registers, and the
 * target descriptors used by the standard x86-64 frame sequences.
 */
struct FrameLoweringSetup
{
    MirFunction *m_func{ nullptr };
    CallingConvDesc *m_cc{ nullptr };
    MirFunctionAnalysisData *m_analysisData{ nullptr };
    MirType *m_ptrType{ nullptr };
    MirType *m_i64{ nullptr };
    bool m_hasFP{ false };
    MirRegisterRef m_fpRef;
    MirRegisterRef m_spRef;
    MirRegister *m_fpReg{ nullptr };
    MirRegister *m_spReg{ nullptr };
    const MirTargetInstructionDesc *m_descPush{ nullptr };
    const MirTargetInstructionDesc *m_descPop{ nullptr };
    const MirTargetInstructionDesc *m_descMov{ nullptr };
    const MirTargetInstructionDesc *m_descSub{ nullptr };
    const MirTargetInstructionDesc *m_descAdd{ nullptr };
};

/**
 * Resolves the shared prologue/epilogue inputs. Returns false when the function or its calling
 * convention is unavailable, in which case the caller must emit nothing.
 */
bool resolveFrameLoweringSetup(FrameLowererCtx &ctx, MirOperandBuilder &opBuilder, FrameLoweringSetup &setup)
{
    MirFunction *func = ctx.m_targetFunc;
    if (!func)
    {
        return false;
    }
    CallingConvDesc *cc = func->getCallingConv();
    if (!cc)
    {
        return false;
    }

    setup.m_func = func;
    setup.m_cc = cc;
    setup.m_analysisData = func->getAnalysisData();
    setup.m_ptrType = ctx.m_ctx->getTypeTable()->getPtr(ctx.m_ctx->getTypeTable()->_void());
    setup.m_i64 = ctx.m_ctx->getTypeTable()->i64();
    setup.m_hasFP = cc->hasFramePointer(func);
    setup.m_fpRef = cc->getFramePointerReg();
    setup.m_spRef = cc->getStackPointerReg();
    setup.m_fpReg = opBuilder.buildPhysReg(setup.m_ptrType, setup.m_fpRef.getId(), "rbp", setup.m_fpRef.getClass());
    setup.m_spReg = opBuilder.buildPhysReg(setup.m_ptrType, setup.m_spRef.getId(), "rsp", setup.m_spRef.getClass());

    setup.m_descPush = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::PUSH64r);
    setup.m_descPop = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::POP64r);
    setup.m_descMov = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::MOV64rr);
    setup.m_descSub = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::SUB64ri);
    setup.m_descAdd = EzTriple::x86_64TargetInst::getTargetDesc(EzTriple::x86_64TargetInst::ADD64ri);
    return true;
}

} // namespace

/**
 * Emits the x86-64 prologue at the top of the entry block: optional push rbp/mov rbp,rsp,
 * sub rsp,frameSize, then pushes of the used callee-saved registers.
 */
void X86_64FrameLowerer::insertPrologue(FrameLowererCtx &ctx)
{
    MirOperandBuilder opBuilder(ctx.m_ctx);
    FrameLoweringSetup setup;
    if (!resolveFrameLoweringSetup(ctx, opBuilder, setup))
    {
        return;
    }

    MirBlock *entryBlock = setup.m_func->getBlocks().empty() ? nullptr : setup.m_func->getBlocks().front();
    if (!entryBlock)
    {
        return;
    }

    SourceReference *srcRef = setup.m_func->getSourceRef();

    auto &instrList = entryBlock->getInstructions();
    auto it = instrList.begin();
    MirInstructionBuilder builder(ctx.m_ctx,
                                  entryBlock,
                                  it != instrList.end() ? InsertionType::InsertBefore : InsertionType::Append,
                                  it);

    // 1. Set up standard stack frame pointer if required:
    //    pushq %rbp
    //    movq %rsp, %rbp
    if (setup.m_hasFP)
    {
        builder.buildTarget(setup.m_descPush, srcRef, { setup.m_fpReg });
        builder.buildTarget(setup.m_descMov, srcRef, { setup.m_fpReg, setup.m_spReg });
    }

    // 2. Allocate total frame stack space:
    //    subq $frameSize, %rsp
    if (setup.m_analysisData && setup.m_analysisData->m_totalFrameSize > 0)
    {
        MirInteger *sizeImm =
                opBuilder.buildInt(setup.m_i64, FlexInt(static_cast<int64_t>(setup.m_analysisData->m_totalFrameSize)));
        builder.buildTarget(setup.m_descSub, srcRef, { setup.m_spReg, setup.m_spReg, sizeImm });
    }

    // 3. Push callee-saved registers:
    const auto &usedCallees = setup.m_func->getUsedCalleeSavedRegs();
    for (const MirRegisterRef &regRef : usedCallees)
    {
        if (setup.m_hasFP && regRef == setup.m_fpRef)
        {
            continue;
        }
        MirRegister *calleeReg = opBuilder.buildPhysReg(setup.m_ptrType, regRef.getId(), "", regRef.getClass());
        builder.buildTarget(setup.m_descPush, srcRef, { calleeReg });
    }
}

/**
 * Emits the x86-64 epilogue before every return: pops callee-saved registers in reverse, then
 * restores rsp/rbp (or adjusts rsp for frame-less functions).
 */
void X86_64FrameLowerer::insertEpilogue(FrameLowererCtx &ctx)
{
    MirOperandBuilder opBuilder(ctx.m_ctx);
    FrameLoweringSetup setup;
    if (!resolveFrameLoweringSetup(ctx, opBuilder, setup))
    {
        return;
    }

    const auto &usedCallees = setup.m_func->getUsedCalleeSavedRegs();

    for (MirBlock *block : setup.m_func->getBlocks())
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
                if (setup.m_hasFP && *rIt == setup.m_fpRef)
                {
                    continue;
                }
                MirRegister *calleeReg = opBuilder.buildPhysReg(setup.m_ptrType, rIt->getId(), "", rIt->getClass());
                epilogueBuilder.buildTarget(setup.m_descPop, srcRef, { calleeReg });
            }

            // 2. Restore stack pointer and frame pointer:
            if (setup.m_hasFP)
            {
                epilogueBuilder.buildTarget(setup.m_descMov, srcRef, { setup.m_spReg, setup.m_fpReg });
                epilogueBuilder.buildTarget(setup.m_descPop, srcRef, { setup.m_fpReg });
            }
            else if (setup.m_analysisData && setup.m_analysisData->m_totalFrameSize > 0)
            {
                MirInteger *sizeImm = opBuilder.buildInt(
                        setup.m_i64, FlexInt(static_cast<int64_t>(setup.m_analysisData->m_totalFrameSize)));
                epilogueBuilder.buildTarget(setup.m_descAdd, srcRef, { setup.m_spReg, setup.m_spReg, sizeImm });
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
